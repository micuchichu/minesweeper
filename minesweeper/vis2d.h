#pragma once

#include "visualizer.h"
#include "theme.h"
#include "particles.h"
#include "engine.h"

#include <algorithm>
#include <cmath>

#include "raylib.h"
#include "raymath.h"

#include <iostream>

class Vis2D_Standard : public IVisualizer {
private:
    Camera2D cam;
    float cellSize = 30.0f;
    float boardWidth;
    int hoveredIndex = -1;

public:
    Vis2D_Standard(int& gridSize, int& bombs) : IVisualizer(gridSize, bombs) {}

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
            Vector2 mouseWorldBefore = GetScreenToWorld2D(GetCRTMousePosition(), cam);
            cam.zoom *= zoomFactor;
            Vector2 mouseWorldAfter = GetScreenToWorld2D(GetCRTMousePosition(), cam);
            cam.target.x += mouseWorldBefore.x - mouseWorldAfter.x;
            cam.target.y += mouseWorldBefore.y - mouseWorldAfter.y;
        }

        hoveredIndex = -1;

        if (!gameOver && !victory && GetCRTMousePosition().y > 80) {
            Vector2 mouse = GetScreenToWorld2D(GetCRTMousePosition(), cam);

            if (mouse.x >= 0 && mouse.x < boardWidth && mouse.y >= 0 && mouse.y < boardWidth) {
                int x = mouse.x / cellSize;
                int y = mouse.y / cellSize;

                hoveredIndex = x + (y * ground->size);

                int state = ground->state.get(hoveredIndex);

                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    if (state == 0) ground->state.set(hoveredIndex, 2);
                    else if (state == 2) ground->state.set(hoveredIndex, 0);
                }
                else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (state == 0) {
                        ground->reveal(hoveredIndex);
                        std::cout << ground->revealed_cells << "/" << ground->total_cells - BOMBS << std::endl;

                        Vector2 exactWorldPos = { (x * cellSize) + (cellSize / 2.0f), (y * cellSize) + (cellSize / 2.0f) };

                        if (ground->getBomb(hoveredIndex)) {
                            gameOver = true;
                            Particles::EmitExplosion(exactWorldPos, 100, Theme::cellFlag);
                        }
                        else {
                            if (ground->revealed_cells == ground->total_cells - BOMBS) victory = true;
                            Particles::EmitDebris(exactWorldPos, 8, Theme::labelText);
                        }
                    }
                }
            }
        }
    }

    void Draw() override {
        BeginMode2D(cam);

        if ((gameOver || victory) && IsKeyPressed(KEY_R)) {
            Particles::Clear();
            ground->clear();
            generateBombsExact(*ground, BOMBS);
            ground->buildCache();
            ground->state.init(ground->total_cells);
            ground->revealed_cells = 0;
            gameOver = false;
            victory = false;
        }

        int hX = -1, hY = -1;
        if (hoveredIndex != -1) {
            hX = hoveredIndex % ground->size;
            hY = hoveredIndex / ground->size;
        }

        Vector2 screenStart = GetScreenToWorld2D({ 0, 0 }, cam);
        Vector2 screenEnd = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);

        int startX = std::max((int)(screenStart.x / cellSize), 0);
        int endX = std::min((uint64_t)(screenEnd.x / cellSize + 1), ground->size);

        int startY = std::max((int)(screenStart.y / cellSize), 0);
        int endY = std::min((uint64_t)(screenEnd.y / cellSize + 1), ground->size);

        float visualSize = cam.zoom * cellSize;
        bool highDetailMode = visualSize > 15.0f;

        float fade = (25.0f - visualSize) / 15.0f;
        fade = std::clamp(fade, 0.0f, 1.0f);

        Theme::DrawBaseSlice(0, 0, ground->size, cellSize);

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

        EndMode2D();
    }
};