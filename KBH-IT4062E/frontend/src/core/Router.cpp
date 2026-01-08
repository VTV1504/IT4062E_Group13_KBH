#include "Router.h"
#include "../app/App.h"

#include "../screens/TitleScreen.h"
#include "../screens/LobbyScreen.h"
#include "../screens/GameScreen.h"
#include "../screens/ProfileScreen.h"

#include "../overlays/EnterRoomOverlay.h"
#include "../overlays/LeaderboardOverlay.h"
#include "../overlays/SignInOverlay.h"
#include "../overlays/CreateAccountOverlay.h"
#include "../overlays/ChangePasswordOverlay.h"
#include "../overlays/GuestResultOverlay.h"
#include "../overlays/UserResultOverlay.h"

void Router::change(RouteId id) {
    app->views().clearAndPush(make(id));
}

void Router::push(RouteId id) {
    app->views().push(make(id));
}

void Router::pop() {
    app->views().pop();
}

void Router::replaceTop(RouteId id) {
    app->views().replaceTop(make(id));
}

std::unique_ptr<View> Router::make(RouteId id) {
    switch (id) {
        case RouteId::Title:               return std::make_unique<TitleScreen>();
        case RouteId::Lobby:               return std::make_unique<LobbyScreen>();
        case RouteId::Game:                return std::make_unique<GameScreen>();
        case RouteId::Profile:             return std::make_unique<ProfileScreen>();

        case RouteId::EnterRoomOverlay:    return std::make_unique<EnterRoomOverlay>();
        case RouteId::LeaderboardOverlay:  return std::make_unique<LeaderboardOverlay>();
        case RouteId::SignInOverlay:       return std::make_unique<SignInOverlay>();
        case RouteId::CreateAccountOverlay:return std::make_unique<CreateAccountOverlay>();
        case RouteId::ChangePasswordOverlay:return std::make_unique<ChangePasswordOverlay>();
        case RouteId::GuestResultOverlay:  return std::make_unique<GuestResultOverlay>();
        case RouteId::UserResultOverlay:   return std::make_unique<UserResultOverlay>();
        default:                           return nullptr;
    }
}
