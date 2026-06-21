#pragma once

#include "visualizer.h"
#include "theme.h"
#include "particles.h"

#include <algorithm>
#include <cmath>

#include "raylib.h"
#include "raymath.h"

class Vis4D_Slices : public IVisualizer {
private:
    Camera2D cam;
    float cellSize = 30.0f;
    float slicePadding = 45.0f;
    float boardWidth;
    int hoveredIndex = -1;

public:
    Vis4D_Slices(int& gridSize, int& bombs) : IVisualizer(gridSize, bombs) {}

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

            if (mouse.x >= 0 && mouse.y >= 0) {
                float sliceStride = boardWidth + slicePadding;
                int z = mouse.x / sliceStride;
                int w = mouse.y / sliceStride;

                if (z >= 0 && z < ground->size && w >= 0 && w < ground->size) {
                    Vector2 localMouse = mouse - Vector2{ z * sliceStride, w * sliceStride };

                    if (localMouse.x < boardWidth && localMouse.y < boardWidth) {
                        int x = localMouse.x / cellSize;
                        int y = localMouse.y / cellSize;

                        Vector2 exactPos = { x * cellSize, y * cellSize };

                        if (localMouse.x - exactPos.x < cellSize && localMouse.y - exactPos.y < cellSize) {

                            hoveredIndex = x + (y * ground->size) + (z * ground->size * ground->size) + (w * ground->size * ground->size * ground->size);

                            int state = ground->state.get(hoveredIndex);

                            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                                if (state == 0) ground->state.set(hoveredIndex, 2);
                                else if (state == 2) ground->state.set(hoveredIndex, 0);
                            }
                            else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                if (state == 0) {
                                    ground->reveal(hoveredIndex);
                                    ground->revealed_cells++;

									Vector2 exactWorldPos = { (x * cellSize) + (cellSize / 2.0f) + (z * sliceStride), (y * cellSize) + (cellSize / 2.0f) + (w * sliceStride) };

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

        int hX = -1, hY = -1, hZ = -1, hW = -1;
        if (hoveredIndex != -1) {
            hX = hoveredIndex % ground->size;
            hY = (hoveredIndex / ground->size) % ground->size;
            hZ = (hoveredIndex / (ground->size * ground->size)) % ground->size;
            hW = (hoveredIndex / (ground->size * ground->size * ground->size));
        }

        Vector2 screenStart = GetScreenToWorld2D({ 0, 0 }, cam);
        Vector2 screenEnd = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);

        float sliceStride = boardWidth + slicePadding;

        int startZ = std::max((int)(screenStart.x / sliceStride), 0);
        int endZ = std::min((uint64_t)(screenEnd.x / sliceStride + 1), ground->size);

        int startW = std::max((int)(screenStart.y / sliceStride), 0);
        int endW = std::min((uint64_t)(screenEnd.y / sliceStride + 1), ground->size);

        float visualSize = cam.zoom * cellSize;
        bool highDetailMode = visualSize > 15.0f;

        float fade = (25.0f - visualSize) / 15.0f;
        fade = std::clamp(fade, 0.0f, 0.7f);

        for (int w = startW; w < endW; w++) {
            for (int z = startZ; z < endZ; z++) {

                float sliceX = z * sliceStride;
                float sliceY = w * sliceStride;

                int startX = std::max((int)((screenStart.x - sliceX) / cellSize), 0);
                int endX = std::min((uint64_t)((screenEnd.x - sliceX) / cellSize + 1), ground->size);

                int startY = std::max((int)((screenStart.y - sliceY) / cellSize), 0);
                int endY = std::min((uint64_t)((screenEnd.y - sliceY) / cellSize + 1), ground->size);

                if (startX >= ground->size || endX <= 0 || startY >= ground->size || endY <= 0) continue;

                Theme::DrawBaseSlice(sliceX, sliceY, ground->size, cellSize);

                if (cam.zoom > 0.4f) {
                    DrawText(TextFormat("Z:%d  W:%d", z, w), sliceX + 5, sliceY - 30, 20, Theme::labelText);
                }

                for (int y = startY; y < endY; y++) {
                    for (int x = startX; x < endX; x++) {
                        int index = x + (y * ground->size) + (z * ground->size * ground->size) + (w * ground->size * ground->size * ground->size);
                        int state = ground->state.get(index);

                        float posX = sliceX + x * cellSize;
                        float posY = sliceY + y * cellSize;

                        int count = ground->counts.get(index);
                        bool isBomb = ground->getBomb(index);
                        bool isHovered = (index == hoveredIndex);
                        bool isNeighbor = (hoveredIndex != -1) &&
                            (std::abs(hX - x) <= 1 && std::abs(hY - y) <= 1 &&
                                std::abs(hZ - z) <= 1 && std::abs(hW - w) <= 1);

                        Theme::DrawCell(posX, posY, cellSize, state, count, isBomb, gameOver, isHovered, isNeighbor, highDetailMode, fade);
                    }
                }
            }
        }

        Particles::UpdateAndDraw();

        EndMode2D();
    }
};