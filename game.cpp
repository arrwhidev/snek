#include "raylib.h"
#include <vector>
#include <iostream>
#include <format>
#include <string>

// globals

const int width = 320;
const int height = 240;
const int blockWidth = 10;
const int blockHeight = 10;
const int scalingFactor = 3;

const int DIRECTION_UP = 0;
const int DIRECTION_DOWN = 1;
const int DIRECTION_LEFT = 2;
const int DIRECTION_RIGHT = 3;

int score = 0;
Shader bloomShader;

class GameObject {
public:
    virtual void render() = 0;
    virtual void update() = 0;
};

class UI : public GameObject {
public:
    void render() override {
        std::string scoreText = std::format("score: {}", score);
        DrawText(scoreText.c_str(), 10, 10, 10, BLACK);
    }

    void update() override {}
};

class Food : public GameObject {
private:
    Rectangle rect;

public:
    Food() {
        this->rect = { 30, 30, blockWidth, blockHeight };
    }

    void render() override {
        // BeginShaderMode(bloomShader);
        DrawRectangleRec(this->rect, BLUE);
        // EndShaderMode();
    }

    void update() override {}

    void eaten() {
        this->rect.x = GetRandomValue(0, width);
        this->rect.y = GetRandomValue(0, height);
    }

    Rectangle getRect() {
        return this->rect;
    }
};

class Snake : public GameObject {
private:
    int direction;
    std::vector<Vector2> body;
    
public:
    Snake(Vector2 start, int direction) {
        body.push_back(start);
        body.push_back({start.x + (blockWidth * 1), start.y});
        body.push_back({start.x + (blockWidth * 2), start.y});
        body.push_back({start.x + (blockWidth * 3), start.y});
        body.push_back({start.x + (blockWidth * 4), start.y});

        this->direction = direction;
    }

    void render() override {
        for (auto& segment : this->body) {
            DrawRectangle(
                segment.x,
                segment.y,
                blockWidth,
                blockHeight,
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

    void update() override {
        this->direction = getDirectionFromInput();

        Vector2 newHead = { 
            this->body[0].x,
            this->body[0].y
        };

        switch (this->direction)
        {
            case DIRECTION_UP:
                newHead.y -= blockHeight;
                break;
            case DIRECTION_DOWN:
                newHead.y += blockHeight;
                break;
            case DIRECTION_LEFT:
                newHead.x -= blockWidth;
                break;
            case DIRECTION_RIGHT:
                newHead.x += blockWidth;
                break;
            default:
                break;
        }

        // check bounds
        if (this->body.front().x < 0) {
            newHead.x = width - blockWidth;
        }
        if (this->body.front().x > width) {
            newHead.x = 0;
        }
        if (this->body.front().y < 0) {
            newHead.y = height - blockWidth;
        }
        if (this->body.front().y > height) {
            newHead.y = 0;
        }

        // add new head & remove tail
        this->body.insert(body.begin(), newHead);
        this->body.pop_back();
    }

    void grow() {
        Vector2 tail = this->body.back();
        switch (this->direction)
        {
            case DIRECTION_UP:
                this->body.push_back({tail.x, tail.y + blockHeight});
                break;
            case DIRECTION_DOWN:
                this->body.push_back({tail.x, tail.y - blockHeight});
                break;
            case DIRECTION_LEFT:
                this->body.push_back({tail.x + blockWidth, tail.y});
                break;
            case DIRECTION_RIGHT:
                this->body.push_back({tail.x - blockHeight, tail.y});
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
            blockWidth,
            blockHeight
        };
    }
};

int main(void) {
    InitWindow(width * scalingFactor, height * scalingFactor, "snek");
    SetTargetFPS(30);
    bloomShader = LoadShader(0, TextFormat("bloom.fs", 330));

    Snake snek({ width / 2, height / 2 }, DIRECTION_LEFT);
    Food food;
    UI ui;

    Camera2D camera = { 0 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    RenderTexture2D renderTexture = LoadRenderTexture(width, height);
    while (!WindowShouldClose()) {
        /**
         * Update 
         */

        snek.update();
        food.update();

        // check collisions
        if (CheckCollisionRecs(snek.getHead(), food.getRect())) {
            food.eaten();
            snek.grow();
            score++;
        }

        // TODO: add snake collide with itself

        /**
         * Render
         */

        // Render to texture at internal resolution
        BeginTextureMode(renderTexture);
        ClearBackground(PINK);
        BeginMode2D(camera);
        // BeginShaderMode(bloomShader);
            snek.render();
            food.render();
        // EndShaderMode();
        ui.render();
        EndTextureMode();

        // Render the texture and scale it
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTexturePro(renderTexture.texture,
            { 0.0f, 0.0f, (float)renderTexture.texture.width, (float)-renderTexture.texture.height },
            { 0.0f, 0.0f, (float)renderTexture.texture.width * scalingFactor, (float)renderTexture.texture.height * scalingFactor },
            { 0.0f, 0.0f },
            0.0f,
            WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
