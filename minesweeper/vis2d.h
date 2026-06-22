#pragma once

#include "visualizer.h"
#include "theme.h"
#include "particles.h"
#include "engine.h"
#include "network.h"
#include "appstate.h"

#include <iostream>
#include <algorithm>
#include <cmath>

#include "raylib.h"
#include "raymath.h"

class Vis2D_Standard : public IVisualizer {
private:
    Camera2D cam;
    float cellSize = 30.0f;
    float boardWidth;
    int hoveredIndex = -1;
    NetworkManager* net;
    GameSession* session;

public:
    Vis2D_Standard(int& gridSize, int& bombs, NetworkManager* network, GameSession* s) : IVisualizer(gridSize, bombs), net(network), session(s) {}

    void OnInit() override {
        cam = { 0 };
        cam.zoom = 1.0f;
        boardWidth = ground->size * cellSize;

        Theme::InitGridAssets(cellSize);
    }

    void HandleInput() override {
        if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
            cam.target.x -= GetMouseDelta().x / cam.zoom;
            cam.target.y -= GetMouseDelta().y / cam.zoom;
        }

        if (GetMouseWheelMove() != 0) {
            float zoomFactor = 1.1f;
            if (GetMouseWheelMove() < 0) zoomFactor = 1.0f / zoomFactor;
            Vector2 mouseWorldBefore = GetScreenToWorld2D(GetMousePosition(), cam);
            cam.zoom *= zoomFactor;
            Vector2 mouseWorldAfter = GetScreenToWorld2D(GetMousePosition(), cam);
            cam.target.x += mouseWorldBefore.x - mouseWorldAfter.x;
            cam.target.y += mouseWorldBefore.y - mouseWorldAfter.y;
        }

        Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), cam);
        static Vector2 lastSentMouse = { -9999.0f, -9999.0f };

        if (Vector2Distance(worldMouse, lastSentMouse) > 2.0f && net->role != NetRole::OFFLINE) {
            lastSentMouse = worldMouse;
            PacketCursor pc;
            pc.x = worldMouse.x;
            pc.y = worldMouse.y;

#ifdef _WIN32
            strncpy_s(pc.name, net->playerName, _TRUNCATE);
#else
            strncpy(pc.name, net->playerName, 15);
            pc.name[15] = '\0';
#endif

            if (net->role == NetRole::CLIENT) {
                pc.playerID = 0;
                net->SendToServer(&pc, sizeof(pc), false);
            }
            else if (net->role == NetRole::HOST) {
                pc.playerID = 0;
                net->Broadcast(&pc, sizeof(pc), false);
            }
        }

        hoveredIndex = -1;

        if (!gameOver && !victory && GetMouseY() > 80) {
            Vector2 mouse = GetScreenToWorld2D(GetCRTMousePosition(), cam);

            if (mouse.x >= 0 && mouse.x < boardWidth && mouse.y >= 0 && mouse.y < boardWidth) {
                int x = mouse.x / cellSize;
                int y = mouse.y / cellSize;

                hoveredIndex = x + (y * ground->size);

                int state = ground->state.get(hoveredIndex);

                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    if (net->role == NetRole::CLIENT) {
#ifndef NDEBUG
                        std::cout << "[VIS2D] CLIENT: Sending Right Click (Flag) at index " << hoveredIndex << " to Host.\n";
#endif
                        PacketClick p; p.index = hoveredIndex; p.action = 2;
                        net->SendToServer(&p, sizeof(p));
                    }
                    else {
#ifndef NDEBUG
                        std::cout << "[VIS2D] HOST/OFFLINE: Local Right Click (Flag) at index " << hoveredIndex << ".\n";
#endif
                        if (state == 0) ground->state.set(hoveredIndex, 2);
                        else if (state == 2) ground->state.set(hoveredIndex, 0);

                        if (net->role == NetRole::HOST) {
#ifndef NDEBUG
                            std::cout << "[VIS2D] HOST: Broadcasting Right Click result to clients.\n";
#endif
                            PacketResult res; res.index = hoveredIndex; res.state = 2;
                            net->Broadcast(&res, sizeof(res));
                        }
                    }
                }
                else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (net->role == NetRole::CLIENT) {
#ifndef NDEBUG
                        std::cout << "[VIS2D] CLIENT: Sending Left Click (Reveal) at index " << hoveredIndex << " to Host.\n";
#endif
                        PacketClick p; p.index = hoveredIndex; p.action = 0;
                        net->SendToServer(&p, sizeof(p));
                    }
                    else {
                        if (state == 0) {
#ifndef NDEBUG
                            std::cout << "[VIS2D] HOST/OFFLINE: Local Left Click (Reveal) at index " << hoveredIndex << ".\n";
#endif
                            ground->reveal(hoveredIndex);

                            Vector2 exactWorldPos = { (x * cellSize) + (cellSize / 2.0f), (y * cellSize) + (cellSize / 2.0f) };

                            if (ground->getBomb(hoveredIndex)) {
                                gameOver = true;
                                Particles::EmitExplosion(exactWorldPos, 100, Theme::cellFlag);
                            }
                            else {
                                if (ground->revealed_cells == ground->total_cells - BOMBS) victory = true;
                                Particles::EmitDebris(exactWorldPos, 8, Theme::labelText);
                            }

                            if (net->role == NetRole::HOST) {
#ifndef NDEBUG
                                std::cout << "[VIS2D] HOST: Broadcasting Left Click result to clients.\n";
#endif
                                PacketResult res; res.index = hoveredIndex; res.state = 0;
                                net->Broadcast(&res, sizeof(res));
                            }
                        }
                    }
                }
            }
        }
    }

    void Draw() override {
        BeginMode2D(cam);

        if ((gameOver || victory) && IsKeyPressed(KEY_R)) {
            if (net->role != NetRole::CLIENT) {
#ifndef NDEBUG
                std::cout << "[VIS2D] HOST/OFFLINE: Processing Restart request.\n";
#endif
                Particles::Clear();
                ground->clear();
                if (session->randomizeSeed) session->nextSeed = ((uint64_t)GetTime() * 491149) ^ 0x01252d8f21;
                session->currentSeed = session->nextSeed;
                ground->seed = session->currentSeed;

                if (net->role == NetRole::HOST) {
#ifndef NDEBUG
                    std::cout << "[VIS2D] HOST: Broadcasting new board INIT (seed) to clients.\n";
#endif
                    PacketInit p;
                    p.dim = ground->dimensions;
                    p.size = ground->size;
                    p.bombs = BOMBS;
                    p.seed = ground->seed;
                    net->Broadcast(&p, sizeof(p));
                }

                generateBombsExact(*ground, BOMBS, ground->seed);
                ground->buildCache();
                ground->state.init(ground->total_cells);
                ground->revealed_cells = 0;
                gameOver = false;
                victory = false;
            }
#ifndef NDEBUG
            else {
                std::cout << "[VIS2D] CLIENT: Tried to press 'R', but only the Host can restart.\n";
            }
#endif
        }

        int hX = -1, hY = -1;
        if (hoveredIndex != -1) {
            hX = hoveredIndex % ground->size;
            hY = hoveredIndex / ground->size;
        }

        Vector2 screenStart = GetScreenToWorld2D({ 0, 0 }, cam);
        Vector2 screenEnd = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);

        int startX = std::max((int)(screenStart.x / cellSize), 0);
        int endX = std::min((int)(screenEnd.x / cellSize + 1), (int)ground->size);

        int startY = std::max((int)(screenStart.y / cellSize), 0);
        int endY = std::min((int)(screenEnd.y / cellSize + 1), (int)ground->size);

        float visualSize = cam.zoom * cellSize;
        bool highDetailMode = visualSize > 15.0f;

        float fade = (25.0f - visualSize) / 15.0f;
        fade = std::clamp(fade, 0.0f, 1.0f);

        Theme::DrawBaseSlice(0.0f, 0.0f, ground->size, cellSize);

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                int index = x + (y * ground->size);
                int state = ground->state.get(index);
                int count = ground->counts.get(index);
                bool isBomb = ground->getBomb(index);

                float posX = x * cellSize;
                float posY = y * cellSize;

                bool isHovered = (index == hoveredIndex);
                bool isNeighbor = (hoveredIndex != -1) && (std::abs(hX - x) <= 1 && std::abs(hY - y) <= 1);

                Theme::DrawCell(posX, posY, cellSize, state, count, isBomb, gameOver, isHovered, isNeighbor, highDetailMode, fade);
            }
        }

        Particles::UpdateAndDraw();

        for (auto const& pair : net->remoteCursors) {
            uint32_t id = pair.first;
            float cx = pair.second.x;
            float cy = pair.second.y;

            Color curCol = Theme::cellFlag;
            if (id != 0) {
                curCol = ColorFromHSV(fmod(id * 137.5f, 360.0f), 0.8f, 1.0f);
            }

            DrawCircle(cx, cy, 10, curCol);

            const char* nameTag = pair.second.name;
            if (nameTag[0] == '\0') {
                nameTag = (id == 0) ? "HOST" : TextFormat("P%d", id);
            }

            DrawText(nameTag, cx + 14, cy + 16, 12, WHITE);
        }

        EndMode2D();
    }
};