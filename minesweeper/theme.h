#pragma once

#include "raylib.h"
#include "raymath.h"

#include <iostream>

inline Vector2 GetCRTMousePosition() {
    Vector2 mouse = GetMousePosition();
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    Vector2 centered = { (mouse.x / screenW) - 0.5f, (mouse.y / screenH) - 0.5f };
    float r2 = (centered.x * centered.x) + (centered.y * centered.y);

    return {
        (mouse.x / screenW + centered.x * (r2 * 0.2f)) * screenW,
        (mouse.y / screenH + centered.y * (r2 * 0.2f)) * screenH
    };
}

inline Color GetNeighborColor(int count) {
    if (count == 0) return BLANK;
    if (count == 1) return BLUE;
    if (count == 2) return GREEN;

    float t = (float)(count - 1) / 79.0f;

    if (t > 1.0f) t = 1.0f;
    float hue = 360.0f * (1.0f - t);
    float saturation = 1.0f - (t * 0.4f);

    return ColorFromHSV(hue, saturation, 1.0f);
}

namespace Theme {
    const Color bgSlice = Color{ 39, 39, 42, 255 };
    const Color cellHidden = Color{ 82, 82, 91, 255 };
    const Color cellReveal = Color{ 24, 24, 27, 255 };
    const Color cellFlag = Color{ 239, 68, 68, 255 };
    const Color textFlag = Color{ 127, 29, 29, 255 };
    const Color labelText = Color{ 161, 161, 170, 255 };

    const float cellMargin = 2.0f;
    const float cornerRoundness = 0.2f;

    static Texture2D flagSprite = { 0 };

    static RenderTexture2D cellRevealRT = { 0 };
    static RenderTexture2D cellHiddenRT = { 0 };
    static Shader gridShader = { 0 };

    static int gridShaderGridSizeLoc = -1;
    static int gridShaderCellSizeLoc = -1;

    inline float hash2D(int32_t x, int32_t y) {
        uint32_t seed = 0x5D1B4A5D;
        uint32_t mx = 0x85EBCA6B;
        uint32_t my = 0xC2B2AE35;

        uint32_t hash = x * mx;
        hash = (hash << 13) ^ hash;

        hash ^= y * my;
        hash = (hash >> 15) ^ hash;

        hash *= seed;
        hash ^= hash >> 16;

        return static_cast<float>(hash) / static_cast<float>(0xFFFFFFFF);
    }

    inline void CreateCellRT(RenderTexture2D& rt, Color color, float size) {
        if (rt.id != 0) UnloadRenderTexture(rt);

        int texSize = (int)ceilf(size);
        if (texSize < 1) texSize = 1;

        rt = LoadRenderTexture(texSize, texSize);
        BeginTextureMode(rt);
        ClearBackground(BLANK);
        DrawRectangleRounded({ 0, 0, (float)texSize, (float)texSize }, cornerRoundness, 4, color);
        EndTextureMode();
    }

    inline void InitGlobalAssets() {
        if (flagSprite.id == 0) flagSprite = LoadTexture("assets/flag.png");

        if (gridShader.id == 0) {
            gridShader = LoadShader(0, "./assets/grid.glsl");

            gridShaderGridSizeLoc = GetShaderLocation(gridShader, "gridSize");
            gridShaderCellSizeLoc = GetShaderLocation(gridShader, "cellSize");
            int marginLoc = GetShaderLocation(gridShader, "margin");
            int bgColorLoc = GetShaderLocation(gridShader, "bgColor");

            SetShaderValue(gridShader, marginLoc, &cellMargin, SHADER_UNIFORM_FLOAT);
            Vector4 bgColorNorm = ColorNormalize(bgSlice);
            SetShaderValue(gridShader, bgColorLoc, &bgColorNorm, SHADER_UNIFORM_VEC4);
        }
    }

    inline void InitGridAssets(float cellSize) {
        float innerCellSize = cellSize - (cellMargin * 2);
        CreateCellRT(cellRevealRT, cellReveal, innerCellSize);
        CreateCellRT(cellHiddenRT, cellHidden, innerCellSize);

        if (gridShader.id != 0) {
            SetShaderValue(gridShader, gridShaderCellSizeLoc, &cellSize, SHADER_UNIFORM_FLOAT);
        }
    }

    inline void UnloadAssets() {
        if (flagSprite.id != 0) UnloadTexture(flagSprite);
        if (cellRevealRT.id != 0) UnloadRenderTexture(cellRevealRT);
        if (cellHiddenRT.id != 0) UnloadRenderTexture(cellHiddenRT);
        if (gridShader.id != 0) UnloadShader(gridShader);

        flagSprite = { 0 };
        cellRevealRT = { 0 };
        cellHiddenRT = { 0 };
        gridShader = { 0 };
    }

