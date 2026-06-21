#pragma once

#include "engine.h"

class IVisualizer {
protected:
    GroundND* ground;

    int &GRID_SIZE, &BOMBS;

public:
    bool gameOver = false;
    bool victory = false;

	IVisualizer(int& gridSize, int& bombs) : GRID_SIZE(gridSize), BOMBS(bombs) {}
    virtual ~IVisualizer() = default;

    void Init(GroundND* engineRef) {
        ground = engineRef;
        OnInit();
    }

    virtual void OnInit() = 0;
    virtual void HandleInput() = 0;
    virtual void Draw() = 0;
};