#pragma once

#include "raylib.h"
#include "gameui.h"
#include "appstate.h"
#include "network.h"

namespace MainMenu {

    inline void DrawAndProcess(AppState& currentState, bool& shouldQuit, GameSession& session, NetworkManager& net) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        const char* title1 = "DIMENSION";
        const char* title2 = "SWEEPER";

        int title1Width = MeasureText(title1, 70);
        int title2Width = MeasureText(title2, 70);

        DrawText(title1, (screenW / 2) - (title1Width / 2) + 4, screenH / 2 - 196, 70, Fade(BLACK, 0.85f));
        DrawText(title1, (screenW / 2) - (title1Width / 2), screenH / 2 - 200, 70, GameUI::Zinc200);

        DrawText(title2, (screenW / 2) - (title2Width / 2) + 4, screenH / 2 - 126, 70, Fade(BLACK, 0.85f));
        DrawText(title2, (screenW / 2) - (title2Width / 2), screenH / 2 - 130, 70, GameUI::Red500);

        int btnWidth = 240;
        int btnHeight = 45;
        int btnX = (screenW / 2) - (btnWidth / 2);

        if (currentState == AppState::MAIN_MENU) {

            if (GameUI::Button("PLAY SOLO", btnX, screenH / 2 - 20, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Green500, false)) {
                currentState = AppState::IN_GAME;
                session.connectionMsg[0] = '\0';
            }
            if (GameUI::Button("HOST GAME", btnX, screenH / 2 + 40, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Green500, false)) {
                currentState = AppState::HOST_MENU;
                session.connectionMsg[0] = '\0';
            }
            if (GameUI::Button("JOIN GAME", btnX, screenH / 2 + 100, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Green500, false)) {
                currentState = AppState::JOIN_MENU;
                session.connectionMsg[0] = '\0';
            }
            if (GameUI::Button("CUSTOMIZE", btnX, screenH / 2 + 160, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Zinc600, false)) {
                currentState = AppState::CUSTOMIZE_MENU;
                session.connectionMsg[0] = '\0';
            }
            if (GameUI::Button("QUIT", btnX, screenH / 2 + 220, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Red700, false)) {
                shouldQuit = true;
            }
        }
        else if (currentState == AppState::CUSTOMIZE_MENU) {
            const char* prompt = "ENTER DISPLAY NAME:";
            DrawText(prompt, screenW / 2 - MeasureText(prompt, 20) / 2, screenH / 2 - 20, 20, GameUI::Zinc400);

            static bool nameBoxActive = false;
            Rectangle boxRect = { (float)screenW / 2 - 150, (float)screenH / 2 + 10, 300, 45 };
            bool hovered = CheckCollisionPointRec(GetCRTMousePosition(), boxRect);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) nameBoxActive = hovered;

