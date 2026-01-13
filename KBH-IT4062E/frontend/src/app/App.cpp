#include "App.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>

#include "../ui/UiTheme.h"
#include "../config/ClientConfig.h"
#include "../overlays/JoinRoomOverlay.h"
#include "../core/Router.h"

bool App::init() {
    std::cout << "[App] Initializing SDL...\n";
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "[App] SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    // PNG support is enough for now
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        std::cerr << "[App] IMG_Init failed: " << IMG_GetError() << "\n";
        return false;
    }

    if (TTF_Init() != 0) {
        std::cerr << "[App] TTF_Init failed: " << TTF_GetError() << "\n";
        return false;
    }

    std::cout << "[App] Creating window...\n";
    win = SDL_CreateWindow(
        "Keyboard Hero",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        UiTheme::DesignW,
        UiTheme::DesignH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!win) {
        std::cerr << "[App] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    std::cout << "[App] Creating renderer...\n";
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        std::cerr << "[App] SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return false;
    }

    // Use a fixed logical resolution for all screens/overlays
    // This maintains 1536x1024 aspect ratio with black bars on any window size
    SDL_RenderSetLogicalSize(ren, UiTheme::DesignW, UiTheme::DesignH);
    SDL_RenderSetIntegerScale(ren, SDL_FALSE); // Allow non-integer scaling for smooth resize
    
    std::cout << "[App] Initializing resources...\n";
    res.init(ren);
    
    // Connect to server using config
    std::cout << "[App] Loading config...\n";
    ClientConfig cfg = ClientConfig::load();
    std::cout << "[App] Connecting to server...\n";
    if (!net.connect(cfg.server_ip, cfg.server_port)) {
        std::cerr << "[App] Warning: Failed to connect to server at " 
                  << cfg.server_ip << ":" << cfg.server_port 
                  << ". Game will run in offline mode.\n";
        // Continue anyway - some features may require connection
    } else {
        std::cout << "[App] Connected to server at " 
                  << cfg.server_ip << ":" << cfg.server_port << "\n";
    }
    
    std::cout << "[App] Initialization complete!\n";
    std::cout << "[App] Returning from init()\n";
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
    std::cout << "[App] Starting main loop...\n";
    rt.change(RouteId::Title);
    std::cout << "[App] Changed route to Title\n";

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
            
            // Handle fullscreen toggle (F11 or Alt+Enter)
            if (e.type == SDL_KEYDOWN) {
                bool toggleFullscreen = false;
                
                if (e.key.keysym.sym == SDLK_F11) {
                    toggleFullscreen = true;
                } else if (e.key.keysym.sym == SDLK_RETURN && 
                          (e.key.keysym.mod & KMOD_ALT)) {
                    toggleFullscreen = true;
                }
                
                if (toggleFullscreen) {
                    Uint32 flags = SDL_GetWindowFlags(win);
                    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        SDL_SetWindowFullscreen(win, 0);
                    } else {
                        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                    continue; // Don't pass this event to views
                }
            }
            
            viewStack.handleEvent(e);
        }
        
        // Poll network events (limit to 10 per frame to avoid blocking SDL events)
        int maxNetEvents = 10;
        while (net.has_events() && maxNetEvents-- > 0) {
            auto event = net.poll_event();
            if (event) {
                // Process network events
                // Views can access via app->network().poll_event()
                // For now, just consume them in views or handle here
                
                // Example: print event types for debugging
                switch (event->type) {
                    case NetEventType::Hello:
                        // Connection established
                        std::cout << "[App] Received Hello from server\n";
                        break;
                        
                    case NetEventType::RoomState: {
                        // Update room state
                        auto* rs = static_cast<RoomStateEvent*>(event.get());
                        st.setRoomState(*rs);
                        
                        // If we're in JoinRoomOverlay, navigate to LobbyScreen
                        View* topView = viewStack.top();
                        if (topView && dynamic_cast<JoinRoomOverlay*>(topView)) {
                            std::cout << "[App] Join successful, navigating to Lobby\n";
                            defer([this]() {
                                rt.change(RouteId::Lobby);
                            });
                        }
                        break;
                    }
                    
                    case NetEventType::GameInit: {
                        // Game starting - navigate to GameScreen
                        auto* gi = static_cast<GameInitEvent*>(event.get());
                        st.setGameInit(*gi);
                        std::cout << "[App] Game init received, paragraph: " << gi->paragraph.substr(0, 50) << "...\n";
                        
                        // Push GameScreen on top of LobbyScreen (keep lobby in stack)
                        defer([this]() {
                            rt.push(RouteId::Game);
                        });
                        break;
                    }
                    
                    case NetEventType::GameState: {
                        // Game state update
                        auto* gs = static_cast<GameStateEvent*>(event.get());
                        st.setGameState(*gs);
                        
                        // If game ended, navigate to result screen
                        if (gs->ended) {
                            std::cout << "[App] Game ended\n";
                            // TODO: Navigate to result screen
                        }
                        break;
                    }
                    
                    case NetEventType::GameEnd: {
                        // Game finished with rankings
                        auto* ge = static_cast<GameEndEvent*>(event.get());
                        std::cout << "[App] Game ended: " << ge->reason << "\n";
                        
                        // Store game end event in state
                        st.setGameEnd(*ge);
                        
                        // Push MatchResultOverlay on top of GameScreen
                        defer([this]() {
                            rt.push(RouteId::MatchResultOverlay);
                        });
                        break;
                    }
                    
                    case NetEventType::Error: {
                        auto* err = static_cast<ErrorEvent*>(event.get());
                        std::cerr << "[App] Server error: " << err->code << " - " << err->message << "\n";
                        
                        // If we're in JoinRoomOverlay, show error
                        View* topView = viewStack.top();
                        if (topView) {
                            JoinRoomOverlay* joinOverlay = dynamic_cast<JoinRoomOverlay*>(topView);
                            if (joinOverlay) {
                                joinOverlay->set_error(err->message);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
                // Store event or push to AppState for views to consume
            }
        }

        // Important: execute navigation/actions AFTER event processing to avoid use-after-free
        processDeferred();

        viewStack.update(dt);

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        viewStack.render(ren);
        SDL_RenderPresent(ren);
    }
}

void App::shutdown() {
    // Disconnect from server
    net.disconnect();
    
    // Clean up resources safely
    res.shutdown();

    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
