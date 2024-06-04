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

const int BLOCK_SIZE = 5;
const int MAX_BOOST  = 100;

const float SNAKE_ROTATION     = 0.08f;
const float SNAKE_MOVE_SPEED   = 100.0f;
const float SPFOOD_SPAWN_TIMER = 8.0f;
const float SPFOOD_ALIVE_TIMER = 3.5f;

const int EMITTER_STATE_ACTIVE   = 0;
const int EMITTER_STATE_STOPPED  = 1;
const int EMITTER_STATE_STOPPING = 2;

// variables

int boostRemaining = MAX_BOOST;
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

    void update(float dt) override {
    }

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
    float rotation = PI; // Start facing left
    float moveTimer = 0.0f;
    std::vector<Vector2> body;
    
public:
    Snake(Vector2 start) {
        // Assuming snake is facing left
        for (size_t i = 0; i < 8; i++) {
            this->body.push_back({start.x + (BLOCK_SIZE * i), start.y});
        }
    }

    void render() override {
        for (auto& segment : this->body) {
            DrawRectangle(
                segment.x,
                segment.y,
                BLOCK_SIZE,
                BLOCK_SIZE,
                RED);
        }
    }

    void update(float dt) override {
        if (IsKeyDown(KEY_RIGHT)) {
            this->rotation += SNAKE_ROTATION;
        } else if (IsKeyDown(KEY_LEFT)) {
            this->rotation -= SNAKE_ROTATION;
        }

        bool boost = false;
        if (IsKeyDown(KEY_UP) && boostRemaining > 5) {
            boost = true;
            boostRemaining -= 5;
            if (boostRemaining < 0) {
                boostRemaining = 0;
            }
        } else {
            boostRemaining++;
            if (boostRemaining > MAX_BOOST) {
                boostRemaining = MAX_BOOST;
            }
        }

        if (IsKeyPressed(KEY_G)) {
            this->grow();
        }

        Vector2 velocity = {
            SNAKE_MOVE_SPEED * std::cos(this->rotation),
            SNAKE_MOVE_SPEED * std::sin(this->rotation)
        };
        if (boost) {
            velocity.x *= 2.3f;
            velocity.y *= 2.3f;
        }
        Vector2 newHead = { 
            this->body.front().x + velocity.x * dt,
            this->body.front().y + velocity.y * dt
        };

        // Wrap around bounds
        if (newHead.x < 0) {
            newHead.x = WIDTH - BLOCK_SIZE;
        }
        if (newHead.x + BLOCK_SIZE > WIDTH) {
            newHead.x = 0;
        }
        if (newHead.y < 0) {
            newHead.y = HEIGHT - BLOCK_SIZE;
        }
        if (newHead.y + BLOCK_SIZE > HEIGHT) {
            newHead.y = 0;
        }

        // Add new head and pop tail
        this->body.insert(body.begin(), newHead);
        this->body.pop_back();
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
            BLOCK_SIZE,
            BLOCK_SIZE
        };
    }
};

class BoostUI : public GameObject {
public:
    void render() override {
        Color c;
        int boostPercent = 76 * boostRemaining / 100;
        if (boostPercent < 25) {
            c = RED;
        } else if (boostPercent < 50) {
            c = ORANGE;
        } else {
            c = GREEN;
        }

        DrawRectangleLines(WIDTH / 2 - 40, 5, 80, 9, BLACK);
        DrawRectangle((WIDTH / 2 - 40) + 2, 5 + 2, boostPercent, 5, c);

    }

    void update(float dt) override {}
};

class GameUI : public GameObject {
private:
    BoostUI boost;

public:
    void render() override {
        std::string scoreText = std::to_string(score);
        DrawText(scoreText.c_str(), 5, 5, 1, BLACK);

        boost.render();
    }

    void update(float dt) override {}
};

// scenes

class GameScene : public Scene { 
private:
    RenderTexture2D renderTexture = LoadRenderTexture(WIDTH, HEIGHT);
    RenderTexture2D uiTexture = LoadRenderTexture(WIDTH, HEIGHT);
    Snake snake = Snake({ WIDTH / 2, HEIGHT / 2 });
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
