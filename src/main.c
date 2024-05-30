// ------------------------------------------------------------------------------
// Simple Raylib Template
// https://github.com/itsYakub/Simple-Raylib-Template.git
// ------------------------------------------------------------------------------
// Author:
// https://github.com/itsYakub
// ------------------------------------------------------------------------------
// LICENCE (MIT):
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
// ------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "raylib.h"
#include "raymath.h"

// macro deffinitions
#define TEXT_FONT_SIZE 32

#define PLAYER_GRAVITY_X 0.0
#define PLAYER_GRAVITY_Y 24.0
#define PLAYER_SPEED 512.0f

#define COLLECTLIBLE_RADIUS 16.0f
#define COLLECTIBLE_SPAWN_CHANCE 4 // What's the chance in between 0 - COLLECTIBLE_SPAWN_CHANCE for this to happen
#define COLLECTIBLE_SPAWN_CHANCE_VALUE 0 // What's the exact value that must be picked by the 0 - COLLECTIBLE_SPAWN_CHANCE random number generation

#define OBSTACLE_CAPACITY 8 // The size of the Obstacle buffer (where all the obstacle objects are stored)
#define OBSTACLE_WIDTH 320
#define OBSTACLE_HEIGHT 512
#define OBSTACLE_DIST_INITIAL renderGetSize().y - 64.0f
#define OBSTACLE_DIST_REDUCTION 8
#define OBSTACLE_SHRINK_REVERSE_CHANCE 5 // What's the chance in between 0 - OBSTACLE_SHRINK_REVERSE_CHANCE for this to happen
#define OBSTACLE_SHRINK_REVERSE_CHANCE_VALUE 0 // What's the exact value that must be picked by the 0 - OBSTACLE_SHRINK_REVERSE_CHANCE_VALUE random number generation

#define MATH_MIN(a, b) { a < b ? a : b }

// Forward declarations
struct Player;
struct Collectible;
struct Obstacle;
struct ObstacleList;

typedef enum {
    STATE_START,
    STATE_GAMEPLAY,
    STATE_GAMEOVER,
    STATE_PAUSE,
    STATE_RESUME
} GameplayStateMachine;

typedef struct Player {
    Vector2 position;
    Vector2 position_prev;

    Vector2 velocity;
    Vector2 physical_size;
    
    Vector2 sprite_size;
    float sprite_rotation;

    uint32_t points;
    bool game_over; // You crash once - this value is set to true;
} Player;

Player playerInit(Vector2 position);
void playerUpdate();
void playerRender();

void playerSetPosition(Vector2 position);
void playerIncrementPosition(Vector2 incrementation);
void playerSetVelocity(Vector2 velocity);

void playerCheckCollisions();

typedef struct {
    Vector2 position;
} Collectible;


typedef struct Obstacle {
    // The general idea is as follows:
    // There're two points, 'point0' & 'point1', which are separated by the 'distance'.
    // Every time the new obstacle is created, from the position we substract the half of the distance to get the point0, and add the half of the distance to het the point1.
    // We use the 'position' variable to offset the obstacles on every new instantiation.
    Vector2 position;

    Vector2 point0;
    Vector2 point1;

    float distance;

    bool has_collectible;
    Collectible collectible;
} Obstacle;

Obstacle obstacleInit(Vector2 position, float distance);
void obstacleRender(Obstacle* obstacle);

void collectibleRender(Obstacle* obstacle);

typedef struct ObstacleList {
    Obstacle list[OBSTACLE_CAPACITY];

    int count;
    int index_of_first_obstacle;
    int index_of_last_obstacle;
} ObstacleList;

ObstacleList obstacleListInit();
void obstacleListUpdate();
void obstacleListRender();
void obstacleListLoopObstacles();

struct {
    struct {
        GameplayStateMachine gameplay_state_machine;
        RenderTexture2D render_texture;
    } Game;

    struct {
        Player player;
        Camera2D camera;
    } PlayerGlobals;

    struct {
        ObstacleList obstacle_list;
    } ObstacleGlobals;

    struct {
        bool render_data;
        bool render_colliders;
    } Debug;
} GlobalState;