            if (nameBoxActive) {
                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
                    const char* clipText = GetClipboardText();
                    if (clipText != nullptr) {
#ifdef _WIN32
                        strncpy_s(session.playerName, clipText, 15);
#else
                        strncpy(session.playerName, clipText, 15);
#endif
                        session.playerName[15] = '\0';
                    }
                }

                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= 32 && key <= 125) {
                        int len = strlen(session.playerName);
                        if (len < 15) {
                            session.playerName[len] = (char)key;
                            session.playerName[len + 1] = '\0';
                        }
                    }
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                    int len = strlen(session.playerName);
                    if (len > 0) session.playerName[len - 1] = '\0';
                }
            }

            DrawRectangleRounded(boxRect, 0.2f, 4, nameBoxActive ? GameUI::Zinc800 : (hovered ? GameUI::Zinc700 : GameUI::Zinc900));
            DrawRectangleLinesEx(boxRect, 2.0f, nameBoxActive ? GameUI::Green500 : GameUI::Zinc600);

            int tW = MeasureText(session.playerName, 24);
            DrawText(session.playerName, boxRect.x + boxRect.width / 2 - tW / 2, boxRect.y + 10, 24, WHITE);

            if (nameBoxActive && (int)(GetTime() * 2) % 2 == 0) {
                DrawRectangle(boxRect.x + boxRect.width / 2 + tW / 2 + 4, boxRect.y + 10, 2, 24, WHITE);
            }

            if (GameUI::Button("SAVE & BACK", btnX, screenH / 2 + 80, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Green500, false)) {
#ifdef _WIN32
                strcpy_s(net.playerName, session.playerName);
#else
                strcpy(net.playerName, session.playerName);
#endif
                currentState = AppState::MAIN_MENU;
            }
        }
        else if (currentState == AppState::HOST_MENU) {
            const char* prompt = "ENTER HOST PORT:";
            DrawText(prompt, screenW / 2 - MeasureText(prompt, 20) / 2, screenH / 2 - 20, 20, GameUI::Zinc400);

            static bool portBoxActive = false;
            Rectangle boxRect = { (float)screenW / 2 - 100, (float)screenH / 2 + 10, 200, 45 };
            bool hovered = CheckCollisionPointRec(GetCRTMousePosition(), boxRect);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) portBoxActive = hovered;

            if (portBoxActive) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= '0' && key <= '9') {
                        int len = strlen(session.hostPort);
                        if (len < 5) {
                            session.hostPort[len] = (char)key;
                            session.hostPort[len + 1] = '\0';
                        }
                    }
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                    int len = strlen(session.hostPort);
                    if (len > 0) session.hostPort[len - 1] = '\0';
                }
            }

            DrawRectangleRounded(boxRect, 0.2f, 4, portBoxActive ? GameUI::Zinc800 : (hovered ? GameUI::Zinc700 : GameUI::Zinc900));
            DrawRectangleLinesEx(boxRect, 2.0f, portBoxActive ? GameUI::Green500 : GameUI::Zinc600);

            int tW = MeasureText(session.hostPort, 24);
            DrawText(session.hostPort, boxRect.x + boxRect.width / 2 - tW / 2, boxRect.y + 10, 24, WHITE);

            if (portBoxActive && (int)(GetTime() * 2) % 2 == 0) {
                DrawRectangle(boxRect.x + boxRect.width / 2 + tW / 2 + 4, boxRect.y + 10, 2, 24, WHITE);
            }

            if (GameUI::Button("START HOSTING", btnX, screenH / 2 + 80, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Green500, false)) {
                uint16_t port = (uint16_t)atoi(session.hostPort);
                if (port == 0) port = 7777;

#ifdef _WIN32
                strcpy_s(net.playerName, session.playerName);
#else
                strcpy(net.playerName, session.playerName);
#endif

                if (net.StartHost(port)) {
                    currentState = AppState::IN_GAME;
                    session.connectionMsg[0] = '\0';
                }
                else {
#ifdef _WIN32
                    strcpy_s(session.connectionMsg, "Failed to host. Is the port in use?");
#else
                    strcpy(session.connectionMsg, "Failed to host. Is the port in use?");
#endif
                }
            }
            if (GameUI::Button("COPY INVITE INFO", btnX, screenH / 2 + 140, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Zinc600, false)) {
                uint16_t port = (uint16_t)atoi(session.hostPort);
                if (port == 0) port = 7777;
                SetClipboardText(TextFormat("127.0.0.1:%d", port));

#ifdef _WIN32
                strcpy_s(session.connectionMsg, "Copied IP:Port to clipboard!");
#else
                strcpy(session.connectionMsg, "Copied IP:Port to clipboard!");
#endif
            }
            if (GameUI::Button("BACK", btnX, screenH / 2 + 200, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Zinc600, false)) {
                currentState = AppState::MAIN_MENU;
                session.connectionMsg[0] = '\0';
            }

            const char* hint = "(Hint: Replace 127.0.0.1 with your Public IP for internet play)";
            DrawText(hint, screenW / 2 - MeasureText(hint, 16) / 2, screenH / 2 + 255, 16, GameUI::Zinc600);
        }
        else if (currentState == AppState::JOIN_MENU) {

            const char* prompt = "ENTER HOST IP ADDRESS:";
            DrawText(prompt, screenW / 2 - MeasureText(prompt, 20) / 2, screenH / 2 - 20, 20, GameUI::Zinc400);

            static bool ipBoxActive = false;
            Rectangle boxRect = { (float)screenW / 2 - 150, (float)screenH / 2 + 10, 300, 45 };
            bool hovered = CheckCollisionPointRec(GetCRTMousePosition(), boxRect);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ipBoxActive = hovered;

            if (ipBoxActive) {
                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
                    const char* clipText = GetClipboardText();
                    if (clipText != nullptr) {
#ifdef _WIN32
                        strncpy_s(session.joinIp, clipText, 63);
#else
                        strncpy(session.joinIp, clipText, 63);
#endif
                        session.joinIp[63] = '\0';
                    }
                }

                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= 32 && key <= 125) {
                        int len = strlen(session.joinIp);
                        if (len < 63) {
                            session.joinIp[len] = (char)key;
                            session.joinIp[len + 1] = '\0';
                        }
                    }
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                    int len = strlen(session.joinIp);
                    if (len > 0) session.joinIp[len - 1] = '\0';
                }
            }

            DrawRectangleRounded(boxRect, 0.2f, 4, ipBoxActive ? GameUI::Zinc800 : (hovered ? GameUI::Zinc700 : GameUI::Zinc900));
            DrawRectangleLinesEx(boxRect, 2.0f, ipBoxActive ? GameUI::Green500 : GameUI::Zinc600);

            int tW = MeasureText(session.joinIp, 24);
            DrawText(session.joinIp, boxRect.x + boxRect.width / 2 - tW / 2, boxRect.y + 10, 24, WHITE);

            if (ipBoxActive && (int)(GetTime() * 2) % 2 == 0) {
                DrawRectangle(boxRect.x + boxRect.width / 2 + tW / 2 + 4, boxRect.y + 10, 2, 24, WHITE);
            }

            if (GameUI::Button("CONNECT", btnX, screenH / 2 + 80, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Green500, false)) {

                char ipCopy[64];
#ifdef _WIN32
                strncpy_s(ipCopy, session.joinIp, 64);
#else
                strncpy(ipCopy, session.joinIp, 64);
#endif
                ipCopy[63] = '\0';

                uint16_t port = 7777;
                char* colon = strchr(ipCopy, ':');
                if (colon != nullptr) {
                    int p = atoi(colon + 1);
                    if (p > 0) port = (uint16_t)p;
                    *colon = '\0';
                }

#ifdef _WIN32
                strcpy_s(net.playerName, session.playerName);
#else
                strcpy(net.playerName, session.playerName);
#endif

                if (net.ConnectClient(ipCopy, port)) {
                    currentState = AppState::IN_GAME;
                    session.connectionMsg[0] = '\0';
                }
                else {
#ifdef _WIN32
                    strcpy_s(session.connectionMsg, "Failed to connect. Is the host running?");
#else
                    strcpy(session.connectionMsg, "Failed to connect. Is the host running?");
#endif
                }
            }
            if (GameUI::Button("BACK", btnX, screenH / 2 + 140, btnWidth, btnHeight, GameUI::Zinc800, GameUI::Zinc600, false)) {
                currentState = AppState::MAIN_MENU;
                session.connectionMsg[0] = '\0';
            }
            }


            if (session.connectionMsg[0] != '\0') {
                int msgW = MeasureText(session.connectionMsg, 20);
                DrawText(session.connectionMsg, screenW / 2 - msgW / 2, screenH - 60, 20, GameUI::Red500);
            }
    }
}