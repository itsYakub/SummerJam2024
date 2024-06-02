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
#include "rlgl.h"

// macro deffinitions
#define GAME_BACKGROUND_COLOR 0x4d9be6ff
#define GAME_LINES_COLOR 0x2e222fff

#define TEXT_FONT_SIZE 32
#define TEXT_FONT_SPACING 4
#define TEXT_COLOR_DARK 0x2e222fff
#define TEXT_COLOR_LIGHT 0xffffffff 

#define PLAYER_GRAVITY_X 0.0
#define PLAYER_GRAVITY_Y 24.0
#define PLAYER_SPEED 512.0f

#define COLLECTLIBLE_RADIUS 16.0f
#define COLLECTIBLE_SPAWN_CHANCE 2 // What's the chance in between 0 - COLLECTIBLE_SPAWN_CHANCE for this to happen
#define COLLECTIBLE_SPAWN_CHANCE_VALUE 0 // What's the exact value that must be picked by the 0 - COLLECTIBLE_SPAWN_CHANCE random number generation

#define OBSTACLE_CAPACITY 8 // The size of the Obstacle buffer (where all the obstacle objects are stored)
#define OBSTACLE_WIDTH 512
#define OBSTACLE_DIST_INITIAL renderGetSize().y - 128.0f
#define OBSTACLE_DIST_REDUCTION 16
#define OBSTACLE_SHRINK_REVERSE_CHANCE 5 // What's the chance in between 0 - OBSTACLE_SHRINK_REVERSE_CHANCE for this to happen
#define OBSTACLE_SHRINK_REVERSE_CHANCE_VALUE 0 // What's the exact value that must be picked by the 0 - OBSTACLE_SHRINK_REVERSE_CHANCE_VALUE random number generation
#define OBSTACLE_UPPER_COLOR 0x7f708aff
#define OBSTACLE_LOWER_COLOR 0xf9c22bff

#define MATH_MIN(a, b) { a < b ? a : b }

#define internal static

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
    
    float sprite_rotation;

    uint32_t points;
    uint32_t collected_common;
    uint32_t collected_rare;
    uint32_t collected_legendary;
    bool game_over; // You crash once - this value is set to true;
} Player;

Player playerInit(Vector2 position);
void playerUpdate();
void playerRender();
void playerRenderScore(Vector2 position, Vector2 text_offset);
void playerUnload();

void playerSetPosition(Vector2 position);
void playerIncrementPosition(Vector2 incrementation);
void playerSetVelocity(Vector2 velocity);

void playerCheckCollisions();

typedef enum {
    COLLECTIBLE_COMMON = 0,
    COLLECTLIBLE_RARE,
    COLLECTLIBLE_LEGENDARY,
    COLLECTIBLE_RARITY_COUNT
} CollectibleRarity;