const char* stateMachineGetName();
void stateMachineSet(GameplayStateMachine state_machine);

Vector2 renderGetSize();

void debugRender();
void debugRenderData();
void debugRenderCollisions();

int main(int argc, char** argv) {
    const char* TITLE = "Summer Jam 2024";
    const int WIDTH = 1280;
    const int HEIGHT = 768;

    ConfigFlags config_flags =
        FLAG_MSAA_4X_HINT | 
        FLAG_WINDOW_RESIZABLE |
        FLAG_VSYNC_HINT;

    SetConfigFlags(config_flags);

    // Initializing resources:
    // - main game window;
    // - game's audio device;
    InitWindow(WIDTH, HEIGHT, TextFormat("Raylib %s - %s", RAYLIB_VERSION, TITLE));
    InitAudioDevice();

    SetExitKey(KEY_NULL);

    GlobalState.Game.render_texture = LoadRenderTexture(WIDTH, HEIGHT);
    SetTextureFilter(GlobalState.Game.render_texture.texture, TEXTURE_FILTER_BILINEAR);

    GlobalState.Game.gameplay_state_machine = STATE_START;

    GlobalState.PlayerGlobals.player = playerInit(
        (Vector2) { 
            renderGetSize().x / 2.0f - 256.0f, 
            renderGetSize().y / 2.0f 
        }
    );

    GlobalState.PlayerGlobals.camera = (Camera2D) {
        .offset = { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f },
        .target = { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f},
        .zoom = 1.0f
    };

    GlobalState.ObstacleGlobals.obstacle_list = obstacleListInit();

    GlobalState.Debug.render_data = false;
    GlobalState.Debug.render_colliders = false;

    while(!WindowShouldClose()) {
        // Update your game logic here...

        // State-Independent update loop...
        if(IsKeyPressed(KEY_F3)) {
            GlobalState.Debug.render_data = !GlobalState.Debug.render_data;
            GlobalState.Debug.render_colliders = !GlobalState.Debug.render_colliders;
        }
        float scale = MATH_MIN(GetScreenWidth() / renderGetSize().x, GetScreenHeight() / renderGetSize().y);
        SetMouseOffset((GetScreenWidth() - (renderGetSize().x * scale)) * 0.5f * -1.0, (GetScreenHeight() - (renderGetSize().y * scale)) * 0.5f * -1.0);
        SetMouseScale(1 / scale, 1 / scale);

        // State-Dependent update loop...
        switch (GlobalState.Game.gameplay_state_machine) {
            case STATE_START: {

                if(IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    playerSetVelocity((Vector2) { 0.0f, -PLAYER_GRAVITY_Y * 16.0f * GetFrameTime()});
                    stateMachineSet(STATE_GAMEPLAY);
                }

            } break;

            case STATE_GAMEPLAY: {
                playerUpdate();
                playerCheckCollisions();
                obstacleListUpdate();
                GlobalState.PlayerGlobals.camera.target.x += PLAYER_SPEED * GetFrameTime();

                if(IsKeyPressed(KEY_ESCAPE)) {
                    stateMachineSet(STATE_PAUSE);
                }

                if(GlobalState.PlayerGlobals.player.game_over) {
                    stateMachineSet(STATE_GAMEOVER);
                }
            } break;

            case STATE_GAMEOVER: {

            } break;

            case STATE_PAUSE: {
                if(IsKeyPressed(KEY_ESCAPE)) {
                    stateMachineSet(STATE_RESUME);
                }
            } break;

            case STATE_RESUME: {
                stateMachineSet(STATE_GAMEPLAY);
            } break;
        }

        BeginTextureMode(GlobalState.Game.render_texture);
        ClearBackground(RAYWHITE);

        // Render your graphics here...

        // State-Independent rendering...
        BeginMode2D(GlobalState.PlayerGlobals.camera);

            playerRender();
            obstacleListRender();
            debugRenderCollisions();
        
        EndMode2D();

        debugRenderData();

        // State-dependent rendering...
        switch (GlobalState.Game.gameplay_state_machine) {
            case STATE_START: {
                const char* text0 = "Summer Jam 2024";
                const char* text1 = "Press SPACE or LBM to start";

                Vector2 text0_size = MeasureTextEx(GetFontDefault(), text0, TEXT_FONT_SIZE * 4, 2.0f);
                Vector2 text1_size = MeasureTextEx(GetFontDefault(), text1, TEXT_FONT_SIZE, 2.0f);

                DrawTextPro(
                    GetFontDefault(), 
                    text0, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f - 192}, 
                    Vector2Divide(text0_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE * 4, 
                    2.0f, 
                    BLACK
                );

                DrawTextPro(
                    GetFontDefault(), 
                    text1, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f + 128}, 
                    Vector2Divide(text1_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE, 
                    2.0f, 
                    Fade(BLACK, 0.5f)
                );
            } break;

            case STATE_GAMEPLAY: {

            } break;

            case STATE_GAMEOVER: {

            } break;

            case STATE_PAUSE: {

            } break;

            case STATE_RESUME: {
                
            } break;
        }
        
        EndTextureMode();

        BeginDrawing();

            ClearBackground(BLACK);

            DrawTexturePro(
                GlobalState.Game.render_texture.texture, 
                (Rectangle){
                    0.0f,
                    0.0f,
                    GlobalState.Game.render_texture.texture.width,
                    GlobalState.Game.render_texture.texture.height * -1.0f,
                }, 
                (Rectangle) { 
                    (GetScreenWidth() - (GlobalState.Game.render_texture.texture.width * scale)) * 0.5f, 
                    (GetScreenHeight() - (GlobalState.Game.render_texture.texture.height * scale)) * 0.5f,
                    GlobalState.Game.render_texture.texture.width * scale,
                    GlobalState.Game.render_texture.texture.height * scale,
                }, 
                Vector2Zero(), 
                0.0f, 
                WHITE
            );

        EndDrawing();
    }

    // Unloading resources...
    UnloadRenderTexture(GlobalState.Game.render_texture);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}

