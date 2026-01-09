#include "App.h"
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "../ui/UiTheme.h"

bool App::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    // PNG support is enough for now
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) return false;

    if (TTF_Init() != 0) return false;

    win = SDL_CreateWindow(
        "Keyboard Hero",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        UiTheme::DesignW,
        UiTheme::DesignH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!win) return false;

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) return false;

    // Use a fixed logical resolution for all screens/overlays
    SDL_RenderSetLogicalSize(ren, UiTheme::DesignW, UiTheme::DesignH);

    res.init(ren);

    net.setOnMessage([this](const Json::Value& payload) { handleServerMessage(payload); });
    if (net.connectTo("127.0.0.1", 5000)) {
        st.setConnectionStatus(true, "Connected to server");
    } else {
        st.setConnectionStatus(false, net.lastError());
    }
    return true;
}

void App::defer(std::function<void()> fn) {
    deferred.push_back(std::move(fn));
}

void App::processDeferred() {
    if (deferred.empty()) return;

    // Move queue out so deferred actions can schedule more deferred actions safely
    auto q = std::move(deferred);
    deferred.clear();

    for (auto& fn : q) {
        if (fn) fn();
    }
}

void App::run() {
    rt.change(RouteId::Title);

    Uint64 last = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    while (!quitRequested) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)((now - last) / freq);
        last = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quitRequested = true;
                break;
            }
            viewStack.handleEvent(e);
        }

        // Important: execute navigation/actions AFTER event processing to avoid use-after-free
        processDeferred();

        viewStack.update(dt);
        net.poll();

        SDL_RenderClear(ren);
        viewStack.render(ren);
        SDL_RenderPresent(ren);
    }
}

void App::shutdown() {
    // Clean up resources safely
    res.shutdown();
    net.disconnect();

    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void App::handleServerMessage(const Json::Value& payload) {
    std::string type = payload.get("type", "").asString();
    if (type.empty()) return;

    if (type == "CONNECTED") {
        st.setConnectionStatus(true, payload.get("message", "connected").asString());
        return;
    }

    if (type == "REGISTER_RESULT" || type == "LOGIN_RESULT" || type == "CHANGE_PASSWORD_RESULT" || type == "GUEST_RESULT") {
        AuthResult result;
        result.type = type;
        result.success = payload.get("status", "FAILED").asString() == "OK";
        result.message = payload.get("message", "").asString();
        result.username = payload.get("username", payload.get("nickname", "")).asString();

        if (type == "LOGIN_RESULT" && result.success) {
            sess.signIn(result.username);
        }
        if (type == "GUEST_RESULT" && result.success) {
            sess.signIn(result.username);
        }
        st.setAuthResult(result);
        return;
    }

    if (type == "ROOM_CREATED" || type == "ROOM_JOIN_RESULT") {
        RoomResult result;
        result.success = payload.get("status", "OK").asString() == "OK";
        result.message = payload.get("message", "").asString();
        result.room_code = payload.get("room_code", "").asString();
        result.mode = payload.get("mode", "").asString();
        result.is_host = (payload.get("host", "").asString() == sess.user().username);
        st.setRoomResult(result);

        if (result.success) {
            st.setRoomInfo(result.room_code, result.mode, result.is_host);
            defer([this]() { router().change(RouteId::Lobby); });
        }
        return;
    }

    if (type == "ROOM_LEFT") {
        st.setRoomInfo("", "", false);
        return;
    }

    if (type == "ALL_READY") {
        st.setAllReady(true);
        return;
    }

    if (type == "SELF_TRAINING_READY") {
        st.setGamePayload(GameMode::Training,
                          payload.get("text", "").asString(),
                          payload.get("duration_seconds", 0).asInt(),
                          0,
                          "");
        defer([this]() { router().change(RouteId::Game); });
        return;
    }

    if (type == "GAME_START") {
        std::string mode = payload.get("mode", "arena").asString();
        GameMode gm = (mode == "survival") ? GameMode::Survival : GameMode::Arena;
        st.setGamePayload(gm,
                          payload.get("text", "").asString(),
                          payload.get("duration_seconds", 0).asInt(),
                          payload.get("stage", 0).asInt(),
                          payload.get("difficulty", "").asString());
        defer([this]() { router().change(RouteId::Game); });
        return;
    }

    if (type == "STAGE_START") {
        st.setGamePayload(GameMode::Survival,
                          payload.get("text", "").asString(),
                          payload.get("duration_seconds", 0).asInt(),
                          payload.get("stage", 0).asInt(),
                          "");
        return;
    }

    if (type == "SELF_TRAINING_END") {
        GameResult result;
        result.mode = GameMode::Training;
        result.wpm = payload.get("wpm", 0.0).asDouble();
        result.accuracy = payload.get("accuracy", 0.0).asDouble();
        result.score = static_cast<int>(result.wpm * 10.0);
        st.setLastResult(result);

        defer([this]() {
            if (sess.isLoggedIn()) router().push(RouteId::UserResultOverlay);
            else router().push(RouteId::GuestResultOverlay);
        });
        return;
    }

    if (type == "GAME_END") {
        std::vector<RankingEntry> entries;
        const Json::Value ranking = payload["ranking"];
        for (const auto& item : ranking) {
            RankingEntry entry;
            entry.name = item.get("name", "").asString();
            entry.finished = item.get("finished", false).asBool();
            entry.progress = item.get("progress", 0).asUInt64();
            entry.finish_time = item.get("finish_time", 0.0).asDouble();
            entry.wpm = item.get("wpm", 0.0).asDouble();
            entry.accuracy = item.get("accuracy", 0.0).asDouble();
            entry.points = item.get("points", 0).asInt();
            entry.survived_stages = item.get("survived_stages", 0).asInt();
            entries.push_back(entry);
        }
        st.setRanking(entries);

        defer([this]() {
            if (sess.isLoggedIn()) router().push(RouteId::UserResultOverlay);
            else router().push(RouteId::GuestResultOverlay);
        });
        return;
    }

    if (type == "SELF_TRAINING_LEADERBOARD") {
        std::vector<LeaderboardEntry> entries;
        for (const auto& item : payload["leaderboard"]) {
            LeaderboardEntry entry;
            entry.username = item.get("username", "").asString();
            entry.value_a = item.get("avg_wpm", 0.0).asDouble();
            entry.value_b = item.get("avg_accuracy", 0.0).asDouble();
            entries.push_back(entry);
        }
        st.setTrainingLeaderboard(entries, payload.get("player_rank", 0).asInt());
        return;
    }

    if (type == "SURVIVAL_LEADERBOARD") {
        std::vector<LeaderboardEntry> entries;
        for (const auto& item : payload["leaderboard"]) {
            LeaderboardEntry entry;
            entry.username = item.get("username", "").asString();
            entry.points = item.get("total_points", 0).asInt();
            entry.rooms = item.get("total_rooms", 0).asInt();
            entries.push_back(entry);
        }
        st.setSurvivalLeaderboard(entries, payload.get("player_rank", 0).asInt());
        return;
    }

    if (type == "ERROR") {
        st.setNotice(payload.get("message", "Error").asString());
    }
}
