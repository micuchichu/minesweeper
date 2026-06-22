#pragma once

#include "raylib.h"
#include "theme.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace GameUI {
    const Color Zinc900 = { 24, 24, 27, 245 };
    const Color Zinc800 = { 39, 39, 42, 255 };
    const Color Zinc700 = { 63, 63, 70, 255 };
    const Color Zinc600 = { 82, 82, 91, 255 };
    const Color Zinc400 = { 161, 161, 170, 255 };
    const Color Zinc200 = { 228, 228, 231, 255 };
    const Color Red500 = { 239, 68, 68, 255 };
    const Color Red700 = { 127, 29, 29, 255 };
    const Color Green500 = { 34, 197, 94, 255 };
    const Color Green700 = { 21, 128, 61, 255 };

    inline bool Button(const char* label, float x, float y, float width, float height, Color baseColor, Color hoverColor, bool locked) {
        Rectangle btnRect = { x, y, width, height };
        bool hover = !locked && CheckCollisionPointRec(GetCRTMousePosition(), btnRect);

        Color bgColor = locked ? Zinc800 : (hover ? hoverColor : baseColor);
        Color textColor = locked ? Zinc600 : WHITE;

        DrawRectangleRounded(btnRect, 0.2f, 4, bgColor);

        int textW = MeasureText(label, 20);
        DrawText(label, x + (width / 2.0f) - (textW / 2.0f), y + (height / 2.0f) - 10.0f, 20, textColor);

        return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
    }

    inline bool Checkbox(const char* label, float x, float y, bool& checked, bool locked = false) {
        float boxSize = 20.0f;
        Rectangle boxRect = { x, y, boxSize, boxSize };
        bool hover = !locked && CheckCollisionPointRec(GetCRTMousePosition(), boxRect);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            checked = !checked;
        }

        Color bgColor = locked ? Zinc800 : (hover ? Zinc700 : BLANK);
        Color borderColor = locked ? Zinc600 : (checked ? Green500 : Zinc600);

        DrawRectangleRounded(boxRect, 0.2f, 4, bgColor);
        DrawRectangleLinesEx(boxRect, 2.0f, borderColor);

        if (checked) {
            DrawRectangleRounded({ x + 4, y + 4, boxSize - 8, boxSize - 8 }, 0.1f, 4, locked ? Zinc600 : Green500);
        }

        DrawText(label, x + boxSize + 8, y + 2, 16, locked ? Zinc600 : Zinc400);
        return hover;
    }

    inline float DrawSpinner(float x, float y, const char* label, int& value, int minVal, int maxVal, bool locked) {
        static const char* activeSpinner = nullptr;
        static char editBuffer[32] = "";

        DrawText(label, x, y + 6, 18, locked ? Zinc600 : Zinc400);
        int labelW = MeasureText(label, 18);

        float minusX = x + labelW + 10;
        float plusX = minusX + 32;

        bool minusClicked = Button("-", minusX, y, 28, 28, Zinc700, Zinc600, locked);
        bool plusClicked = Button("+", plusX, y, 28, 28, Zinc700, Zinc600, locked);

        bool isActive = (activeSpinner == label);

        if (locked && isActive) {
            activeSpinner = nullptr;
            isActive = false;
        }

        const char* displayStr = isActive ? editBuffer : TextFormat("%d", value);

        int textW = MeasureText(displayStr, 20);
        float boxWidth = std::max(40.0f, (float)textW + 16.0f);
        Rectangle textRect = { plusX + 36, y, boxWidth, 28 };

        Vector2 mPos = GetCRTMousePosition();
        bool textHovered = !locked && CheckCollisionPointRec(mPos, textRect);

        if (!locked && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (textHovered) {
                activeSpinner = label;
                snprintf(editBuffer, sizeof(editBuffer), "%d", value);
            }
            else if (isActive) {
                value = (editBuffer[0] != '\0') ? std::clamp(atoi(editBuffer), minVal, maxVal) : minVal;
                activeSpinner = nullptr;
                isActive = false;
            }
        }

        if (isActive) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= '0' && key <= '9' && strlen(editBuffer) < 15) {
                    int len = strlen(editBuffer);
                    editBuffer[len] = (char)key;
                    editBuffer[len + 1] = '\0';
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                int len = strlen(editBuffer);
                if (len > 0) editBuffer[len - 1] = '\0';
            }

            if (IsKeyPressed(KEY_ENTER)) {
                value = (editBuffer[0] != '\0') ? std::clamp(atoi(editBuffer), minVal, maxVal) : minVal;
                activeSpinner = nullptr;
                isActive = false;
            }
        }

        DrawRectangleRounded(textRect, 0.2f, 4, isActive ? Zinc800 : (textHovered ? Zinc700 : BLANK));

        if (isActive) {
            DrawRectangleLinesEx(textRect, 2.0f, Green500);
            if ((int)(GetTime() * 2) % 2 == 0) {
                int currentTextW = MeasureText(editBuffer, 20);
                DrawRectangle(textRect.x + 8 + currentTextW + 2, textRect.y + 4, 2, 20, WHITE);
            }
        }

        DrawText(displayStr, textRect.x + 8, textRect.y + 5, 20, locked ? Zinc600 : WHITE);

        if (minusClicked || plusClicked) {
            if (isActive) {
                value = (editBuffer[0] != '\0') ? std::clamp(atoi(editBuffer), minVal, maxVal) : minVal;
                activeSpinner = nullptr;
            }
            int step = IsKeyDown(KEY_LEFT_SHIFT) ? 10 : 1;
            if (minusClicked) value = std::max(minVal, value - step);
            if (plusClicked) value = std::min(maxVal, value + step);
        }

        return textRect.x + textRect.width + 10;
    }

    inline float DrawSeedBox(float x, float y, const char* label, uint64_t& seed, bool locked) {
        static const char* activeSeedBox = nullptr;
        static char seedBuffer[64] = "";

        DrawText(label, x, y + 6, 18, locked ? Zinc600 : Zinc400);
        int labelW = MeasureText(label, 18);
        float textX = x + labelW + 10;

        bool isActive = (activeSeedBox == label);
        if (locked && isActive) {
            activeSeedBox = nullptr;
            isActive = false;
        }

        const char* displayStr = isActive ? seedBuffer : TextFormat("%llu", seed);

        int textW = MeasureText(displayStr, 20);
        float boxWidth = std::max(100.0f, (float)textW + 16.0f);
        Rectangle textRect = { textX, y, boxWidth, 28 };

        Vector2 mPos = GetCRTMousePosition();
        bool textHovered = !locked && CheckCollisionPointRec(mPos, textRect);

        if (!locked && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (textHovered) {
                activeSeedBox = label;
                snprintf(seedBuffer, sizeof(seedBuffer), "%llu", seed);
            }
            else if (isActive) {
                seed = (seedBuffer[0] != '\0') ? strtoull(seedBuffer, nullptr, 10) : 0;
                activeSeedBox = nullptr;
                isActive = false;
            }
        }

        if (isActive) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= '0' && key <= '9' && strlen(seedBuffer) < 20) {
                    int len = strlen(seedBuffer);
                    seedBuffer[len] = (char)key;
                    seedBuffer[len + 1] = '\0';
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                int len = strlen(seedBuffer);
                if (len > 0) seedBuffer[len - 1] = '\0';
            }

            if (IsKeyPressed(KEY_ENTER)) {
                seed = (seedBuffer[0] != '\0') ? strtoull(seedBuffer, nullptr, 10) : 0;
                activeSeedBox = nullptr;
                isActive = false;
            }
        }

        DrawRectangleRounded(textRect, 0.2f, 4, isActive ? Zinc800 : (textHovered ? Zinc700 : BLANK));

        if (isActive) {
            DrawRectangleLinesEx(textRect, 2.0f, Green500);
            if ((int)(GetTime() * 2) % 2 == 0) {
                int currentTextW = MeasureText(seedBuffer, 20);
                DrawRectangle(textRect.x + 8 + currentTextW + 2, textRect.y + 4, 2, 20, WHITE);
            }
        }

        DrawText(displayStr, textRect.x + 8, textRect.y + 5, 20, locked ? Zinc600 : WHITE);

        return textRect.x + textRect.width + 10;
    }

    inline bool DrawHeader(int screenW, int dim, int flagsPlaced, int totalBombs, float timePlayed) {
        DrawRectangle(0, 0, screenW, 80, Zinc900);
        DrawRectangle(0, 80, screenW, 2, Zinc700);

        bool exitClicked = Button("MENU", 20, 25, 80, 30, Zinc800, Zinc600, false);

        DrawText(TextFormat("%dD MINESWEEPER", dim), 120, 28, 24, Zinc200);

        int bombsLeft = totalBombs - flagsPlaced;
        const char* bombsText = TextFormat("%d / %d", bombsLeft, totalBombs);
        int bombsWidth = MeasureText(bombsText, 26);

        int totalCenterWidth = 32 + 10 + bombsWidth;
        float startX = (screenW / 2.0f) - (totalCenterWidth / 2.0f);

        float frame = (int)(GetTime() * 4) % 3;
        Rectangle flagSource = { frame * 16.0f, 0.0f, 16.0f, 16.0f };
        Rectangle flagDest = { startX + bombsWidth + 10, 24.0f, 32.0f, 32.0f };
        DrawTexturePro(Theme::flagSprite, flagSource, flagDest, { 0, 0 }, sin(GetTime()) * 5.0f, WHITE);

        DrawText(bombsText, startX, 27, 26, Red500);

        const char* timeText = TextFormat("TIME: %.3f", timePlayed);
        int timeWidth = MeasureText(timeText, 26);
        DrawText(timeText, screenW - timeWidth - 30, 27, 26, Red500);

        return exitClicked;
    }

    inline bool DrawFooter(int screenW, int screenH, int& nextDim, int& nextGridSize, int& nextBombs, uint64_t& nextSeed, bool& randomizeSeed, bool locked = false) {
        DrawRectangle(0, screenH - 60, screenW, 60, Zinc900);
        DrawRectangle(0, screenH - 60, screenW, 2, Zinc700);

        float nextX = 20;

        nextX = DrawSpinner(nextX, screenH - 44, "DIM:", nextDim, 2, 6, locked);
        nextX += 15;
        nextX = DrawSpinner(nextX, screenH - 44, "SIZE:", nextGridSize, 2, 10000, locked);
        nextX += 15;
        nextX = DrawSpinner(nextX, screenH - 44, "BOMBS:", nextBombs, 1, 999999, locked);
        nextX += 15;

        bool genClicked = Button("NEW GAME", nextX, screenH - 44, 115, 28, Green700, Green500, locked);

        int textW = MeasureText(TextFormat("%llu", nextSeed), 20);
        float boxWidth = std::max(100.0f, (float)textW + 16.0f);
        float labelW = MeasureText("SEED:", 18);
        float seedStartX = screenW - labelW - boxWidth - 30;

        float cbWidth = 20.0f + 8.0f + MeasureText("RANDOM", 16);
        float cbX = seedStartX - cbWidth - 20.0f;
        Checkbox("RANDOM", cbX, screenH - 40, randomizeSeed, locked);

        DrawSeedBox(seedStartX, screenH - 44, "SEED:", nextSeed, randomizeSeed || locked);

        if (nextX + 130 + MeasureText("(Hold SHIFT to step by 10)", 14) < cbX - 20) {
            DrawText("(Hold SHIFT to step by 10)", nextX + 130, screenH - 37, 14, locked ? Zinc700 : Zinc600);
        }

        return genClicked;
    }

    inline uint8_t DrawSpinnerAlert(int screenW, int screenH) {
        int centerY = (screenH / 2) - 30;
        const char* alertText = "Larger grids may cause lag and high resources usage!";
        int textW = MeasureText(alertText, 30);
        int textX = (screenW / 2) - (textW / 2);

        DrawRectangle(0, 0, screenW, screenH, Fade(Zinc900, 0.5f));

        DrawText(alertText, textX + 4, centerY + 4, 30, Fade(BLACK, 0.85f));
        DrawText(alertText, textX, centerY, 30, Red500);

        bool okClicked = Button("OK", (screenW / 2) - 110, centerY + 60, 100, 30, Green700, Green500, false);
        bool cancelClicked = Button("CANCEL", (screenW / 2) + 10, centerY + 60, 100, 30, Red700, Red500, false);

        if (okClicked) return 1;
        if (cancelClicked) return 2;

        return 0;
    }

    inline void DrawModal(int screenW, int screenH, bool isGameOver) {
        int centerY = (screenH / 2) - 30;

        const char* mainText = isGameOver ? "kaboom" : "SECTOR CLEARED!";
        Color mainColor = isGameOver ? Red500 : Green500;

        int textW = MeasureText(mainText, 50);
        int textX = (screenW / 2) - (textW / 2);

        DrawText(mainText, textX + 4, centerY + 4, 50, Fade(BLACK, 0.85f));
        DrawText(mainText, textX, centerY, 50, mainColor);

        const char* subText = "Press 'R' to restart";
        int subW = MeasureText(subText, 26);
        int subX = (screenW / 2) - (subW / 2);
        int subY = centerY + 60;

        DrawText(subText, subX + 3, subY + 3, 26, Fade(BLACK, 0.85f));
        DrawText(subText, subX, subY, 26, Zinc200);
    }
}