Player playerInit(Vector2 position) {
    Player result = {
        .position = position,
        .velocity = Vector2Zero(),
        .physical_size = { 48.0f, 48.0f },

        .sprite_size = { 64.0f, 64.0f },
        .sprite_rotation = 0.0f,

        .points = 0,
        .game_over = false
    };

    return result;
}

void playerUpdate() {
    // Firstly, we apply our physics forces ..
    GlobalState.PlayerGlobals.player.velocity = Vector2Add(
        GlobalState.PlayerGlobals.player.velocity, 
        (Vector2) { PLAYER_GRAVITY_X * GetFrameTime(), PLAYER_GRAVITY_Y * GetFrameTime() }
    );

    // ... (Don't forget to clamp it between the reasonabe bounds!) ...
    GlobalState.PlayerGlobals.player.velocity = Vector2Clamp(
        GlobalState.PlayerGlobals.player.velocity, 
        (Vector2) { PLAYER_GRAVITY_X * -4.0f, PLAYER_GRAVITY_Y * -4.0f}, 
        (Vector2) { PLAYER_GRAVITY_X * 4.0f, PLAYER_GRAVITY_Y * 4.0f }
    );

    // ... Then we can menage the general gameplay stuff!
    GlobalState.PlayerGlobals.player.velocity.x = PLAYER_SPEED * GetFrameTime();

    if(IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        GlobalState.PlayerGlobals.player.velocity.y -= PLAYER_GRAVITY_Y * 2.0f * GetFrameTime();
    }

    // Lastly, we apply all the forces to our position.
    playerIncrementPosition(GlobalState.PlayerGlobals.player.velocity);
}

void playerRender() {
    Player* player = &GlobalState.PlayerGlobals.player;

    player->sprite_rotation = Lerp(
        player->sprite_rotation,
        GlobalState.PlayerGlobals.player.velocity.y * 2.0f,
        20.0f * GetFrameTime()
    );

    DrawRectanglePro(
        (Rectangle) { 
            player->position.x,
            player->position.y,
            player->sprite_size.x,
            player->sprite_size.y
        }, 
        Vector2Divide(player->sprite_size, (Vector2) { 2.0f, 2.0f }), 
        player->sprite_rotation, 
        RED
    );
}