typedef struct {
    CollectibleRarity collectible_rarity;

    Texture2D* spirte;
    float sprite_rotation;

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

Obstacle obstacleInit(Vector2 position, float distance, bool spawn_collectible);

Collectible collectibleInit(Obstacle* obstacle);
void collectibleRender(Obstacle* obstacle);

typedef struct ObstacleList {
    Obstacle list[OBSTACLE_CAPACITY];
} ObstacleList;

ObstacleList obstacleListInit();
void obstacleInitData(Obstacle* obstacle, Vector2* position, float* distance);
void obstacleListUpdate();
void obstacleListRender();
void obstacleListLoopObstacles();

struct {
    struct {
        GameplayStateMachine gameplay_state_machine;
        RenderTexture2D render_texture;

        float gameplay_time;

        bool quit;
        bool start_key_held;
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

    struct {
        Texture2D player_sprite;
        Texture2D collectible_textures[3];
    } Resources;
} GlobalState;

const char* stateMachineGetName();
void stateMachineSet(GameplayStateMachine state_machine);

Vector2 renderGetSize();

void debugRender();
void debugRenderData();
void debugRenderCollisions();

void gameInit();

void resourcesLoad();
void resourcesUnload();

internal bool collisionCheckRectLine(Rectangle rect, Vector2 line_start, Vector2 line_end);
internal void renderDrawLineGradient(Vector2 start, Vector2 end, int thickness, Color a, Color b);

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

    resourcesLoad();
    gameInit();

    while(!WindowShouldClose() && !GlobalState.Game.quit) {
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
                GlobalState.Game.gameplay_time += GetFrameTime();

                if(IsKeyPressed(KEY_ESCAPE)) {
                    stateMachineSet(STATE_PAUSE);
                }

                if(GlobalState.PlayerGlobals.player.game_over) {
                    stateMachineSet(STATE_GAMEOVER);
                }
            } break;

            case STATE_GAMEOVER: {
                if(IsKeyPressed(KEY_ESCAPE)) {
                    GlobalState.Game.quit = true;
                }

                if(IsKeyPressed(KEY_ENTER)) {
                    gameInit();
                }
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
        ClearBackground(GetColor(GAME_BACKGROUND_COLOR));

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

                Vector2 text0_size = MeasureTextEx(GetFontDefault(), text0, TEXT_FONT_SIZE * 4, TEXT_FONT_SPACING);
                Vector2 text1_size = MeasureTextEx(GetFontDefault(), text1, TEXT_FONT_SIZE, TEXT_FONT_SPACING);

                DrawTextPro(
                    GetFontDefault(), 
                    text0, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f - 192}, 
                    Vector2Divide(text0_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE * 4, 
                    TEXT_FONT_SPACING, 
                    GetColor(TEXT_COLOR_DARK)
                );

                DrawTextPro(
                    GetFontDefault(), 
                    text1, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f + 128}, 
                    Vector2Divide(text1_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE, 
                    TEXT_FONT_SPACING, 
                    Fade(GetColor(TEXT_COLOR_DARK), 0.5f)
                );

            } break;

            case STATE_GAMEPLAY: {
                playerRenderScore(
                    (Vector2) { 
                        8.0f, 
                        renderGetSize().y - 240.0f 
                    }, 
                    (Vector2) { 
                        32.0f, 
                        16.0f 
                    }
                );

            } break;

            case STATE_GAMEOVER: {
                DrawRectangle(
                    0,
                    0,
                    renderGetSize().x,
                    renderGetSize().y,
                    Fade(BLACK, 0.5f)
                );

                const char* text0 = "Game Over!";
                const char* text1 = TextFormat("> Total Time: %.02fs\n> Total Score: %i", GlobalState.Game.gameplay_time, GlobalState.PlayerGlobals.player.points);
                const char* text2 = "Press ENTER to RESTART or ESCAPE to QUIT...";

                Vector2 text0_size = MeasureTextEx(GetFontDefault(), text0, TEXT_FONT_SIZE * 5, TEXT_FONT_SPACING);
                Vector2 text1_size = MeasureTextEx(GetFontDefault(), text1, TEXT_FONT_SIZE, TEXT_FONT_SPACING);
                Vector2 text2_size = MeasureTextEx(GetFontDefault(), text2, TEXT_FONT_SIZE, TEXT_FONT_SPACING);

                SetTextLineSpacing(TEXT_FONT_SIZE);

                DrawTextPro(
                    GetFontDefault(), 
                    text0, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f}, 
                    Vector2Divide(text0_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE * 5, 
                    TEXT_FONT_SPACING, 
                    GetColor(TEXT_COLOR_LIGHT)
                );

                DrawTextPro(
                    GetFontDefault(), 
                    text1, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f + text1_size.y}, 
                    Vector2Divide(text1_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE, 
                    TEXT_FONT_SPACING, 
                    Fade(GetColor(TEXT_COLOR_LIGHT), 0.8f)
                );

                DrawTextPro(
                    GetFontDefault(), 
                    text2, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f + 256.0f}, 
                    Vector2Divide(text2_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE, 
                    TEXT_FONT_SPACING, 
                    Fade(GetColor(TEXT_COLOR_LIGHT), 0.8f)
                );

            } break;

            case STATE_PAUSE: {
                DrawRectangle(
                    0,
                    0,
                    renderGetSize().x,
                    renderGetSize().y,
                    Fade(BLACK, 0.5f)
                );

                const char* text0 = "Paused!";
                const char* text1 = "Press ESCAPE to resume...";

                Vector2 text0_size = MeasureTextEx(GetFontDefault(), text0, TEXT_FONT_SIZE * 5, 2.0f);
                Vector2 text1_size = MeasureTextEx(GetFontDefault(), text1, TEXT_FONT_SIZE, 2.0f);

                DrawTextPro(
                    GetFontDefault(), 
                    text0, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f}, 
                    Vector2Divide(text0_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE * 5, 
                    TEXT_FONT_SPACING, 
                    GetColor(TEXT_COLOR_LIGHT)
                );

                DrawTextPro(
                    GetFontDefault(), 
                    text1, 
                    (Vector2) { renderGetSize().x / 2.0f, renderGetSize().y / 2.0f + 128}, 
                    Vector2Divide(text1_size, (Vector2) { 2.0f, 2.0f } ), 
                    0.0f, 
                    TEXT_FONT_SIZE, 
                    TEXT_FONT_SPACING, 
                    Fade(GetColor(TEXT_COLOR_LIGHT), 0.8f)
                );    

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
    resourcesUnload();
    UnloadRenderTexture(GlobalState.Game.render_texture);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}

Player playerInit(Vector2 position) {
    Player result = {
        .position = position,
        .velocity = Vector2Zero(),
        .physical_size = { 
            GlobalState.Resources.player_sprite.width / 2.0f,
            GlobalState.Resources.player_sprite.width / 2.0f,
        },

        .sprite_rotation = 0.0f,

        .points = 0,
        .collected_common = 0,
        .collected_rare = 0,
        .collected_legendary = 0,
        
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

    if((IsKeyReleased(KEY_SPACE) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) && GlobalState.Game.start_key_held) {
        GlobalState.Game.start_key_held = false;
    }

    if((IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && !GlobalState.Game.start_key_held) {
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

    DrawTexturePro(
        GlobalState.Resources.player_sprite, 
        (Rectangle) {
            0.0f,
            0.0f,
            GlobalState.Resources.player_sprite.width,
            GlobalState.Resources.player_sprite.height
        }, 
        (Rectangle) {
            player->position.x,
            player->position.y,
            GlobalState.Resources.player_sprite.width,
            GlobalState.Resources.player_sprite.height
        }, 
        Vector2Divide((Vector2) { GlobalState.Resources.player_sprite.width, GlobalState.Resources.player_sprite.height }, (Vector2) { 2.0f, 2.0f }), 
        player->sprite_rotation, 
        WHITE
    );
}

void playerRenderScore(Vector2 position, Vector2 text_offset) {
    Player* player = &GlobalState.PlayerGlobals.player;
    int sprite_width = GlobalState.Resources.collectible_textures[0].width + text_offset.x;
    int sprite_height = GlobalState.Resources.collectible_textures[0].height + text_offset.y;

    // rendering all the sprites in the column
    for(int i = 0; i < 3; i++) {
        DrawTexturePro(
            GlobalState.Resources.collectible_textures[i], 
            (Rectangle) {
                0.0f,
                0.0f,
                GlobalState.Resources.collectible_textures[0].width,
                GlobalState.Resources.collectible_textures[0].height
            }, 
            (Rectangle) {
                position.x,
                position.y + sprite_height * i,
                GlobalState.Resources.collectible_textures[0].width,
                GlobalState.Resources.collectible_textures[0].height
            }, 
            Vector2Zero(), 
            0.0f, 
            WHITE
        );
    }

    SetTextLineSpacing(sprite_height);

    DrawText(
        TextFormat("%u\n%u\n%u", player->collected_common, player->collected_rare, player->collected_legendary), 
        position.x + sprite_width, 
        position.y, 
        sprite_height, 
        GetColor(TEXT_COLOR_LIGHT)
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

    for(int obstacles_to_check = 0; obstacles_to_check < OBSTACLE_CAPACITY - 1; obstacles_to_check++) {
        Obstacle* obstacle = &GlobalState.ObstacleGlobals.obstacle_list.list[obstacles_to_check];
        Obstacle* obstacle_next = &GlobalState.ObstacleGlobals.obstacle_list.list[obstacles_to_check + 1];

        Rectangle player_rect = { 
            player->position.x - (player->physical_size.x / 2.0f), 
            player->position.y - (player->physical_size.y / 2.0f), 
            player->physical_size.x, 
            player->physical_size.y 
        };

        if(collisionCheckRectLine(player_rect, obstacle->point0, obstacle_next->point0) || collisionCheckRectLine(player_rect, obstacle->point1, obstacle_next->point1)) {
            player->game_over = true;
        }

        if(obstacle->has_collectible) {
            if(CheckCollisionCircleRec(obstacle->collectible.position, COLLECTLIBLE_RADIUS, player_rect)) {
                switch (obstacle->collectible.collectible_rarity) {
                    case COLLECTIBLE_COMMON:            player->points++;       player->collected_common++;     break;
                    case COLLECTLIBLE_RARE:             player->points += 2;    player->collected_rare++;       break;
                    case COLLECTLIBLE_LEGENDARY:        player->points += 4;    player->collected_legendary++;  break;
                    case COLLECTIBLE_RARITY_COUNT:      player->points += 0;                                    break;
                    default:                            player->points += 0;                                    break;
                }

                obstacle->has_collectible = false;
            }
        }
    }
}

Obstacle obstacleInit(Vector2 position, float distance, bool spawn_collectible){
    Obstacle result = {
        .position = position,
        .distance = distance,

        .point0.x = position.x,
        .point1.x = position.x,
    };

    result.point0.y = position.y - (distance / 2.0f);
    result.point1.y = position.y + (distance / 2.0f);
    
    // Simple check if there is a space for collectible to be spawned
    if(distance > COLLECTLIBLE_RADIUS * 2.0f && spawn_collectible) {
        // RNG that picks if the collectible will be spawned
        if(GetRandomValue(0, COLLECTIBLE_SPAWN_CHANCE) == COLLECTIBLE_SPAWN_CHANCE_VALUE) {
            result.has_collectible = true;
            result.collectible = collectibleInit(&result);
        } 
    }

    return result;
}

Collectible collectibleInit(Obstacle* obstacle) {
    Collectible result = (Collectible) { 
        .position = (Vector2) { 
            obstacle->position.x - GetRandomValue(
                (OBSTACLE_WIDTH / -2.0f) + (COLLECTLIBLE_RADIUS * 2.0f), 
                (OBSTACLE_WIDTH / 2.0f) - (COLLECTLIBLE_RADIUS * 2.0f)
            ),
            // This formula either substracts or add a value, which is in between the point0 and point1 from the position.y value. It accounts the radius of the collectible
            obstacle->position.y - GetRandomValue(
                (obstacle->distance / -2.0f) + (COLLECTLIBLE_RADIUS * 2.0f), 
                (obstacle->distance / 2.0f) - (COLLECTLIBLE_RADIUS * 2.0f)
            )
        },
        .sprite_rotation = GetRandomValue(-30, 30)
    };

    // 'collectible_rarity_random_index' picks the random value...
    int collectible_rarity_random_index = GetRandomValue(0, 30);
    // .. which then helps us assign the proper rarity to our collectible.

    if(collectible_rarity_random_index >= 0 && collectible_rarity_random_index < 11) { // If 'collectible_rarity_random_index' is in range 0 - 10, then the rarity is COLLECTIBLE_COMMON (33%)
        result.collectible_rarity = COLLECTIBLE_COMMON;
    } else if(collectible_rarity_random_index >= 11 && collectible_rarity_random_index < 21) { // Otherwise, if 'collectible_rarity_random_index' is in range 11 - 20, then the rarity is COLLECTLIBLE_RARE (33%)
        result.collectible_rarity = COLLECTLIBLE_RARE;
    } else if(collectible_rarity_random_index >= 21 && collectible_rarity_random_index < 31) { // lastly, if 'collectible_rarity_random_index' is in range 21 - 30, then the rarity is COLLECTLIBLE_LEGENDARY (33%)
        result.collectible_rarity = COLLECTLIBLE_LEGENDARY;
    }

    result.spirte = &GlobalState.Resources.collectible_textures[result.collectible_rarity];

    return result;
}

void collectibleRender(Obstacle* obstacle) {
    if(!obstacle || !obstacle->has_collectible) {
        return;
    }

    DrawTexturePro(
        *obstacle->collectible.spirte,
        (Rectangle) {
            0,
            0,
            obstacle->collectible.spirte->width,
            obstacle->collectible.spirte->height
        },
        (Rectangle) {
            obstacle->collectible.position.x,
            obstacle->collectible.position.y,
            COLLECTLIBLE_RADIUS * 2,
            COLLECTLIBLE_RADIUS * 2
        },
        (Vector2) { COLLECTLIBLE_RADIUS, COLLECTLIBLE_RADIUS },
        obstacle->collectible.sprite_rotation,
        WHITE
    );

}

ObstacleList obstacleListInit() {
    ObstacleList result = { 0 };

    Vector2 obstacle_position = { 0.0f, renderGetSize().y / 2.0f };
    float obstacle_distance = OBSTACLE_DIST_INITIAL;

    result.list[0] = obstacleInit(obstacle_position, obstacle_distance, false);

    for(int obstacle_index = 1; obstacle_index < OBSTACLE_CAPACITY; obstacle_index++) {
        int obstacle_move_direction = 0;

        // RNG that picks the horizontal direction that the next obstacle will be placed (1 -> up; -1 -> down)
        do {
            obstacle_move_direction = GetRandomValue(-1, 1);
        } while(obstacle_move_direction == 0);

        obstacle_position.x += OBSTACLE_WIDTH;
        obstacle_position.y = result.list[obstacle_index - 1].position.y + obstacle_move_direction * (OBSTACLE_DIST_REDUCTION * 2);
        obstacle_position.y = Clamp(obstacle_position.y, obstacle_distance / 2.0f + 32.0f, renderGetSize().y - obstacle_distance / 2.0f - 32.0f);

        obstacle_distance = obstacle_distance > 0.0f && obstacle_distance < OBSTACLE_DIST_INITIAL ?
            obstacle_distance - OBSTACLE_DIST_REDUCTION :
            obstacle_distance;

        result.list[obstacle_index] = obstacleInit(obstacle_position, obstacle_distance, obstacle_index >= OBSTACLE_CAPACITY / 2);
    }

    return result;
}

void obstacleInitData(Obstacle* obstacle, Vector2* position, float* distance) {
    int obstacle_move_direction = 0;

    // RNG that picks the horizontal direction that the next obstacle will be placed (1 -> up; -1 -> down)
    do {
        obstacle_move_direction = GetRandomValue(-1, 1);
    } while(obstacle_move_direction == 0);

    *position = (Vector2) {
        obstacle->position.x + OBSTACLE_WIDTH,
        obstacle->position.y + obstacle_move_direction * (OBSTACLE_DIST_REDUCTION * 2)
    };

    *distance = obstacle->distance >= 0.0f && obstacle->distance <= OBSTACLE_DIST_INITIAL ?
        obstacle->distance - OBSTACLE_DIST_REDUCTION :
        obstacle->distance;

    position->y = Clamp(position->y, *distance / 2.0f + 32.0f, renderGetSize().y - *distance / 2.0f - 32.0f);
}

void obstacleListUpdate() {
    obstacleListLoopObstacles();
}

void obstacleListRender() {
    const int LINE_THICKNESS = 4;

    for(int obstacle_index = 0; obstacle_index < OBSTACLE_CAPACITY - 1; obstacle_index++) {
        Obstacle* obstacle_current = &GlobalState.ObstacleGlobals.obstacle_list.list[obstacle_index];
        Obstacle* obstacle_next = &GlobalState.ObstacleGlobals.obstacle_list.list[obstacle_index + 1];
        

        Vector2 points0[4] = {
            obstacle_current->point0,
            obstacle_next->point0,
            (Vector2) { obstacle_current->point0.x + OBSTACLE_WIDTH / 2.0f, obstacle_current->point0.y },
            (Vector2) { obstacle_next->point0.x - OBSTACLE_WIDTH / 2.0f, obstacle_next->point0.y }
        };

        Vector2 points1[4] = {
            obstacle_current->point1,
            obstacle_next->point1,
            (Vector2) { obstacle_current->point1.x + OBSTACLE_WIDTH / 2.0f, obstacle_current->point1.y },
            (Vector2) { obstacle_next->point1.x - OBSTACLE_WIDTH / 2.0f, obstacle_next->point1.y }
        };

        for(int points0_index = 0; points0_index < OBSTACLE_WIDTH / 2.0f; points0_index++) {
            Vector2 spline_point = GetSplinePointBezierCubic(
                points0[0], 
                points0[2], 
                points0[3], 
                points0[1], 
                points0_index / (OBSTACLE_WIDTH / 2.0f)
            );

            /*
        
            DrawLineEx(
                spline_point, 
                (Vector2) { spline_point.x, 0.0f }, 
                4.0f, 
                GetColor(OBSTACLE_UPPER_COLOR)
            );
        
            */

            DrawRectangleGradientV(
                spline_point.x, 
                0.0f, 
                LINE_THICKNESS, 
                Vector2Distance(
                    spline_point, 
                    (Vector2) { 
                        spline_point.x, 
                        0.0f
                    }
                ), 
                GetColor(0x3e3546ff), 
                GetColor(OBSTACLE_UPPER_COLOR)
            );
        }

        for(int points1_index = 0; points1_index < OBSTACLE_WIDTH / 2.0f; points1_index++) {
            Vector2 spline_point = GetSplinePointBezierCubic(
                points1[0], 
                points1[2], 
                points1[3], 
                points1[1], 
                points1_index / (OBSTACLE_WIDTH / 2.0f)
            );

            /*

            DrawLineEx(
                spline_point, 
                (Vector2) { spline_point.x, renderGetSize().y }, 
                4.0f, 
                GetColor(OBSTACLE_LOWER_COLOR)
            );
                        
            */


            DrawRectangleGradientV(
                spline_point.x, 
                spline_point.y, 
                LINE_THICKNESS, 
                Vector2Distance(
                    spline_point, 
                    (Vector2) { 
                        spline_point.x, 
                        renderGetSize().y + 1
                    }
                ), 
                GetColor(OBSTACLE_LOWER_COLOR),
                GetColor(0xf79617ff) 
            );
        }

        DrawSplineSegmentBezierCubic(
            points0[0], 
            points0[2], 
            points0[3], 
            points0[1], 
            LINE_THICKNESS,
            GetColor(GAME_LINES_COLOR)    
        );

        DrawSplineSegmentBezierCubic(
            points1[0], 
            points1[2], 
            points1[3], 
            points1[1], 
            LINE_THICKNESS,
            GetColor(GAME_LINES_COLOR)
        );

        collectibleRender(obstacle_current);
    }
}

void obstacleListLoopObstacles() {
    ObstacleList* obstacle_list = &GlobalState.ObstacleGlobals.obstacle_list;
    Obstacle* obstacle_current = &obstacle_list->list[0];
        
    if(GetWorldToScreen2D(obstacle_current->position, GlobalState.PlayerGlobals.camera).x < -OBSTACLE_WIDTH) {
        for(int i = 0; i < OBSTACLE_CAPACITY - 1; i++) {
            obstacle_list->list[i] = obstacle_list->list[i + 1];
        }

        obstacle_current = &obstacle_list->list[OBSTACLE_CAPACITY - 1];

        Vector2 obstacle_position = { 0 };
        float obstacle_distance = 0.0f;

        obstacleInitData(&obstacle_list->list[OBSTACLE_CAPACITY - 2], &obstacle_position, &obstacle_distance);

        *obstacle_current = obstacleInit(
            obstacle_position, 
            obstacle_distance, 
            true
        );
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
            "Game:\n> FPS: %i\n> State: %s\n> Time: %.02fs\n\nPlayer:\n> Position: x.%.1f, y.%.1f\n> Velocity: x.%.1f, y.%.1f\n> Alive: %s\n> Points: %i\n",
            GetFPS(),
            stateMachineGetName(),
            GlobalState.Game.gameplay_time,

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

    for(int i = 0; i < OBSTACLE_CAPACITY - 1; i++) {
        Obstacle* obstacle = &GlobalState.ObstacleGlobals.obstacle_list.list[i];
        Obstacle* obstacle_next = &GlobalState.ObstacleGlobals.obstacle_list.list[i + 1];

        Rectangle point0_rect = { 
            obstacle->point0.x - OBSTACLE_WIDTH / 2.0f, 
            obstacle->point0.y - Vector2Distance(obstacle->point0, (Vector2) { obstacle->point0.x, 0.0f } ), 
            OBSTACLE_WIDTH, 
            Vector2Distance(obstacle->point0, (Vector2) { obstacle->point0.x, 0.0f }) 
        };

        Rectangle point1_rect = { 
            obstacle->point1.x - OBSTACLE_WIDTH / 2.0f, 
            obstacle->point1.y, 
            OBSTACLE_WIDTH, 
            Vector2Distance(obstacle->point1, (Vector2) { obstacle->point1.x, renderGetSize().y }) 
        };

        DrawLineEx(obstacle->point0, obstacle_next->point0, 1.0f, GREEN);
        DrawLineEx(obstacle->point1, obstacle_next->point1, 1.0f, GREEN);

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

void gameInit() {
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

    GlobalState.Game.gameplay_time = 0.0f;
    GlobalState.Game.quit = false;
    GlobalState.Game.start_key_held = true;
}

void resourcesLoad(){
    // player resources
    GlobalState.Resources.player_sprite = LoadTexture("../res/graphics/player_sprite.png");

    // collectible resources
    GlobalState.Resources.collectible_textures[0] = LoadTexture("../res/graphics/collectible_common.png");
    GlobalState.Resources.collectible_textures[1] = LoadTexture("../res/graphics/collectible_rare.png");
    GlobalState.Resources.collectible_textures[2] = LoadTexture("../res/graphics/collectible_legendary.png");

    SetTextureFilter(GlobalState.Resources.player_sprite, TEXTURE_FILTER_BILINEAR);

    SetTextureFilter(GlobalState.Resources.collectible_textures[0], TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(GlobalState.Resources.collectible_textures[1], TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(GlobalState.Resources.collectible_textures[2], TEXTURE_FILTER_BILINEAR);
}

void resourcesUnload() {
    // unloading player resources
    UnloadTexture(GlobalState.Resources.player_sprite);

    // unloading collectible resources
    for(int i = 0; i < 3; i++) {
        UnloadTexture(GlobalState.Resources.collectible_textures[i]);
    }
}

internal bool collisionCheckRectLine(Rectangle rect, Vector2 line_start, Vector2 line_end) {
    // Source: https://www.jeffreythompson.org/collision-detection/line-rect.php

    Vector2 collision_points = Vector2Zero();

    bool collision_rect_up = CheckCollisionLines(
        (Vector2) {
            rect.x,
            rect.y
        }, 
        (Vector2) {
            rect.x + rect.width,
            rect.y
        },
        line_start, 
        line_end, 
        &collision_points
    );    

    bool collision_rect_down = CheckCollisionLines(
        (Vector2) {
            rect.x,
            rect.y + rect.height
        }, 
        (Vector2) {
            rect.x + rect.width,
            rect.y + rect.height
        },
        line_start, 
        line_end, 
        &collision_points
    );    

    bool collision_rect_left = CheckCollisionLines(
        (Vector2) {
            rect.x,
            rect.y
        }, 
        (Vector2) {
            rect.x,
            rect.y + rect.height
        },
        line_start, 
        line_end, 
        &collision_points
    );    

    bool collision_rect_right = CheckCollisionLines(
        (Vector2) {
            rect.x + rect.width,
            rect.y
        }, 
        (Vector2) {
            rect.x + rect.width,
            rect.y + rect.height
        },
        line_start, 
        line_end, 
        &collision_points
    );

    return collision_rect_up || collision_rect_down || collision_rect_left || collision_rect_right;
}

internal void renderDrawLineGradient(Vector2 start, Vector2 end, int thickness, Color a, Color b) {
    for(int i = 0; i < thickness; i++) {
        rlBegin(RL_QUADS);

            rlNormal3f(0.0f, 0.0f, 1.0f);

            rlColor4ub(a.r, a.g, a.b, a.a);
            rlVertex2f(start.x + i, start.y);

            rlColor4ub(b.r, b.g, b.b, b.a);
            rlVertex2f(end.x + i, end.y);

        rlEnd();

    }
}