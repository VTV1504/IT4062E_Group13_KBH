#include "app/App.h"
#include <iostream>

int main() {
    std::cout << "=== Keyboard Hero Client Starting ===\n";
    
    App app;
    std::cout << "[Main] Calling app.init()...\n";
    if (!app.init()) {
        std::cerr << "[Main] app.init() failed!\n";
        return 1;
    }
    
    std::cout << "[Main] Starting main loop...\n";
    app.run();
    
    std::cout << "[Main] Shutting down...\n";
    app.shutdown();
    
    std::cout << "[Main] Exit.\n";
    return 0;
}