void playerSetPosition(Vector2 position) {
    GlobalState.PlayerGlobals.player.position_prev = GlobalState.PlayerGlobals.player.position;
    GlobalState.PlayerGlobals.player.position = position;
}

void playerIncrementPosition(Vector2 incrementation) {
    GlobalState.PlayerGlobals.player.position_prev = GlobalState.PlayerGlobals.player.position;
    GlobalState.PlayerGlobals.player.position = Vector2Add(GlobalState.PlayerGlobals.player.position, incrementation);
}

void playerSetVelocity(Vector2 velocity) {
    GlobalState.PlayerGlobals.player.velocity = velocity;
}

void playerCheckCollisions() {
    Player* player = &GlobalState.PlayerGlobals.player;
    const int OBSTACLES_INDEX_FIRST = GlobalState.ObstacleGlobals.obstacle_list.index_of_first_obstacle;

    for(int obstacles_to_check = 0; obstacles_to_check < OBSTACLE_CAPACITY; obstacles_to_check++) {
        Obstacle* obstacle = &GlobalState.ObstacleGlobals.obstacle_list.list[obstacles_to_check];

        Rectangle player_rect = { player->position.x - (player->physical_size.x / 2.0f), player->position.y - (player->physical_size.y / 2.0f), player->physical_size.x, player->physical_size.y };
        Rectangle point0_rect = { obstacle->point0.x - OBSTACLE_WIDTH / 2.0f, obstacle->point0.y - OBSTACLE_HEIGHT, OBSTACLE_WIDTH, OBSTACLE_HEIGHT };
        Rectangle point1_rect = { obstacle->point1.x - OBSTACLE_WIDTH / 2.0f, obstacle->point1.y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT };

        if(CheckCollisionRecs(player_rect, point0_rect) || CheckCollisionRecs(player_rect, point1_rect)) {
            player->game_over = true;
        }

        if(obstacle->has_collectible) {
            if(CheckCollisionCircleRec(obstacle->collectible.position, COLLECTLIBLE_RADIUS, player_rect)) {
                player->points++;
                obstacle->has_collectible = false;
            }
        }
    }
}

Obstacle obstacleInit(Vector2 position, float distance){
    Obstacle result = {
        .position = Vector2Clamp(
            position, 
            (Vector2) { position.x, distance / 2.0f },
            (Vector2) { position.x, renderGetSize().y - distance / 2.0f }
        ),

        .distance = distance,

        .point0.x = position.x,
        .point1.x = position.x,
    };

    result.point0.y = position.y - (distance / 2.0f);
    result.point1.y = position.y + (distance / 2.0f);
    
    // Simple check if there is a space for collectible to be spawned
    if(distance > COLLECTLIBLE_RADIUS * 2.0f) {
        // RNG that picks if the collectible will be spawned
        if(GetRandomValue(0, COLLECTIBLE_SPAWN_CHANCE) == COLLECTIBLE_SPAWN_CHANCE_VALUE) {
            result.has_collectible = true;
            result.collectible = (Collectible) { 
                (Vector2) { 
                    position.x - GetRandomValue(
                        (OBSTACLE_WIDTH / -2.0f) + (COLLECTLIBLE_RADIUS * 2.0f), 
                        (OBSTACLE_WIDTH / 2.0f) - (COLLECTLIBLE_RADIUS * 2.0f)
                    ),
                    // This formula either substracts or add a value, which is in between the point0 and point1 from the position.y value. It accounts the radius of the collectible
                    position.y - GetRandomValue(
                        (distance / -2.0f) + (COLLECTLIBLE_RADIUS * 2.0f), 
                        (distance / 2.0f) - (COLLECTLIBLE_RADIUS * 2.0f)
                    )
                }
            };
        } 
    }

    return result;
}

