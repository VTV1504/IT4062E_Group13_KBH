#include "typing_engine.h"

TypingEngine::TypingEngine(const std::string& text)
    : target_text(text) {}

void TypingEngine::add_player(int player_id) {
    sessions[player_id] = std::make_unique<TypingSession>(target_text);
}

void TypingEngine::remove_player(int player_id) {
    sessions.erase(player_id);
}

void TypingEngine::on_key(int player_id, char c, double timestamp) {
    auto it = sessions.find(player_id);
    if (it != sessions.end()) {
        it->second->on_key(c, timestamp);
    }
}

bool TypingEngine::all_finished() const {
    for (const auto& kv : sessions) {
        if (!kv.second->finished()) {
            return false;
        }
    }
    return true;
}

const TypingSession* TypingEngine::get_session(int player_id) const {
    auto it = sessions.find(player_id);
    if (it == sessions.end()) return nullptr;
    return it->second.get();
}
