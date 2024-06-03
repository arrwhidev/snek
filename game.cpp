#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <iostream>
#include <string>

// utils

float GetRandomFloat(float a, float b) {
    return ((b - a) * ((float)rand() / RAND_MAX)) + a;
}

// constants

const int WIDTH           = 320;
const int HEIGHT          = 240;
const int BLOCK_WIDTH     = 10;
const int BLOCK_HEIGHT    = 10;
const int SCALING_FACTOR  = 3;
const int DIRECTION_UP    = 0;
const int DIRECTION_DOWN  = 1;
const int DIRECTION_LEFT  = 2;
const int DIRECTION_RIGHT = 3;

// variables

int score = 0;
Shader bloomShader;

// interfaces

class GameObject {
public:
    virtual void render() = 0;
    virtual void update(float dt) = 0;
};

class Scene : public GameObject {
protected:
    Camera2D camera;
};

// classes

class Particle : public GameObject {
private:
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    float size;
    float life;
    float maxLife;
    float rotation;

public:
    Particle(Vector2 position, Vector2 velocity, Vector2 acceleration, float life, float size) {
        this->position = position;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->life = life;
        this->maxLife = life;
        this->size = size;
        this->rotation = 0.0f;
    }

    void render() override {
        if (this->isAlive()) {
            float lifeBasedAlpha = 1.0f * (this->life / this->maxLife);
            DrawRectanglePro(
                { position.x, position.y, this->size, this->size },
                { 0, 0 },
                this->rotation,
                ColorAlpha(BLACK, lifeBasedAlpha));
        }
    }

    void update(float dt) override {
        this->life--;
        if (this->isAlive()) {
            this->rotation += 0.7f;

            this->velocity.x += this->acceleration.x * dt;
            this->velocity.y += this->acceleration.y * dt;
            this->position.x += this->velocity.x * dt;
            this->position.y += this->velocity.y * dt;

            // kill particle when oob
            if (this->position.x < 0 || this->position.x > WIDTH || this->position.y < 0 || this->position.y > HEIGHT) {
                this->life = 0;
            }
        }
    }

    bool isAlive() {
        return this->life > 0;
    }
};

class ParticleEmitter : public GameObject {
private:
    int particlesAlive;
    std::vector<Particle*> particles;

public:
    ParticleEmitter(Vector2 position) {
        int num = 45;
        this->particlesAlive = num;
        for (size_t i = 0; i < num; i++)
        {
            float size = GetRandomValue(2.0f, 5.0f);
            float speed = GetRandomFloat(50.0f, 135.0f);
            float life = GetRandomFloat(20.0f, 50.0f);
            float randomAngle = GetRandomValue(0, 360) * (PI / 180.0f);
            Vector2 velocity = { cosf(randomAngle) * speed, sinf(randomAngle) * speed };
            Vector2 acceleration = { 22.0f, 22.0f };
            this->particles.push_back(new Particle(position, velocity, acceleration, life, size));
        }        
    }

    void render() override {
        if (this->hasParticles()) {
            for (auto &p : this->particles) {
                p->render();
            }
        }
    }

    void update(float dt) override {
        if (this->hasParticles()) {
            for (auto p : this->particles) {
                p->update(dt);
            }

            // clean up dead particles
        }
    }

    bool hasParticles() {
        return this->particlesAlive > 0;
    }
};

class Food : public GameObject {
private:
    Rectangle rect;

public:
    Food() {
        this->rect = { 30, 30, BLOCK_WIDTH, BLOCK_HEIGHT };
    }

    void render() override {
        DrawRectangleRec(this->rect, BLUE);
    }

    void update(float dt) override {}

    void eaten() {
        this->rect.x = GetRandomValue(0, WIDTH);
        this->rect.y = GetRandomValue(0, HEIGHT);
    }

    Rectangle getRect() {
        return this->rect;
    }
};

class Snake : public GameObject {
private:
    float moveTimer = 0.0f;
    int direction;
    std::vector<Vector2> body;
    
public:
    Snake(Vector2 start, int direction) {
        this->direction = direction;

        this->body.push_back(start);
        this->body.push_back({start.x + (BLOCK_WIDTH * 1), start.y});
        this->body.push_back({start.x + (BLOCK_WIDTH * 2), start.y});
        this->body.push_back({start.x + (BLOCK_WIDTH * 3), start.y});
        this->body.push_back({start.x + (BLOCK_WIDTH * 4), start.y});
    }

    void render() override {
        for (auto& segment : this->body) {
            DrawRectangle(
                segment.x,
                segment.y,
                BLOCK_WIDTH,
                BLOCK_HEIGHT,
                RED);
        }
    }

    int getDirectionFromInput() {
        if (IsKeyDown(KEY_UP))
            return DIRECTION_UP;
        else if (IsKeyDown(KEY_DOWN))
            return DIRECTION_DOWN;
        else if (IsKeyDown(KEY_LEFT))
            return DIRECTION_LEFT;
        else if (IsKeyDown(KEY_RIGHT))
            return DIRECTION_RIGHT;
        else 
            return this->direction;
    }