void obstacleRender(Obstacle* obstacle) {
    DrawRectanglePro(
        (Rectangle) {
            obstacle->point0.x,
            obstacle->point0.y,
            OBSTACLE_WIDTH,
            OBSTACLE_HEIGHT
        }, 
        (Vector2) { 
            OBSTACLE_WIDTH / 2.0f, 
            0.0f
        }, 
        180.0f, 
        GRAY
    );
    
    DrawRectanglePro(
        (Rectangle) {
            obstacle->point1.x,
            obstacle->point1.y,
            OBSTACLE_WIDTH,
            OBSTACLE_HEIGHT
        }, 
        (Vector2) { 
            OBSTACLE_WIDTH / 2.0f, 
            0.0f
        }, 
        0.0f, 
        DARKGRAY
    );

    collectibleRender(obstacle);

}

void collectibleRender(Obstacle* obstacle) {
    if(!obstacle || !obstacle->has_collectible) {
        return;
    }

    DrawCircleV(
        obstacle->collectible.position, 
        COLLECTLIBLE_RADIUS, 
        GOLD
    );

    DrawCircleLinesV(
        obstacle->collectible.position, 
        COLLECTLIBLE_RADIUS, 
        BLACK
    );
}

ObstacleList obstacleListInit() {
    ObstacleList result = { 0 };

    Vector2 obs_position = { 0.0f, renderGetSize().y / 2.0f };
    float distance = OBSTACLE_DIST_INITIAL;

    for(int obstacle_index = 0; obstacle_index < OBSTACLE_CAPACITY; obstacle_index++) {
        int obstacle_move_direction = 0;
        int obstacle_shrink = 1;

        // RNG that picks the horizontal direction that the next obstacle will be placed (1 -> up; -1 -> down)
        do {
            obstacle_move_direction = GetRandomValue(-1, 1);
        } while(obstacle_move_direction == 0);

        // RNG that picks if the next obstacle will be shrinked or extended
        if(GetRandomValue(0, OBSTACLE_SHRINK_REVERSE_CHANCE) == OBSTACLE_SHRINK_REVERSE_CHANCE_VALUE) {
            obstacle_shrink = -1; // The value -1 will extend the obstacle (make the distance bigger)
        }

        result.list[obstacle_index] = obstacleInit(obs_position, distance);

        obs_position.x += OBSTACLE_WIDTH;
        obs_position.y = result.list[obstacle_index].position.y + obstacle_move_direction * (OBSTACLE_DIST_REDUCTION * 2);

        if(distance >= 0.0f) {
            distance -= OBSTACLE_DIST_REDUCTION * obstacle_shrink;
        }

        obs_position.y = Clamp(obs_position.y, distance / 2.0f, renderGetSize().y - distance / 2.0f);
    }

    result.count = OBSTACLE_CAPACITY;
    result.index_of_first_obstacle = 0;
    result.index_of_last_obstacle = OBSTACLE_CAPACITY - 1;

    return result;
}

void obstacleListUpdate() {
    obstacleListLoopObstacles();
}

void obstacleListRender() {
    for(int obstacle_index = 0; obstacle_index < OBSTACLE_CAPACITY; obstacle_index++) {
        obstacleRender(&GlobalState.ObstacleGlobals.obstacle_list.list[obstacle_index]);
    }
}

void obstacleListLoopObstacles() {
    Obstacle* obstacle_last = &GlobalState.ObstacleGlobals.obstacle_list.list[GlobalState.ObstacleGlobals.obstacle_list.index_of_last_obstacle];
    Obstacle* obstacle_current = &GlobalState.ObstacleGlobals.obstacle_list.list[GlobalState.ObstacleGlobals.obstacle_list.index_of_first_obstacle];
        
    if(GetWorldToScreen2D(obstacle_current->position, GlobalState.PlayerGlobals.camera).x < -OBSTACLE_WIDTH) {
        int obstacle_move_direction = 0;
        int obstacle_shrink = 1;

        do {
            obstacle_move_direction = GetRandomValue(-1, 1);
        } while(obstacle_move_direction == 0);

        if(GetRandomValue(0, 10) == 0) {
            obstacle_shrink = -1; // The value -1 will reverse the shrink (make the distance bigger)
        }

        Vector2 position = Vector2Zero();
        float distance = 0.0f;

        position.x = obstacle_last->position.x + OBSTACLE_WIDTH;
        position.y = obstacle_last->position.y + obstacle_move_direction * (OBSTACLE_DIST_REDUCTION * 2);
        distance = obstacle_last->distance;

        if(distance >= 0.0f && distance <= OBSTACLE_DIST_INITIAL) {
            distance -= OBSTACLE_DIST_REDUCTION * obstacle_shrink;
        }

        *obstacle_current = obstacleInit(position, distance);
        
        GlobalState.ObstacleGlobals.obstacle_list.index_of_last_obstacle++;
        if(GlobalState.ObstacleGlobals.obstacle_list.index_of_last_obstacle >= OBSTACLE_CAPACITY) {
            GlobalState.ObstacleGlobals.obstacle_list.index_of_last_obstacle = 0;
        }

        GlobalState.ObstacleGlobals.obstacle_list.index_of_first_obstacle++;
        if(GlobalState.ObstacleGlobals.obstacle_list.index_of_first_obstacle >= OBSTACLE_CAPACITY) {
            GlobalState.ObstacleGlobals.obstacle_list.index_of_first_obstacle = 0;
        }
    }
}

