#include "Session.h"

void Session::signIn(const std::string& username) {
    loggedIn = true;
    current.username = username;
    if (onAuthChanged) onAuthChanged();
}

void Session::signOut() {
    loggedIn = false;
    current.username.clear();
    if (onAuthChanged) onAuthChanged();
}