    inline void DrawBaseSlice(float x, float y, int gridSize, float cellSize) {
        float boardWidth = gridSize * cellSize;

        if (gridShader.id != 0 && cellRevealRT.id != 0) {
            Vector2 gridSizeVec = { (float)gridSize, (float)gridSize };
            SetShaderValue(gridShader, gridShaderGridSizeLoc, &gridSizeVec, SHADER_UNIFORM_VEC2);

            BeginShaderMode(gridShader);

            Rectangle source = { 0.0f, 0.0f, (float)cellRevealRT.texture.width, -(float)cellRevealRT.texture.height };
            Rectangle dest = { x, y, boardWidth, boardWidth };
            Vector2 origin = { 0.0f, 0.0f };
            DrawTexturePro(cellRevealRT.texture, source, dest, origin, 0.0f, WHITE);

            EndShaderMode();
        }
    }

    inline void DrawCell(float x, float y, float cellSize, int state, int count, bool isBomb, bool isGameOver, bool isHovered, bool isNeighbor, bool highDetailMode, float fade)
    {
        if (state == 1 && count == 0 && !isNeighbor) {
            return;
        }

        if (highDetailMode) {
            Rectangle cellRect = {
                x + cellMargin, y + cellMargin,
                cellSize - (cellMargin * 2),
                cellSize - (cellMargin * 2)
            };

            if (isGameOver && isBomb) {
                DrawRectangleRounded(cellRect, cornerRoundness, 4, cellFlag);
                int textWidth = MeasureText("*", 24);
                DrawText("*", cellRect.x + (cellRect.width / 2) - (textWidth / 2), cellRect.y + 2, 24, textFlag);
            }
            else if (state == 1) {
                if (count > 0) {
                    Color tc = GetNeighborColor(count);
                    const char* numStr = TextFormat("%d", count);
                    int textWidth = MeasureText(numStr, 18);
                    int textX = cellRect.x + (cellRect.width / 2) - (textWidth / 2);
                    int textY = cellRect.y + (cellRect.height / 2) - 9;

                    DrawText(numStr, textX, textY, 18, tc);
                }
            }
            else if (state == 2) {
                if (cellHiddenRT.id != 0) {
                    Rectangle source = { 0.0f, 0.0f, (float)cellHiddenRT.texture.width, -(float)cellHiddenRT.texture.height };
                    DrawTexturePro(cellHiddenRT.texture, source, cellRect, { 0,0 }, 0.0f, WHITE);
                }
                else {
                    DrawRectangleRounded(cellRect, cornerRoundness, 4, cellHidden);
                }

                Rectangle destRect = { cellRect.x + 4 * cellRect.width / 16, cellRect.y + 12 * cellRect.height / 16, cellRect.width, cellRect.height };
                float time = GetTime() + (hash2D((int32_t)x, (int32_t)y) * 10.0f);
                float frame = (int)time % 3;
                Rectangle flagSource = { frame * 16.0f, 0.0f, 16.0f, 16.0f };
                DrawTexturePro(flagSprite, flagSource, destRect, { 4 * cellRect.width / 16, 12 * cellRect.height / 16 }, sin(time) * 5, WHITE);
            }
            else {
                if (cellHiddenRT.id != 0) {
                    Rectangle source = { 0.0f, 0.0f, (float)cellHiddenRT.texture.width, -(float)cellHiddenRT.texture.height };
                    DrawTexturePro(cellHiddenRT.texture, source, cellRect, { 0,0 }, 0.0f, WHITE);
                }
                else {
                    DrawRectangleRounded(cellRect, cornerRoundness, 4, cellHidden);
                }
            }

            if (isNeighbor) {
                if (isHovered) {
                    DrawRectangleLinesEx(cellRect, 2.0f, WHITE);
                    DrawRectangleRounded(cellRect, cornerRoundness, 4, Fade(WHITE, 0.25f));
                }
                else {
                    DrawRectangleRounded(cellRect, cornerRoundness, 4, Fade(WHITE, 0.08f));
                }
            }
            if (state == 1 && count > 0) {
                Color overlay = GetNeighborColor(count);
                float baseAlpha = 20 + count;
                overlay.a = (unsigned char)(baseAlpha + fade * (255.0f - baseAlpha));
                DrawRectangleRounded(cellRect, cornerRoundness, 4, overlay);
            }
        }
        else {
            Color lodColor = cellHidden;
            if (isGameOver && isBomb) lodColor = cellFlag;
            else if (state == 1) {
                if (count > 0) {
                    Color target = GetNeighborColor(count);
                    lodColor = Color{
                        (unsigned char)(cellReveal.r + fade * (target.r - cellReveal.r)),
                        (unsigned char)(cellReveal.g + fade * (target.g - cellReveal.g)),
                        (unsigned char)(cellReveal.b + fade * (target.b - cellReveal.b)),
                        (unsigned char)(cellReveal.a + fade * (target.a - cellReveal.a))
                    };
                }
                else {
                    return;
                }
            }
            else if (state == 2) lodColor = cellFlag;

            DrawRectangle(x + 1, y + 1, cellSize - 2, cellSize - 2, lodColor);
        }
    }
}