const char* stateMachineGetName() {
    switch (GlobalState.Game.gameplay_state_machine) {
        case STATE_START: return "STATE_START";
        case STATE_GAMEPLAY: return "STATE_GAMEPLAY";
        case STATE_GAMEOVER: return "STATE_GAMEOVER";
        case STATE_PAUSE: return "STATE_PAUSE";
        case STATE_RESUME: return "STATE_RESUME";
    }
}

void stateMachineSet(GameplayStateMachine state_machine) {
    GlobalState.Game.gameplay_state_machine = state_machine;
}

Vector2 renderGetSize() {
    return (Vector2) {
        GlobalState.Game.render_texture.texture.width,
        GlobalState.Game.render_texture.texture.height
    };
}

void debugRender() {
    debugRenderData();
    debugRenderCollisions();
}

void debugRenderData() {
    if(!GlobalState.Debug.render_data) {
        return;
    }

    SetTextLineSpacing(TEXT_FONT_SIZE);
    DrawText(
        TextFormat(
            "Game:\n> FPS: %i\n> State: %s\n\nPlayer:\n> Position: x.%.1f, y.%.1f\n> Velocity: x.%.1f, y.%.1f\n> Alive: %s\n> Points: %i\n",
            GetFPS(),
            stateMachineGetName(),
            GlobalState.PlayerGlobals.player.position.x,
            GlobalState.PlayerGlobals.player.position.y,
            GlobalState.PlayerGlobals.player.velocity.x,
            GlobalState.PlayerGlobals.player.velocity.y,
            GlobalState.PlayerGlobals.player.game_over ? "false" : "true",
            GlobalState.PlayerGlobals.player.points
        ),
        4,
        4,
        TEXT_FONT_SIZE,
        DARKGREEN
    );
}

void debugRenderCollisions() {
    if(!GlobalState.Debug.render_colliders) {
        return;
    }

    for(int i = 0; i < OBSTACLE_CAPACITY; i++) {
        Obstacle* obstacle = &GlobalState.ObstacleGlobals.obstacle_list.list[i];

        Rectangle point0_rect = { obstacle->point0.x - OBSTACLE_WIDTH / 2.0f, obstacle->point0.y - OBSTACLE_HEIGHT, OBSTACLE_WIDTH, OBSTACLE_HEIGHT };
        Rectangle point1_rect = { obstacle->point1.x - OBSTACLE_WIDTH / 2.0f, obstacle->point1.y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT };

        DrawRectangleLinesEx(point0_rect, 1.0f, GREEN);
        DrawRectangleLinesEx(point1_rect, 1.0f, GREEN);

        if(obstacle->has_collectible) {
            DrawCircleLinesV(
                obstacle->collectible.position, 
                COLLECTLIBLE_RADIUS, 
                GREEN
            );
        }
    }

    Player* player = &GlobalState.PlayerGlobals.player;
    Rectangle player_rect = { player->position.x - (player->physical_size.x / 2.0f), player->position.y - (player->physical_size.y / 2.0f), player->physical_size.x, player->physical_size.y };
    DrawRectangleLinesEx(player_rect, 1.0f, GREEN);

}