    void update(float dt) override {
        this->moveTimer += dt;
        if (this->moveTimer >= 0.05f) {
            this->direction = getDirectionFromInput();

            Vector2 newHead = { 
                this->body[0].x,
                this->body[0].y
            };

            switch (this->direction)
            {
                case DIRECTION_UP:
                    newHead.y -= BLOCK_HEIGHT;
                    break;
                case DIRECTION_DOWN:
                    newHead.y += BLOCK_HEIGHT;
                    break;
                case DIRECTION_LEFT:
                    newHead.x -= BLOCK_WIDTH;
                    break;
                case DIRECTION_RIGHT:
                    newHead.x += BLOCK_WIDTH;
                    break;
                default:
                    break;
            }

            // check oob
            if (this->body.front().x < 0) {
                newHead.x = WIDTH - BLOCK_WIDTH;
            }
            if (this->body.front().x + BLOCK_WIDTH > WIDTH) {
                newHead.x = 0;
            }
            if (this->body.front().y < 0) {
                newHead.y = HEIGHT - BLOCK_WIDTH;
            }
            if (this->body.front().y + BLOCK_HEIGHT > HEIGHT) {
                newHead.y = 0;
            }

            // add new head & remove tail
            this->body.insert(body.begin(), newHead);
            this->body.pop_back();
            
            this->moveTimer = 0.0f;
        }
    }

    void grow() {
        Vector2 tail = this->body.back();
        switch (this->direction)
        {
            case DIRECTION_UP:
                this->body.push_back({tail.x, tail.y + BLOCK_HEIGHT});
                break;
            case DIRECTION_DOWN:
                this->body.push_back({tail.x, tail.y - BLOCK_HEIGHT});
                break;
            case DIRECTION_LEFT:
                this->body.push_back({tail.x + BLOCK_WIDTH, tail.y});
                break;
            case DIRECTION_RIGHT:
                this->body.push_back({tail.x - BLOCK_HEIGHT, tail.y});
                break;
            default:
                break;
        }
    }

    Rectangle getHead() {
        Vector2 head = this->body.front();
        return {
            head.x,
            head.y,
            BLOCK_WIDTH,
            BLOCK_HEIGHT
        };
    }
};

class GameUI : public GameObject {
public:
    void render() override {
        std::string scoreText = "score: " + std::to_string(score);
        DrawText(scoreText.c_str(), 10, 10, 10, BLACK);
    }

    void update(float dt) override {}
};

// scenes

class GameScene : public Scene { 
private:
    RenderTexture2D renderTexture = LoadRenderTexture(WIDTH, HEIGHT);
    RenderTexture2D uiTexture = LoadRenderTexture(WIDTH, HEIGHT);
    Snake snake = Snake({ WIDTH / 2, HEIGHT / 2 }, DIRECTION_LEFT);
    Food food;
    GameUI ui;

public:
    GameScene() { 
        this->camera = { 0 };
        this->camera.rotation = 0.0f;
        this->camera.zoom = 1.0f;
    }
    
    void update(float dt) override {
        snake.update(dt);
        food.update(dt);

        if (CheckCollisionRecs(snake.getHead(), food.getRect())) {
            food.eaten();
            snake.grow();
            score++;
        }

        // TODO: add snake collide with itself
    }

    void render() override {
        // Render game objects to texture at internal resolution
        BeginTextureMode(renderTexture);
        ClearBackground(WHITE);
        BeginMode2D(camera);
            snake.render();
            food.render();
        EndTextureMode();

        // Render ui to separate texture
        BeginTextureMode(uiTexture);
        ClearBackground({ 0, 0, 0, 0 });
            ui.render();
        EndTextureMode();

        // Render the texture and scale it
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // BeginShaderMode(bloomShader);
            DrawTexturePro(renderTexture.texture,
                { 0.0f, 0.0f, (float)renderTexture.texture.width, (float)-renderTexture.texture.height },
                { 0.0f, 0.0f, (float)renderTexture.texture.width * SCALING_FACTOR, (float)renderTexture.texture.height * SCALING_FACTOR },
                { 0.0f, 0.0f },
                0.0f,
                WHITE);
        // EndShaderMode();

        DrawTexturePro(uiTexture.texture,
            { 0.0f, 0.0f, (float)uiTexture.texture.width, (float)-uiTexture.texture.height },
            { 0.0f, 0.0f, (float)uiTexture.texture.width * SCALING_FACTOR, (float)uiTexture.texture.height * SCALING_FACTOR },
            { 0.0f, 0.0f },
            0.0f,
            WHITE);
        EndDrawing();
    }
};

int main(void) {
    InitWindow(WIDTH * SCALING_FACTOR, HEIGHT * SCALING_FACTOR, "snek");
    SetTargetFPS(60);
    bloomShader = LoadShader(0, TextFormat("bloom.fs", 330));

    Scene* scene = new GameScene();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        scene->update(dt);
        scene->render();
    }

    CloseWindow();
    return 0;
}
