#pragma once
#include <memory>
#include "View.h"

enum class RouteId {
    Title,
    Lobby,
    Game,
    Profile,

    EnterRoomOverlay,
    JoinRoomOverlay,
    LeaderboardOverlay,
    SignInOverlay,
    CreateAccountOverlay,
    ChangePasswordOverlay,
    GuestResultOverlay,
    UserResultOverlay,
    MatchResultOverlay
};

class App;

class Router {
public:
    explicit Router(App* app) : app(app) {}

    void change(RouteId id);
    void push(RouteId id);
    void pop();
    void replaceTop(RouteId id);

    std::unique_ptr<View> make(RouteId id);

private:
    App* app;
};
