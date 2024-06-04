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
const int SCALING_FACTOR  = 3;

const int DIRECTION_UP    = 0;
const int DIRECTION_DOWN  = 1;
const int DIRECTION_LEFT  = 2;
const int DIRECTION_RIGHT = 3;

const int BLOCK_SIZE           = 5;
const float SNAKE_MOVE_SPEED   = 0.03f;
const float SPFOOD_SPAWN_TIMER = 8.0f;
const float SPFOOD_ALIVE_TIMER = 3.5f;

const int EMITTER_STATE_ACTIVE   = 0;
const int EMITTER_STATE_STOPPED  = 1;
const int EMITTER_STATE_STOPPING = 2;

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
    Color color;
    float size;
    float life;
    float maxLife;
    float rotation;

public:
    Particle(Vector2 position, Vector2 velocity, Vector2 acceleration, float life, float size, Color color) {
        this->position = position;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->life = life;
        this->maxLife = life;
        this->size = size;
        this->rotation = 0.0f;
        this->color = color;
    }

    void render() override {
        if (this->isAlive()) {
            float lifeBasedAlpha = 1.0f * (this->life / this->maxLife);
            DrawRectanglePro(
                { position.x, position.y, this->size, this->size },
                { 0, 0 },
                this->rotation,
                ColorAlpha(this->color, lifeBasedAlpha));
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
    int state;
    int particlesTarget;
    int particlesAlive;
    Vector2 position;
    Color color;
    std::vector<Particle*> particles;

public:
    ParticleEmitter(Vector2 position, int num, Color color) {
        this->particlesAlive = 0;
        this->particlesTarget = num;
        this->position = position;
        this->color = color;
        this->state = EMITTER_STATE_STOPPED;
    }

    void addParticle() {
        float size = GetRandomValue(2.0f, 5.0f);
        float speed = GetRandomFloat(20.0f, 70.0f);
        float life = GetRandomFloat(20.0f, 50.0f);
        float randomAngle = GetRandomValue(0, 360) * (PI / 180.0f);
        Vector2 velocity = { cosf(randomAngle) * speed, sinf(randomAngle) * speed };
        Vector2 acceleration = { 22.0f, 22.0f };
        
        this->particles.push_back(new Particle(this->position, velocity, acceleration, life, size, this->color));
        this->particlesAlive += 1;
    }

    void activate() {
        this->state = EMITTER_STATE_ACTIVE;
    }

    void stop() {
        this->state = EMITTER_STATE_STOPPING;
    }

    void render() override {
        if ((this->state == EMITTER_STATE_ACTIVE || this->state == EMITTER_STATE_STOPPING) && this->hasParticles()) {
            for (auto &p : this->particles) {
                p->render();
            }
        }
    }

    void update(float dt) override {
        if (this->state == EMITTER_STATE_ACTIVE || this->state == EMITTER_STATE_STOPPING) {
            if (this->state == EMITTER_STATE_ACTIVE) {
                while(particlesAlive < particlesTarget) {
                    addParticle();
                }
            }

            if (this->hasParticles()) {
                for (auto p : this->particles) {
                    p->update(dt);
                }

                for (auto it = particles.rbegin(); it != particles.rend();) {
                    if (!(*it)->isAlive()) {
                        delete *it;
                        it = std::vector<Particle*>::reverse_iterator(particles.erase((it + 1).base()));
                        this->particlesAlive -= 1;
                    } else {
                        ++it;
                    }
                }
            }

            if (this->state == EMITTER_STATE_STOPPING && !this->hasParticles()) {
                this->state = EMITTER_STATE_STOPPED;
            }
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
        this->rect = { 
            std::round(GetRandomFloat(0, WIDTH - BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE,
            std::round(GetRandomFloat(0, HEIGHT - BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE,
            BLOCK_SIZE, 
            BLOCK_SIZE 
        };
    }

    void render() override {
        DrawRectangleRec(this->rect, BLUE);
    }

    void update(float dt) override {}

    void eaten() {
        this->rect.x = std::round(GetRandomFloat(0, WIDTH - BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE;
        this->rect.y = std::round(GetRandomFloat(0, HEIGHT - BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE;
    }

    Rectangle getRect() {
        return this->rect;
    }
};

class SpecialFood : public Food {
private:
    ParticleEmitter emitter;
    Rectangle rect;
    bool active = false;
    float activeTimer = 0.0f;

public:
    SpecialFood() :
        emitter({ 0, 0 }, 25, GOLD) {
    }

    void render() override {
        this->emitter.render();

        if (this->active) {
            DrawRectangleRec(this->rect, GOLD);
        }
    }

    void update(float dt) override {
        this->activeTimer += dt;
        if (!this->active && activeTimer >= SPFOOD_SPAWN_TIMER) {
            // TODO: do not spawn on food or snake
            this->rect.x = std::round(GetRandomFloat(0, WIDTH - BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE;
            this->rect.y = std::round(GetRandomFloat(0, HEIGHT - BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE;
            this->rect.width = BLOCK_SIZE;
            this->rect.height = BLOCK_SIZE;

            this->emitter = ParticleEmitter({ this->rect.x, this->rect.y }, 25, GOLD);
            this->emitter.activate();

            this->active = true;
        }
        
        this->emitter.update(dt);

        if (this->active && activeTimer >= SPFOOD_SPAWN_TIMER + SPFOOD_ALIVE_TIMER) {
            this->active = false;
            this->activeTimer = 0.0f;
            this->emitter.stop();
        }
    }

    void eaten() {
        this->active = false;
        this->activeTimer = 0.0f;
        this->emitter.stop();
    }

    Rectangle getRect() {
        if (this->active) {
            return this->rect;
        }
        return { -1, -1, 0, 0 };
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
        this->body.push_back({start.x + (BLOCK_SIZE  * 1), start.y});
        this->body.push_back({start.x + (BLOCK_SIZE  * 2), start.y});
        this->body.push_back({start.x + (BLOCK_SIZE  * 3), start.y});
        this->body.push_back({start.x + (BLOCK_SIZE  * 4), start.y});
    }

    void render() override {
        for (auto& segment : this->body) {
            DrawRectangle(
                segment.x,
                segment.y,
                BLOCK_SIZE ,
                BLOCK_SIZE,
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
        if (this->moveTimer >= SNAKE_MOVE_SPEED) {
            this->direction = getDirectionFromInput();

            Vector2 newHead = { 
                this->body[0].x,
                this->body[0].y
            };

            switch (this->direction)
            {
                case DIRECTION_UP:
                    newHead.y -= BLOCK_SIZE;
                    break;
                case DIRECTION_DOWN:
                    newHead.y += BLOCK_SIZE;
                    break;
                case DIRECTION_LEFT:
                    newHead.x -= BLOCK_SIZE ;
                    break;
                case DIRECTION_RIGHT:
                    newHead.x += BLOCK_SIZE ;
                    break;
                default:
                    break;
            }

            // check oob
            if (this->body.front().x < 0) {
                newHead.x = WIDTH - BLOCK_SIZE ;
            }
            if (this->body.front().x + BLOCK_SIZE  > WIDTH) {
                newHead.x = 0;
            }
            if (this->body.front().y < 0) {
                newHead.y = HEIGHT - BLOCK_SIZE ;
            }
            if (this->body.front().y + BLOCK_SIZE > HEIGHT) {
                newHead.y = 0;
            }

            // add new head & remove tail
            this->body.insert(body.begin(), newHead);
            this->body.pop_back();
            
            this->moveTimer = 0.0f;
        }
    }

    bool isSelfColliding() {
        Rectangle head = this->getHead();
        for (size_t i = 1; i < this->body.size(); i++) {
            Rectangle segment = {
                this->body.at(i).x,
                this->body.at(i).y,
                BLOCK_SIZE,
                BLOCK_SIZE
            };
            if (CheckCollisionRecs(head, segment)) {
                return true;
            }
        }
        return false;
    }

    void grow() {
        Vector2 tail = this->body.back();
        Vector2 preTail = this->body[this->body.size() - 2];
        if (preTail.x < tail.x) {
            // add to right of tail
            this->body.push_back({tail.x + BLOCK_SIZE, tail.y});
        } else if (preTail.x > tail.x) {
            // add to left of tail
            this->body.push_back({tail.x - BLOCK_SIZE, tail.y});
        } else if (preTail.y < tail.y) {
            // add below tail
            this->body.push_back({tail.x, tail.y + BLOCK_SIZE});
        } else {
            // add above tail
            this->body.push_back({tail.x, tail.y - BLOCK_SIZE});
        }
    }

    Rectangle getHead() {
        Vector2 head = this->body.front();
        return {
            head.x,
            head.y,
            BLOCK_SIZE ,
            BLOCK_SIZE
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
    SpecialFood specialFood;
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
        specialFood.update(dt);

        if (CheckCollisionRecs(snake.getHead(), food.getRect())) {
            food.eaten();
            snake.grow();
            score++;
        }

        if (CheckCollisionRecs(snake.getHead(), specialFood.getRect())) {
            specialFood.eaten();
            snake.grow();
            score += 3;
        }

        if (snake.isSelfColliding()) {
            // TODO: game over
        }
    }

    void render() override {
        // Render game objects to texture at internal resolution
        BeginTextureMode(renderTexture);
        ClearBackground(RAYWHITE);
        BeginMode2D(camera);
            snake.render();
            food.render();
            specialFood.render();
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
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

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
