#pragma once

#include <cstdint>
#include <fstream>

enum class AppState {
    MAIN_MENU,
    JOIN_MENU,
    HOST_MENU,
    CUSTOMIZE_MENU,
    IN_GAME
};

struct GameSession {
    int dim = 2;
    int gridSize = 10;
    int bombs = 10;
    uint64_t currentSeed = 12345;

    int nextDim = 2;
    int nextGridSize = 10;
    int nextBombs = 10;
    uint64_t nextSeed = 12345;
    bool randomizeSeed = true;

    bool generateQueued = false;
    bool alertOn = false;
    float timePlayed = 0.0f;

    char joinIp[64] = "127.0.0.1:7777";
    char hostPort[16] = "7777";
    char connectionMsg[128] = "";
    char playerName[16] = "Player";

    struct SaveData {
        char joinIp[64];
        char hostPort[16];
        char playerName[16];
        int nextDim;
        int nextGridSize;
        int nextBombs;
        uint64_t nextSeed;
    };

    void Load() {
        std::ifstream file("savegame.dat", std::ios::binary);
        if (file.is_open()) {
            SaveData data;
            if (file.read(reinterpret_cast<char*>(&data), sizeof(SaveData))) {
#ifdef _WIN32
                strncpy_s(joinIp, data.joinIp, 64);
                strncpy_s(hostPort, data.hostPort, 16);
                strncpy_s(playerName, data.playerName, 16);
#else
                strncpy(joinIp, data.joinIp, 64);
                strncpy(hostPort, data.hostPort, 16);
                strncpy(playerName, data.playerName, 16);
                joinIp[63] = '\0';
                hostPort[15] = '\0';
                playerName[15] = '\0';
#endif

                nextDim = data.nextDim;
                nextGridSize = data.nextGridSize;
                nextBombs = data.nextBombs;
                nextSeed = data.nextSeed;

                dim = nextDim;
                gridSize = nextGridSize;
                bombs = nextBombs;
                currentSeed = nextSeed;
            }
            file.close();
        }
    }

    void Save() {
        std::ofstream file("savegame.dat", std::ios::binary);
        if (file.is_open()) {
            SaveData data;
#ifdef _WIN32
            strncpy_s(data.joinIp, joinIp, 64);
            strncpy_s(data.hostPort, hostPort, 16);
            strncpy_s(data.playerName, playerName, 16);
#else
            strncpy(data.joinIp, joinIp, 64);
            strncpy(data.hostPort, hostPort, 16);
            strncpy(data.playerName, playerName, 16);
#endif

            data.nextDim = nextDim;
            data.nextGridSize = nextGridSize;
            data.nextBombs = nextBombs;
            data.nextSeed = nextSeed;

            file.write(reinterpret_cast<char*>(&data), sizeof(SaveData));
            file.close();
        }
    }
};