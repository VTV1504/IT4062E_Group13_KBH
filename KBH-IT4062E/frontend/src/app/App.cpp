#include "App.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>

#include "../ui/UiTheme.h"
#include "../config/ClientConfig.h"
#include "../overlays/JoinRoomOverlay.h"
#include "../overlays/SignInOverlay.h"
#include "../overlays/CreateAccountOverlay.h"
#include "../overlays/ChangePasswordOverlay.h"
#include "../overlays/GuestResultOverlay.h"
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
        
        // Exit immediately if quit was requested
        if (quitRequested) {
            break;
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
                    
                    case NetEventType::SignInResponse: {
                        auto* resp = static_cast<SignInResponseEvent*>(event.get());
                        if (resp->success) {
                            std::cout << "[App] Sign in successful: user_id=" << resp->user_id 
                                     << ", username=" << resp->username << "\n";
                            st.setUser(resp->user_id, resp->username);
                            
                            // Check if we're signing in from GuestResultOverlay (training result pending)
                            View* topView = viewStack.top();
                            SignInOverlay* signInOverlay = dynamic_cast<SignInOverlay*>(topView);
                            if (signInOverlay) {
                                // Pop SignInOverlay
                                defer([this, username = resp->username]() {
                                    rt.pop();
                                    
                                    // Check if there's a GuestResultOverlay underneath
                                    View* underView = viewStack.top();
                                    if (dynamic_cast<GuestResultOverlay*>(underView)) {
                                        // Replace GuestResultOverlay with UserResultOverlay
                                        std::cout << "[App] Replacing GuestResultOverlay with UserResultOverlay\n";
                                        
                                        // Update display_name in GameEnd rankings to show correct username
                                        if (st.hasGameEnd()) {
                                            auto ge = st.getGameEnd();
                                            if (!ge.rankings.empty()) {
                                                ge.rankings[0].display_name = username;
                                                st.setGameEnd(ge);
                                                std::cout << "[App] Updated display_name to: " << username << "\n";
                                                
                                                // Save training result to backend
                                                const auto& ranking = ge.rankings[0];
                                                // Get paragraph from GameInit
                                                if (st.hasGameInit()) {
                                                    const auto& gi = st.getGameInit();
                                                    std::cout << "[App] Sending save_training_result to backend\n";
                                                    net.send_save_training_result(
                                                        gi.paragraph,
                                                        ranking.wpm,
                                                        ranking.accuracy,
                                                        (int)(ranking.latest_time_ms - gi.server_start_ms),
                                                        ranking.word_idx
                                                    );
                                                }
                                            }
                                        }
                                        
                                        rt.replaceTop(RouteId::UserResultOverlay);
                                    }
                                });
                            } else {
                                // Normal sign in, just pop
                                defer([this]() {
                                    rt.pop();
                                });
                            }
                        } else {
                            std::cerr << "[App] Sign in failed: " << resp->error_message << "\n";
                            // Show error in SignInOverlay
                            View* topView = viewStack.top();
                            if (topView) {
                                SignInOverlay* signInOverlay = dynamic_cast<SignInOverlay*>(topView);
                                if (signInOverlay) {
                                    signInOverlay->setError(resp->error_message);
                                }
                            }
                        }
                        break;
                    }
                    
                    case NetEventType::CreateAccountResponse: {
                        auto* resp = static_cast<CreateAccountResponseEvent*>(event.get());
                        if (resp->success) {
                            std::cout << "[App] Account created: " << resp->username 
                                     << " - Please sign in\n";
                            // Don't auto-login, user must sign in
                            
                            // Pop the CreateAccountOverlay
                            defer([this]() {
                                rt.pop();
                            });
                        } else {
                            std::cerr << "[App] Create account failed: " << resp->error_message << "\n";
                            // Show error in CreateAccountOverlay
                            View* topView = viewStack.top();
                            if (topView) {
                                CreateAccountOverlay* createAccountOverlay = dynamic_cast<CreateAccountOverlay*>(topView);
                                if (createAccountOverlay) {
                                    createAccountOverlay->setError(resp->error_message);
                                }
                            }
                        }
                        break;
                    }
                    
                    case NetEventType::ChangePasswordResponse: {
                        auto* resp = static_cast<ChangePasswordResponseEvent*>(event.get());
                        if (resp->success) {
                            std::cout << "[App] Password changed successfully\n";
                            
                            // Pop the ChangePasswordOverlay
                            defer([this]() {
                                rt.pop();
                            });
                        } else {
                            std::cerr << "[App] Change password failed: " << resp->error_message << "\n";
                            
                            // Show error in ChangePasswordOverlay
                            View* topView = viewStack.top();
                            if (auto* overlay = dynamic_cast<ChangePasswordOverlay*>(topView)) {
                                overlay->setError(resp->error_message);
                            }
                        }
                        break;
                    }
                        
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
                        
                        // Check if training mode or arena mode
                        if (gi->room_id == "training") {
                            // Training: Navigate directly to GameScreen from TitleScreen
                            std::cout << "[App] Training mode - navigating to Game\n";
                            defer([this]() {
                                rt.change(RouteId::Game);
                            });
                        } else {
                            // Arena: Push GameScreen on top of LobbyScreen (keep lobby in stack)
                            defer([this]() {
                                rt.push(RouteId::Game);
                            });
                        }
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
                        
                        // Debug: print rankings data
                        std::cout << "[App] Rankings count: " << ge->rankings.size() << "\n";
                        for (size_t i = 0; i < ge->rankings.size(); i++) {
                            const auto& r = ge->rankings[i];
                            std::cout << "[App] Ranking[" << i << "]: " << r.display_name 
                                      << " WPM=" << r.wpm << " ACC=" << (r.accuracy * 100) << "%"
                                      << " word_idx=" << r.word_idx << "\n";
                        }
                        
                        // Store game end event in state
                        st.setGameEnd(*ge);
                        
                        // Check if this is training mode
                        if (ge->room_id == "training") {
                            // Training mode: check if user is logged in
                            std::cout << "[App] Training mode ended, showing result\n";
                            
                            if (st.isUserAuthenticated()) {
                                // Logged in: show UserResultOverlay
                                std::cout << "[App] User logged in, showing UserResultOverlay\n";
                                // TODO: Send save_training_result to backend
                                defer([this]() {
                                    rt.push(RouteId::UserResultOverlay);
                                });
                            } else {
                                // Not logged in: show GuestResultOverlay
                                std::cout << "[App] Guest user, showing GuestResultOverlay\n";
                                defer([this]() {
                                    rt.push(RouteId::GuestResultOverlay);
                                });
                            }
                        } else {
                            // Arena mode: push MatchResultOverlay on top of GameScreen
                            defer([this]() {
                                rt.push(RouteId::MatchResultOverlay);
                            });
                        }
                        break;
                    }
                    
                    case NetEventType::LeaderboardResponse: {
                        auto* lb = static_cast<LeaderboardResponseEvent*>(event.get());
                        std::cout << "[App] Received leaderboard with " << lb->top8.size() << " entries\n";
                        
                        // Store in session state
                        st.setLeaderboard(*lb);
                        
                        // Push overlay
                        defer([this]() {
                            rt.push(RouteId::LeaderboardOverlay);
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
    std::cout << "[App] shutdown() - Disconnecting from server...\n";
    // Disconnect from server
    net.disconnect();
    
    std::cout << "[App] shutdown() - Cleaning up resources...\n";
    // Clean up resources safely
    res.shutdown();

    std::cout << "[App] shutdown() - Destroying renderer and window...\n";
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);

    std::cout << "[App] shutdown() - Quitting SDL subsystems...\n";
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    
    std::cout << "[App] shutdown() - Complete\n";
}
