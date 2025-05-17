#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <vector>
#include <ctime>
#include <algorithm>

struct Player {
    Vector2 pos;
    Color color;
    int score;
    int up, down, left, right, shoot, reload;
    float shootCooldown;
    int health;
    int bulletsLeft;
    bool reloading;
    float reloadTimer;
};

struct Bullet {
    Vector2 pos, vel;
    int owner; 
};

struct Barrier {
    Rectangle rect;
};

struct Powerup {
    Vector2 pos;
    int type; 
    float timer;
    bool active;
};

float RandomFloat(float min, float max) {
    return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

int main() {
    const int screenWidth = 1920, screenHeight = 1080;
    InitWindow(screenWidth, screenHeight, "1v1 Top Down Shooter");
    SetTargetFPS(60);
    srand((unsigned int)time(0));

    Player players[2] = {
        { {100, 300}, BLUE, 0, KEY_W, KEY_S, KEY_A, KEY_D, KEY_SPACE, KEY_Q, 0, 5, 6, false, 0.0f },
        { {700, 300}, RED, 0, KEY_I, KEY_K, KEY_J, KEY_L, KEY_C, KEY_V, 0, 5, 6, false, 0.0f }
    };

    std::vector<Bullet> bullets;
    bool gameOver = false;
    int winner = -1;

    std::vector<Barrier> barriers = {
        { {400, 200, 200, 30} },
        { {1320, 200, 200, 30} },
        { {400, 850, 200, 30} },
        { {1320, 850, 200, 30} },
        { {200, 400, 30, 200} },
        { {1690, 400, 30, 200} },
        { {900, 500, 120, 40} }
    };

    std::vector<Powerup> powerups;
    float powerupSpawnTimer = 0.0f;
    const float powerupSpawnInterval = 5.0f; 

    float rapidFireTimers[2] = {0, 0};

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        powerupSpawnTimer -= dt;
        if (powerupSpawnTimer <= 0 && powerups.size() < 3) {
            Powerup p;
            p.pos = {RandomFloat(60, screenWidth-60), RandomFloat(60, screenHeight-60)};
            p.type = GetRandomValue(0, 1);
            p.timer = 10.0f; 
            p.active = true;

            Rectangle pRect = {p.pos.x-12, p.pos.y-12, 24, 24};
            bool collides = false;
            for (const auto& barrier : barriers) {
                if (CheckCollisionRecs(pRect, barrier.rect)) {
                    collides = true;
                    break;
                }
            }
            if (!collides) {
                powerups.push_back(p);
                powerupSpawnTimer = powerupSpawnInterval;
            } else {
                powerupSpawnTimer = 1.0f; 
            }
        }

        for (auto& p : powerups) {
            if (p.active) p.timer -= dt;
        }

        powerups.erase(
            std::remove_if(powerups.begin(), powerups.end(), [](const Powerup& p){ return p.timer <= 0; }),
            powerups.end()
        );

        if (!gameOver) {
            for (int i = 0; i < 2; i++) {
                Vector2 oldPos = players[i].pos;

                if (IsKeyDown(players[i].up)) players[i].pos.y -= 4;
                if (IsKeyDown(players[i].down)) players[i].pos.y += 4;
                if (IsKeyDown(players[i].left)) players[i].pos.x -= 4;
                if (IsKeyDown(players[i].right)) players[i].pos.x += 4;
                players[i].pos.x = Clamp(players[i].pos.x, 20, screenWidth-20);
                players[i].pos.y = Clamp(players[i].pos.y, 20, screenHeight-20);

                Rectangle playerRect = {players[i].pos.x-16, players[i].pos.y-16, 32, 32};
                for (const auto& barrier : barriers) {
                    if (CheckCollisionRecs(playerRect, barrier.rect)) {
                        players[i].pos = oldPos; 
                        break;
                    }
                }

                for (auto& p : powerups) {
                    if (p.active && CheckCollisionCircles(players[i].pos, 18, p.pos, 12)) {
                        if (p.type == 0 && players[i].health < 5) {
                            players[i].health++;
                        } else if (p.type == 1) {
                            rapidFireTimers[i] = 5.0f; 
                        }
                        p.active = false;
                    }
                }

                if (rapidFireTimers[i] > 0) rapidFireTimers[i] -= dt;

                if (players[i].reloading) {
                    players[i].reloadTimer -= dt;
                    if (players[i].reloadTimer <= 0) {
                        players[i].bulletsLeft = 6;
                        players[i].reloading = false;
                    }
                } else {

                    if (IsKeyPressed(players[i].reload) && players[i].bulletsLeft < 6) {
                        players[i].reloading = true;
                        players[i].reloadTimer = 1.2f; 
                    }
                }

                if (players[i].shootCooldown > 0) players[i].shootCooldown -= dt;
                float shootDelay = (rapidFireTimers[i] > 0) ? 0.1f : 0.3f;
                if (!players[i].reloading && players[i].bulletsLeft > 0 &&
                    IsKeyDown(players[i].shoot) && players[i].shootCooldown <= 0) {
                    Vector2 dir = (i == 0) ? (Vector2){1,0} : (Vector2){-1,0};
                    bullets.push_back({players[i].pos, Vector2Scale(dir, 8), i});
                    players[i].shootCooldown = shootDelay;
                    players[i].bulletsLeft--;
                }
            }

            for (auto& b : bullets) b.pos = Vector2Add(b.pos, b.vel);

            for (size_t i = 0; i < bullets.size(); ) {
                bool erased = false;

                Rectangle bulletRect = {bullets[i].pos.x-8, bullets[i].pos.y-8, 16, 16};
                for (const auto& barrier : barriers) {
                    if (CheckCollisionRecs(bulletRect, barrier.rect)) {
                        bullets.erase(bullets.begin() + i);
                        erased = true;
                        break;
                    }
                }
                if (erased) continue;

                for (int p = 0; p < 2; p++) {
                    if (p != bullets[i].owner && CheckCollisionCircles(bullets[i].pos, 8, players[p].pos, 16)) {
                        players[p].health--;
                        if (players[p].health <= 0) {
                            players[bullets[i].owner].score++;
                            if (players[bullets[i].owner].score >= 5) {
                                gameOver = true;
                                winner = bullets[i].owner;
                            }
                            players[0].pos = {100, 300};
                            players[1].pos = {700, 300};
                            players[0].health = 5;
                            players[1].health = 5;
                            rapidFireTimers[0] = 0;
                            rapidFireTimers[1] = 0;
                            bullets.clear();
                            powerups.clear();
                            powerupSpawnTimer = 1.0f;
                            erased = true;
                            break;
                        }
                        bullets.erase(bullets.begin() + i);
                        erased = true;
                        break;
                    }
                }
                if (!erased && (bullets[i].pos.x < 0 || bullets[i].pos.x > screenWidth ||
                                bullets[i].pos.y < 0 || bullets[i].pos.y > screenHeight)) {
                    bullets.erase(bullets.begin() + i);
                } else if (!erased) {
                    i++;
                }
            }
        } else {
            if (IsKeyPressed(KEY_ENTER)) {
                for (int i = 0; i < 2; i++) {
                    players[i].score = 0;
                    players[i].health = 5;
                    rapidFireTimers[i] = 0;
                    players[i].bulletsLeft = 6;
                    players[i].reloading = false;
                    players[i].reloadTimer = 0.0f;
                }
                bullets.clear();
                powerups.clear();
                players[0].pos = {100, 300};
                players[1].pos = {700, 300};
                powerupSpawnTimer = 1.0f;
                gameOver = false;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (const auto& barrier : barriers) {
            DrawRectangleRec(barrier.rect, DARKGRAY);
        }

        for (const auto& p : powerups) {
            if (!p.active) continue;
            Color c = (p.type == 0) ? GREEN : ORANGE;
            DrawCircleV(p.pos, 12, c);
            DrawCircleLines((int)p.pos.x, (int)p.pos.y, 12, DARKGRAY);
            if (p.type == 0)
                DrawText("H", (int)p.pos.x-6, (int)p.pos.y-10, 20, DARKGREEN);
            else
                DrawText("R", (int)p.pos.x-6, (int)p.pos.y-10, 20, ORANGE);
        }

        for (int i = 0; i < 2; i++) {
            DrawCircleV(players[i].pos, 16, players[i].color);
            DrawText(TextFormat("P%d: %d", i+1, players[i].score), 20 + i*700, 20, 20, players[i].color);

            int barWidth = 40;
            int barHeight = 8;
            int x = (int)players[i].pos.x - barWidth/2;
            int y = (int)players[i].pos.y - 28;
            DrawRectangle(x, y, barWidth, barHeight, GRAY);
            DrawRectangle(x, y, barWidth * players[i].health / 5, barHeight, (i==0)?BLUE:RED);
            DrawRectangleLines(x, y, barWidth, barHeight, BLACK);

            if (rapidFireTimers[i] > 0) {
                DrawText("RAPID!", x, y-18, 14, ORANGE);
            }

            int chamberY = (int)players[i].pos.y + 28;
            int chamberX = (int)players[i].pos.x - 36 + 12;
            for (int b = 0; b < 6; b++) {
                Color c = (b < players[i].bulletsLeft) ? BLACK : LIGHTGRAY;
                DrawCircle(chamberX + b*12, chamberY, 6, c);
                DrawCircleLines(chamberX + b*12, chamberY, 6, DARKGRAY);
            }

            if (players[i].reloading) {
                DrawText("RELOADING", (int)players[i].pos.x-40, chamberY+14, 14, RED);
            }
        }

        for (auto& b : bullets) DrawCircleV(b.pos, 8, (b.owner == 0) ? BLUE : RED);

        if (gameOver) {
            DrawText(TextFormat("Player %d Wins!", winner+1), screenWidth/2-100, screenHeight/2-20, 30, DARKGRAY);
            DrawText("Press ENTER to restart", screenWidth/2-110, screenHeight/2+20, 20, GRAY);
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}