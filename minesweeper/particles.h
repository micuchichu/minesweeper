#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cstdlib>

namespace Particles {
    struct Particle {
        Vector2 position;
        Vector2 velocity;
        Color color;
        float life;
        float maxLife;
        float size;
        float rotation;
        float rotSpeed;
    };

    static std::vector<Particle> activeParticles;
    static bool initialized = false;

    inline float RandomFloat(float min, float max) {
        return min + (rand() / (float)RAND_MAX) * (max - min);
    }

    inline void InitPool() {
        if (!initialized) {
            activeParticles.reserve(5000);
            initialized = true;
        }
    }

    inline void EmitExplosion(Vector2 position, int count, Color color) {
        InitPool();
        for (int i = 0; i < count; i++) {
            float angle = RandomFloat(0.0f, 2.0f * PI);
            float speed = RandomFloat(500.0f, 6000.0f);
            activeParticles.push_back({
                position,
                { (float)cos(angle) * speed, (float)sin(angle) * speed },
                color,
                RandomFloat(0.5f, 1.5f),
                1.5f,
                RandomFloat(3.0f, 8.0f),
                RandomFloat(0.0f, 360.0f),
                RandomFloat(-300.0f, 300.0f)
                });
        }
    }

    inline void EmitDebris(Vector2 position, int count, Color color) {
        InitPool();
        for (int i = 0; i < count; i++) {
            float angle = RandomFloat(0.0f, 2.0f * PI);
            float speed = RandomFloat(200.0f, 600.0f);
            activeParticles.push_back({
                position,
                { (float)cos(angle) * speed, (float)sin(angle) * speed },
                color,
                RandomFloat(0.2f, 0.5f),
                0.5f,
                RandomFloat(2.0f, 5.0f),
                RandomFloat(0.0f, 360.0f),
                RandomFloat(-200.0f, 200.0f)
                });
        }
    }

    inline void UpdateAndDraw() {
        float dt = GetFrameTime();

        for (int i = 0; i < (int)activeParticles.size(); ) {
            Particle& p = activeParticles[i];
            p.life -= dt;

            if (p.life <= 0) {
                activeParticles[i] = activeParticles.back();
                activeParticles.pop_back();
            }
            else {
                p.velocity.y += 800.0f * dt;
                p.velocity.x *= 0.94f;
                p.velocity.y *= 0.98f;

                p.position.x += p.velocity.x * dt;
                p.position.y += p.velocity.y * dt;
                p.rotation += p.rotSpeed * dt;

                float alpha = p.life / p.maxLife;
                Color drawColor = p.color;
                drawColor.a = (unsigned char)(alpha * 255.0f);

                Rectangle rect = { p.position.x, p.position.y, p.size, p.size };
                DrawRectanglePro(rect, { p.size / 2.0f, p.size / 2.0f }, p.rotation, drawColor);

                i++;
            }
        }
    }

    inline void Clear() {
        activeParticles.clear();
    }
}