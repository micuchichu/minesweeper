#include "raylib.h"
#include "theme.h"
#include <algorithm>

#define XRAY true

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

    inline bool Button(const char* label, float x, float y, float width, float height, Color baseColor, Color hoverColor) {
        Rectangle btnRect = { x, y, width, height };
        bool hover = CheckCollisionPointRec(GetCRTMousePosition(), btnRect);

        DrawRectangleRounded(btnRect, 0.2f, 4, hover ? hoverColor : baseColor);

        int textW = MeasureText(label, 20);
        DrawText(label, x + (width / 2.0f) - (textW / 2.0f), y + (height / 2.0f) - 10.0f, 20, WHITE);

        return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
    }

    inline float DrawSpinner(float x, float y, const char* label, int& value, int minVal, int maxVal) {
        DrawText(label, x, y + 6, 18, Zinc400);
        int labelW = MeasureText(label, 18);

        float minusX = x + labelW + 10;
        float plusX = minusX + 32;

        bool minusClicked = Button("-", minusX, y, 28, 28, Zinc700, Zinc600);
        bool plusClicked = Button("+", plusX, y, 28, 28, Zinc700, Zinc600);

        const char* valText = TextFormat("%d", value);
        int valW = MeasureText(valText, 20);
        DrawText(valText, plusX + 36, y + 5, 20, WHITE);

        if (minusClicked || plusClicked) {
            int step = IsKeyDown(KEY_LEFT_SHIFT) ? 10 : 1;
            if (minusClicked) value = std::max(minVal, value - step);
            if (plusClicked) value = std::min(maxVal, value + step);
        }

        return plusX + 36 + valW + 30;
    }

    inline void DrawHeader(int screenW, int dim, int bombsLeft, float timePlayed) {
        DrawRectangle(0, 0, screenW, 80, Zinc900);
        DrawRectangle(0, 80, screenW, 2, Zinc700);

        DrawText(TextFormat("%dD MINESWEEPER", dim), 30, 28, 24, Zinc200);

        const char* bombsText = TextFormat("BOMBS: %03d", bombsLeft);
        int bombsWidth = MeasureText(bombsText, 26);
        DrawText(bombsText, (screenW / 2) - (bombsWidth / 2), 27, 26, Red500);

        const char* timeText = TextFormat("TIME: %.3f", timePlayed);
        int timeWidth = MeasureText(timeText, 26);
        DrawText(timeText, screenW - timeWidth - 30, 27, 26, Red500);
    }

    inline bool DrawFooter(int screenW, int screenH, int& nextDim, int& nextGridSize, int& nextBombs) {
        DrawRectangle(0, screenH - 60, screenW, 60, Zinc900);
        DrawRectangle(0, screenH - 60, screenW, 2, Zinc700);

        float nextX = 20;
        nextX = DrawSpinner(nextX, screenH - 44, "DIMENSION:", nextDim, 2, 6);
        nextX += 20;
        nextX = DrawSpinner(nextX, screenH - 44, "GRID SIZE:", nextGridSize, 2, 10000);
        nextX += 20;
        nextX = DrawSpinner(nextX, screenH - 44, "BOMBS:", nextBombs, 1, 999999);
        nextX += 20;

        bool genClicked = Button("NEW GAME", nextX, screenH - 44, 115, 28, Green700, Green500);

        DrawText("(Hold SHIFT to step by 10)", nextX + 130, screenH - 37, 14, Zinc600);

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

        bool okClicked = Button("OK", (screenW / 2) - 110, centerY + 60, 100, 30, Green700, Green500);
        bool cancelClicked = Button("CANCEL", (screenW / 2) + 10, centerY + 60, 100, 30, Red700, Red500);

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