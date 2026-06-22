#pragma once
#include "raylib.h"
#include "gameui.h" 
#include "theme.h"
#include "appstate.h"
#include "engine.h"
#include "network.h"
#include "visualizer.h"
#include <iostream>
#include <cmath>

namespace GameHUD {

    inline void DrawAndProcess(AppState& currentState, GameSession& session, GroundND& ground, NetworkManager& net, IVisualizer* activeUI) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        int flagsPlaced = 0;
        for (size_t i = 0; i < ground.total_cells; i++) {
            if (ground.state.get(i) == 2) flagsPlaced++;
        }

        if (GameUI::DrawHeader(screenW, session.dim, session.bombs - flagsPlaced, session.bombs, session.timePlayed)) {
            net.Disconnect();
            currentState = AppState::MAIN_MENU;
        }

        if (GameUI::DrawFooter(screenW, screenH, session.nextDim, session.nextGridSize, session.nextBombs, session.nextSeed, session.randomizeSeed, net.role == NetRole::CLIENT)) {
            if (session.randomizeSeed) session.nextSeed = ((uint64_t)GetTime() * 491149) ^ 0x01252d8f21;
            session.currentSeed = session.nextSeed;

            if (std::pow(session.nextGridSize, session.nextDim) >= 100000) {
                session.alertOn = true;
            }
            else {
                session.generateQueued = true;
                if (net.role == NetRole::HOST) {
                    PacketInit p;
                    p.dim = session.nextDim; p.size = session.nextGridSize;
                    p.bombs = session.nextBombs; p.seed = session.currentSeed;
                    net.Broadcast(&p, sizeof(p));
                }
            }
        }

        if (net.role != NetRole::OFFLINE) {
            uint16_t currentPort = (uint16_t)atoi(session.hostPort);
            if (currentPort == 0) currentPort = 7777;

            const char* roleStr = (net.role == NetRole::HOST) ? TextFormat("HOSTING: Port %d", currentPort) : "CONNECTED TO SERVER";
            DrawText(roleStr, 20, 95, 20, GameUI::Green500);

            int textWidth = MeasureText(roleStr, 20);
            if (GameUI::Button("DISCONNECT", 20 + textWidth + 15, 90, 130, 30, GameUI::Zinc800, GameUI::Red500, false)) {
                net.Disconnect();
            }
        }
        else {
            if (GameUI::Button("HOST GAME", 20, 95, 150, 30, GameUI::Zinc800, GameUI::Green500, false)) {
                uint16_t port = (uint16_t)atoi(session.hostPort);
                if (port == 0) port = 7777;
                net.StartHost(port);
            }
        }

        if (session.alertOn) {
            int alertResponse = GameUI::DrawSpinnerAlert(screenW, screenH);
            if (alertResponse == 1) {
                session.generateQueued = true;
                session.alertOn = false;
                if (net.role == NetRole::HOST) {
                    PacketInit p;
                    p.dim = session.nextDim; p.size = session.nextGridSize;
                    p.bombs = session.nextBombs; p.seed = session.currentSeed;
                    net.Broadcast(&p, sizeof(p));
                }
            }
            else if (alertResponse == 2) {
                session.alertOn = false;
            }
        }

        if (activeUI->gameOver || activeUI->victory) {
            GameUI::DrawModal(screenW, screenH, activeUI->gameOver);
        }

        DrawFPS(20, screenH - 90);
    }
}