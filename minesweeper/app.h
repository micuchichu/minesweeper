#pragma once

#include "raylib.h"
#include "theme.h"
#include "engine.h"
#include "vis4d.h"
#include "vis3d.h"
#include "vis2d.h"
#include "network.h"

#include "appstate.h"
#include "menu.h"
#include "gamehud.h"

#include <iostream>
#include <cmath>

#define XRAY true

class Minesweeper {
private:
    AppState currentState = AppState::MAIN_MENU;
    bool shouldQuit = false;

    GameSession session;

    GroundND ground;
    NetworkManager net;
    IVisualizer* activeUI = nullptr;

    Shader postProcess;
    int timeLoc;
    int resLoc;
    RenderTexture2D target;

public:
    void Run() {
        Initialize();

        while (!WindowShouldClose() && !shouldQuit) {
            HandleNetwork();
            Update();
            Draw();
        }

        session.Save();

        Cleanup();
    }

private:
    void Initialize() {
        ground.seed = session.currentSeed;
        ground.init(session.dim, session.gridSize);
        generateBombsExact(ground, session.bombs, session.currentSeed);
        ground.buildCache();

        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(1000, 900, "minsper");

        postProcess = LoadShader(nullptr, "./assets/shader.glsl");
        timeLoc = GetShaderLocation(postProcess, "time");
        resLoc = GetShaderLocation(postProcess, "resolution");
        target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

        net.Initialize();
        activeUI = new Vis2D_Standard(session.gridSize, session.bombs, &net, &session);
        Theme::InitGlobalAssets();
        activeUI->Init(&ground);

        session.Load();
    }

    void HandleNetwork() {
        net.Update();
        NetEvent ev;

        while (net.PollEvent(ev)) {
            if (ev.type == NET_EVENT_CLIENT_CONNECTED) {
                PacketInit p;
                p.dim = session.dim; p.size = session.gridSize; p.bombs = session.bombs; p.seed = session.currentSeed;
                net.Broadcast(&p, sizeof(p));
            }
            else if (ev.type == NET_EVENT_INIT_BOARD) {
                session.nextDim = ev.initData.dim;
                session.nextGridSize = ev.initData.size;
                session.nextBombs = ev.initData.bombs;
                session.currentSeed = ev.initData.seed;
                session.nextSeed = session.currentSeed;
                session.generateQueued = true;

                currentState = AppState::IN_GAME;
            }
            else if (ev.type == NET_EVENT_PLAYER_CLICK || ev.type == NET_EVENT_BOARD_RESULT) {
                size_t idx = (ev.type == NET_EVENT_PLAYER_CLICK) ? ev.clickData.index : ev.resultData.index;
                uint8_t action = (ev.type == NET_EVENT_PLAYER_CLICK) ? ev.clickData.action : ev.resultData.state;

                if (action == 0 && ground.state.get(idx) == 0) {
                    ground.reveal(idx);
                    if (ground.getBomb(idx)) activeUI->gameOver = true;
                    else if (ground.revealed_cells == ground.total_cells - session.bombs) activeUI->victory = true;
                }
                else if (action == 2) {
                    int s = ground.state.get(idx);
                    if (s == 0 || s == 2) {
                        ground.state.set(idx, (s == 0) ? 2 : 0);
                    }
                }

                if (ev.type == NET_EVENT_PLAYER_CLICK) {
                    PacketResult res;
                    res.index = idx; res.state = action;
                    net.Broadcast(&res, sizeof(res));
                }
            }
        }
    }

    void GenerateNewGame() {
        if (session.randomizeSeed) session.nextSeed = (uint64_t)(rand() << 32) | rand();

        session.dim = session.nextDim;
        session.gridSize = session.nextGridSize;
        session.currentSeed = session.nextSeed;

        int max_cells = 1;
        for (int i = 0; i < session.dim; i++) max_cells *= session.gridSize;

        if (session.nextBombs >= max_cells) session.nextBombs = max_cells - 1;
        if (session.nextBombs < 1) session.nextBombs = 1;
        session.bombs = session.nextBombs;

        ground.seed = session.currentSeed;
        ground.init(session.dim, session.gridSize);
        generateBombsExact(ground, session.bombs, session.currentSeed);
        ground.buildCache();

        delete activeUI;
        if (session.dim >= 4) activeUI = new Vis4D_Slices(session.gridSize, session.bombs, &net);
        else if (session.dim == 3) activeUI = new Vis3D_Slices(session.gridSize, session.bombs);
        else activeUI = new Vis2D_Standard(session.gridSize, session.bombs, &net, &session);

        activeUI->Init(&ground);
        session.timePlayed = 0.0f;
        session.generateQueued = false;
    }

    void Update() {
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (IsWindowResized()) {
            UnloadRenderTexture(target);
            target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }

        if (currentState == AppState::IN_GAME) {

            if (XRAY && IsKeyPressed('X')) {
                for (size_t i = 0; i < ground.total_cells; i++) {
                    if (!ground.getBomb(i)) ground.reveal(i);
                }
            }

            if (session.generateQueued) {
                GenerateNewGame();
            }

            if (GetCRTMousePosition().y < GetScreenHeight() - 60 && !session.alertOn) {
                activeUI->HandleInput();
            }

            if (!activeUI->gameOver && !activeUI->victory && ground.revealed_cells) {
                session.timePlayed += GetFrameTime();
            }

            if ((activeUI->gameOver || activeUI->victory) && IsKeyPressed(KEY_R)) {
                session.timePlayed = 0.0f;
            }
        }
    }

    void Draw() {
        BeginTextureMode(target);

        bool isMenu = (currentState == AppState::MAIN_MENU || currentState == AppState::JOIN_MENU || currentState == AppState::HOST_MENU || currentState == AppState::CUSTOMIZE_MENU);
        ClearBackground(isMenu ? GameUI::Zinc900 : BLACK);

        if (isMenu) {
            MainMenu::DrawAndProcess(currentState, shouldQuit, session, net);
        }
        else if (currentState == AppState::IN_GAME) {
            activeUI->Draw();
            GameHUD::DrawAndProcess(currentState, session, ground, net, activeUI);
        }

        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        BeginShaderMode(postProcess);

        float timeValue = GetTime();
        SetShaderValue(postProcess, timeLoc, &timeValue, SHADER_UNIFORM_FLOAT);

        float resValue[2] = { (float)target.texture.width, (float)target.texture.height };
        SetShaderValue(postProcess, resLoc, resValue, SHADER_UNIFORM_VEC2);

        Rectangle source = { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };
        Rectangle dest = { 0.0f, 0.0f, (float)target.texture.width, (float)target.texture.height };
        DrawTexturePro(target.texture, source, dest, { 0, 0 }, 0.0f, WHITE);

        EndShaderMode();
        EndDrawing();
    }

    void Cleanup() {
        UnloadRenderTexture(target);
        UnloadShader(postProcess);
        Theme::UnloadAssets();
        net.Cleanup();
        delete activeUI;
        CloseWindow();
    }
};