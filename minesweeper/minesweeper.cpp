#include "raylib.h"
#include "engine.h"
#include "vis4d.h"
#include "vis3d.h"
#include "vis2d.h"
#include "gameui.h"

#include <algorithm>

#define XRAY true

int main() {
    int DIM = 2;
    int GRID_SIZE = 2000;
    int BOMBS = 2000000;

    GroundND ground;
    ground.init(DIM, GRID_SIZE);
    generateBombsExact(ground, BOMBS);
    ground.buildCache();
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 900, "minsper");

    Shader postProcess = LoadShader(nullptr, "./shader.fs");
    int timeLoc = GetShaderLocation(postProcess, "time");
    int resLoc = GetShaderLocation(postProcess, "resolution");

    RenderTexture2D target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    IVisualizer* activeUI = new Vis2D_Standard(GRID_SIZE, BOMBS);
    activeUI->Init(&ground);

	Theme::InitGlobalAssets();

    float timePlayed = 0.0f;

    int nextDim = DIM;
    int nextGridSize = GRID_SIZE;
    int nextBombs = BOMBS;
    bool generateQueued = false;

    bool alertOn = false;

    while (!WindowShouldClose()) {

        if (IsWindowResized()) {
            UnloadRenderTexture(target);
            target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }

        if (XRAY && IsKeyPressed('X')) {
            for (int i = 0; i < ground.total_cells; i++) {
                if (!ground.getBomb(i)) ground.reveal(i);
            }
        }

        if (generateQueued) {
            DIM = nextDim;
            GRID_SIZE = nextGridSize;

            int max_cells = 1;
            for (int i = 0; i < DIM; i++) max_cells *= GRID_SIZE;

            if (nextBombs >= max_cells) nextBombs = max_cells - 1;
            if (nextBombs < 1) nextBombs = 1;
            BOMBS = nextBombs;

            ground.init(DIM, GRID_SIZE);
            generateBombsExact(ground, BOMBS);
            ground.buildCache();

            delete activeUI;
            if (DIM >= 4) activeUI = new Vis4D_Slices(GRID_SIZE, BOMBS);
            else if (DIM == 3) activeUI = new Vis3D_Slices(GRID_SIZE, BOMBS);
            else activeUI = new Vis2D_Standard(GRID_SIZE, BOMBS);

            activeUI->Init(&ground);
            timePlayed = 0.0f;
            generateQueued = false;
        }

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        if (GetCRTMousePosition().y < screenH - 60 && !alertOn) {
            activeUI->HandleInput();
        }

        if (!activeUI->gameOver && !activeUI->victory) {
            timePlayed += GetFrameTime();
        }

        if ((activeUI->gameOver || activeUI->victory) && IsKeyPressed(KEY_R)) {
            ground.clear();
            generateBombsExact(ground, BOMBS);
            ground.buildCache();
            ground.state.init(ground.total_cells);

            activeUI->gameOver = false;
            activeUI->victory = false;
            timePlayed = 0.0f;
        }

        int flagsPlaced = 0;
        for (int i = 0; i < ground.total_cells; i++) {
            if (ground.state.get(i) == 2) flagsPlaced++;
        }

        BeginTextureMode(target);
        ClearBackground(BLACK);

        activeUI->Draw();

        GameUI::DrawHeader(screenW, DIM, BOMBS - flagsPlaced, timePlayed);

        if (GameUI::DrawFooter(screenW, screenH, nextDim, nextGridSize, nextBombs)) {
            if (std::pow(nextGridSize, nextDim) >= 100000) alertOn = true;
            else generateQueued = true;
        }

        if (alertOn) {
            if (GameUI::DrawSpinnerAlert(screenW, screenH) == 1) {
                generateQueued = true;
                alertOn = false;
            }
            else if(GameUI::DrawSpinnerAlert(screenW, screenH) == 2) {
                alertOn = false;
			}
        }

        if (activeUI->gameOver || activeUI->victory) {
            GameUI::DrawModal(screenW, screenH, activeUI->gameOver);
        }

        DrawFPS(20, screenH - 90);
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

    UnloadRenderTexture(target);
    UnloadShader(postProcess);
    delete activeUI;
    CloseWindow();
    return 0;
}