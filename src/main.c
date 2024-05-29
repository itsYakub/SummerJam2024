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

#include "raylib.h"
#include "raymath.h"

#define PLAYER_GRAVITY_X 0.0
#define PLAYER_GRAVITY_Y 24.0
#define PLAYER_SPEED 512.0f

#define OBSTACLE_CAPACITY 8
#define OBSTACLE_WIDTH 320
#define OBSTACLE_HEIGHT 512
#define OBSTACLE_DIST_REDUCTION 4

struct Player;
struct Obstacle;
struct ObstacleList;

typedef enum {
    STATE_START,
    STATE_GAMEPLAY,
    STATE_GAMEOVER
} GameplayStateMachine;

typedef struct Player {
    Vector2 position;
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
void playerSetVelocity(Vector2 velocity);

void playerCheckCollisions();

typedef struct Obstacle {
    // The general idea is as follows:
    // There're two points, 'point0' & 'point1', which are separated by the 'distance'.
    // Every time the new obstacle is created, from the position we substract the half of the distance to get the point0, and add the half of the distance to het the point1.
    // We use the 'position' variable to offset the obstacles on every new instantiation.
    Vector2 position;

    Vector2 point0;
    Vector2 point1;

    float distance;
} Obstacle;

Obstacle obstacleInit(Vector2 position, float distance);
void obstacleRender(Obstacle* obstacle);

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
    } StateMachineGlobals;

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

    GlobalState.StateMachineGlobals.gameplay_state_machine = STATE_START;

    GlobalState.PlayerGlobals.player = playerInit(
        (Vector2) { 
            GetScreenWidth() / 2.0f - 256.0f, 
            GetScreenHeight() / 2.0f 
        }
    );

    GlobalState.PlayerGlobals.camera = (Camera2D) {
        .offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f },
        .target = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f },
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

        // State-Dependent update loop...
        switch (GlobalState.StateMachineGlobals.gameplay_state_machine) {
            case STATE_START: {

                if(IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    playerSetVelocity((Vector2) { 0.0f, -PLAYER_GRAVITY_Y * 16.0f * GetFrameTime()});
                    GlobalState.StateMachineGlobals.gameplay_state_machine = STATE_GAMEPLAY;
                }

            } break;

            case STATE_GAMEPLAY: {
                playerUpdate();
                playerCheckCollisions();
                obstacleListUpdate();
                GlobalState.PlayerGlobals.camera.target.x += PLAYER_SPEED * GetFrameTime();

                if(GlobalState.PlayerGlobals.player.game_over) {
                    GlobalState.StateMachineGlobals.gameplay_state_machine = STATE_GAMEOVER;
                }
            } break;

            case STATE_GAMEOVER: {

            } break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Render your graphics here...

        BeginMode2D(GlobalState.PlayerGlobals.camera);

            playerRender();
            obstacleListRender();
            debugRenderCollisions();
        
        EndMode2D();
        debugRenderData();

        EndDrawing();
    }

    // Unloading resources...
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
    GlobalState.PlayerGlobals.player.velocity = Vector2Add(GlobalState.PlayerGlobals.player.velocity, (Vector2) { PLAYER_GRAVITY_X * GetFrameTime(), PLAYER_GRAVITY_Y * GetFrameTime() });

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
    GlobalState.PlayerGlobals.player.position = Vector2Add(GlobalState.PlayerGlobals.player.position, GlobalState.PlayerGlobals.player.velocity);
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
    GlobalState.PlayerGlobals.player.position = position;
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
    }
}

Obstacle obstacleInit(Vector2 position, float distance){
    Obstacle result = {
        .position = position,
        .distance = distance,

        .point0.x = position.x,
        .point1.x = position.x,
    };

    result.point0.y = position.y - (distance / 2.0f);
    result.point1.y = position.y + (distance / 2.0f);

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

}

ObstacleList obstacleListInit() {
    ObstacleList result = { 0 };

    Vector2 obs_position = { 0.0f, GetScreenHeight() / 2.0f };
    float distance = 640.0f;

    for(int obstacle_index = 0; obstacle_index < OBSTACLE_CAPACITY; obstacle_index++) {
        int obstacle_move_direction = 0;

        do {
            obstacle_move_direction = GetRandomValue(-1, 1);
        } while(obstacle_move_direction == 0);

        result.list[obstacle_index] = obstacleInit(obs_position, distance);

        obs_position.x += OBSTACLE_WIDTH;
        obs_position.y = result.list[obstacle_index].position.y + obstacle_move_direction * (OBSTACLE_DIST_REDUCTION * 2);

        if(distance >= 0.0f) {
            distance -= OBSTACLE_DIST_REDUCTION;
        }

        obs_position.y = Clamp(obs_position.y, distance / 2.0f, GetScreenHeight() - distance / 2.0f);
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

        do {
            obstacle_move_direction = GetRandomValue(-1, 1);
        } while(obstacle_move_direction == 0);

        Vector2 position = Vector2Zero();
        float distance = 0.0f;

        position.x = obstacle_last->position.x + OBSTACLE_WIDTH;
        position.y = obstacle_last->position.y + obstacle_move_direction * (OBSTACLE_DIST_REDUCTION * 2);
        distance = obstacle_last->distance;

        if(distance >= 0.0f) {
            distance -= OBSTACLE_DIST_REDUCTION;
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

void debugRender() {
    debugRenderData();
    debugRenderCollisions();
}

void debugRenderData() {
    if(!GlobalState.Debug.render_data) {
        return;
    }

    DrawText(
        TextFormat(
            "FPS: %i\nAlive: %s\nPoints: %i",
            GetFPS(),
            GlobalState.PlayerGlobals.player.game_over ? "false" : "true",
            GlobalState.PlayerGlobals.player.points
        ),
        0,
        0,
        16,
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
    }

    Player* player = &GlobalState.PlayerGlobals.player;
    Rectangle player_rect = { player->position.x - (player->physical_size.x / 2.0f), player->position.y - (player->physical_size.y / 2.0f), player->physical_size.x, player->physical_size.y };
    DrawRectangleLinesEx(player_rect, 1.0f, GREEN);

}