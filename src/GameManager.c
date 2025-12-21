#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>
#include <string.h>

#include "GameManager.h"
#include "RenderMaster/AudioManager.h"

int16_t EncodeDelta(int8_t dx, int8_t dy)
{
    return (int16_t)((dy << 8) | ((uint8_t)dx));
}

uint16_t EncodePosition(int x, int y)
{
    return (y << 8) + x;
}

int8_t DecodeDX(int16_t encoded)
{
    return (int8_t)(encoded & 0xFF);
}

int8_t DecodeDY(int16_t encoded)
{
    return (int8_t)(encoded >> 8);
}

int IsOpposite(int16_t a, int16_t b)
{
    return DecodeDX(a) == -DecodeDX(b) &&
           DecodeDY(a) == -DecodeDY(b);
}

GameManager* GameManager_create(int w, int h, int playerCount)
{
    GameManager* game = (GameManager*)malloc(sizeof(GameManager));
    game->gameWidth = w;
    game->gameHeight = h;
    game->playerCounts = playerCount;

    game->snakeDataList = malloc(sizeof(uint16_t*) * playerCount);
    for (int i = 0; i < playerCount; i++) {
        game->snakeDataList[i] = malloc(sizeof(uint16_t) * w * h);
        memset(game->snakeDataList[i], 65535, sizeof(uint16_t) * w * h);
    }

    game->appleCount = playerCount;
    game->allAppleData = malloc(sizeof(uint16_t) * game->appleCount);

    game->snakePositions = malloc(sizeof(uint16_t) * playerCount);
    game->moveDirections = malloc(sizeof(int16_t)  * playerCount);
    game->livingStatus = malloc(sizeof(int)  * playerCount);

    for (int i = 0; i < playerCount; i++)
    {
        int x = 0, y = 0;
        int dx = 0, dy = 0;

        switch (i)
        {
            case 0: // Player 1: top-left, moving right
                x = 0;          y = h/4;
                dx = 1;         dy = 0;
                break;

            case 1: // Player 2: bottom-right, moving left
                x = w - 1;      y = h-h/4;
                dx = -1;        dy = 0;
                break;

            case 2: // Player 3: bottom-left, moving right
                x = 0;          y = h-h/4;
                dx = 1;         dy = 0;
                break;

            case 3: // Player 4: top-right, moving left
                x = w - 1;      y = h/4;
                dx = -1;        dy = 0;
                break;
        }

        game->snakePositions[i] = EncodePosition(x, y);
        game->moveDirections[i] = EncodeDelta(dx, dy);
        game->livingStatus[i] = 1;
    }

    game->snakeLens = malloc(sizeof(int) * playerCount);
    for(int i = 0; i < playerCount; i++)
    {
        game->snakeLens[i] = 4;
    }
    game->tickRate = 1.0 / 8.0;

    game->scores = calloc(playerCount, sizeof(int));

    game->inputQueues = malloc(sizeof(int16_t*) * playerCount);
    for (int i = 0; i < playerCount; i++) {
        game->inputQueues[i] = calloc(2, sizeof(int16_t));
    }
    game->inputCounts = calloc(playerCount, sizeof(int));

    game->clock = 0;
    game->shuffleCursor = 0;
    game->shuffledCount = game->gameWidth * game->gameHeight;
    game->shuffledPositions = malloc(sizeof(uint16_t) * game->shuffledCount);

    // Fill with possible positions
    uint16_t* shuffledPtr = game->shuffledPositions;
    for (int y = 0; y < game->gameHeight; y++)
    {
        for (int x = 0; x < game->gameWidth; x++)
        {
            *shuffledPtr++ = EncodePosition(x, y);
        }
    }

    // Shuffle positions
    for (int i = game->shuffledCount - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        uint16_t tmp = game->shuffledPositions[i];
        game->shuffledPositions[i] = game->shuffledPositions[j];
        game->shuffledPositions[j] = tmp;
    }

    for(int a = 0; a < playerCount; a++)
    {
        for(int i = 0; i < game->snakeLens[a]; i++)
        {
            game->snakeDataList[a][i] = game->snakePositions[a];
        }
    }
    for(int i = 0; i < game->appleCount; i++)
    {
        uint16_t pos = EncodePosition(rand()%w, rand()%h);
        game->allAppleData[i] = pos;
    }
    return game;
}

void GameManager_destroy(GameManager **game)
{
    if (!game || !*game)
        return;

    GameManager *inner = *game;

    if (inner->inputQueues)
    {
        for (int i = 0; i < inner->playerCounts; i++)
            free(inner->inputQueues[i]);

        free(inner->inputQueues);
    }

    free(inner->inputCounts);

    if (inner->snakeDataList)
    {
        for (int i = 0; i < inner->playerCounts; i++)
            free(inner->snakeDataList[i]);

        free(inner->snakeDataList);
    }

    free(inner->snakePositions);
    free(inner->moveDirections);
    free(inner->snakeLens);
    free(inner->scores);
    free(inner->allAppleData);
    free(inner->shuffledPositions);

    free(inner);
    *game = NULL;
}

int SnakeContains(const GameManager* game, uint16_t pos)
{
    for(int a = 0; a < game->playerCounts; a++)
    {
        for (int i = 0; i < game->snakeLens[a]; i++)
        {
            if (game->snakeDataList[a][i] == pos)
                return 1;
        }
    }
    return 0;
}

int ApplesContain(const uint16_t* apples, int appleCount, int ignoreIndex, uint16_t pos)
{
    for (int i = 0; i < appleCount; i++)
    {
        if (i == ignoreIndex) continue;
        if (*apples++ == pos)
            return 1;
    }
    return 0;
}

uint16_t SpawnAppleSafe(GameManager* game, int appleIndex, uint16_t nextPos)
{
    for (int i = 0; i < game->shuffledCount; i++)
    {
        uint16_t pos = game->shuffledPositions[game->shuffleCursor++];

        if(nextPos == pos)
            continue;

        if (game->shuffleCursor >= game->shuffledCount)
            game->shuffleCursor = 0;

        if (SnakeContains(game, pos))
            continue;

        if (ApplesContain(game->allAppleData, game->appleCount, appleIndex, pos))
            continue;

        return pos;
    }

    // No free space left
    return UINT16_MAX;
}

int GameManager_tick(GameManager* game, double deltaTime)
{
    game->clock += deltaTime;
    int ticked = 0;
    while (game->clock > game->tickRate)
    {
        ticked = 1;
        game->clock -= game->tickRate;

        // Process input
        for(int a = 0; a < game->playerCounts; a++)
        {
            if(!game->livingStatus[a])
                continue;
            
            while (game->inputCounts[a] > 0)
            {
                int16_t desired = game->inputQueues[a][0];
                if(!IsOpposite(desired, game->moveDirections[a]))
                {
                    game->moveDirections[a] = desired;
                    for (int i = 1; i < game->inputCounts[a]; i++)
                        game->inputQueues[a][i - 1] = game->inputQueues[a][i];
                    game->inputCounts[a]--;
                    break;
                }
                game->inputCounts[a]--;
            }

            // Compute next head
            uint16_t nextHead =
                game->snakePositions[a] +
                ((DecodeDY(game->moveDirections[a]) << 8) + DecodeDX(game->moveDirections[a]));

            // Out of bounds check
            int x = nextHead & 0xFF;
            int y = nextHead >> 8;
            if(x < 0 || x >= game->gameWidth || y < 0 || y >= game->gameHeight)
            {
                AudioManager_PlayCh("assets/audio/sfx/collide.ogg", 0.25, 3, 0);
                game->livingStatus[a] = 0;
                //AudioManager_StopCh(0);
                //return 1;
            }

            // Check apples FIRST (before shifting)
            int growPending = 0;
            uint16_t* apples = game->allAppleData;

            for (int i = 0; i < game->appleCount; i++)
            {
                if (*apples == nextHead)
                {
                    growPending = 1;
                    AudioManager_PlayCh("assets/audio/sfx/apple.ogg", 0.75, 2, 0);
                    uint16_t result = SpawnAppleSafe(game, i, nextHead);
                    if (result == UINT16_MAX)
                    {
                        *apples = game->allAppleData[game->appleCount - 1];
                        game->appleCount--;
                    }
                    else
                    {
                        *apples = result;
                    }
                }
                apples++;
            }

            // Only remove tail if NOT growing
            if (!growPending)
            {
                for (int i = 0; i < game->snakeLens[a] - 1; i++)
                {
                    game->snakeDataList[a][i] = game->snakeDataList[a][i + 1];
                }
            }
            else
            {
                game->snakeLens[a]++;
                game->scores[a]++;
            }

            // Collision check
            if(SnakeContains(game, nextHead))
            {
                AudioManager_PlayCh("assets/audio/sfx/collide.ogg", 0.25, 3, 0);
                game->livingStatus[a] = 0;
                //AudioManager_StopCh(0);
                //shouldEnd = 1;
            }

            // Append head
            game->snakeDataList[a][game->snakeLens[a] - 1] = nextHead;
            game->snakePositions[a] = nextHead;
        }
    }
    (void)game;
    (void)deltaTime;

    return ticked;
}

void* CreateGameStatePacket(GameManager* game) {
    json_t* packet = json_object();

    json_object_set_new(packet, "packet", json_string("tick"));

    json_t* players = json_array();
    for(int p = 0; p < game->playerCounts; p++) {
        json_t* player = json_object();

        json_t* snakeArray = json_array();
        for(int i = 0; i < game->snakeLens[p]; i++) {
            json_array_append_new(snakeArray, json_integer(game->snakeDataList[p][i]));
        }
        json_object_set_new(player, "snake", snakeArray);

        json_object_set_new(player, "dir", json_integer(game->moveDirections[p]));
        json_object_set_new(player, "length", json_integer(game->snakeLens[p]));
        json_object_set_new(player, "score", json_integer(game->scores[p]));
        json_object_set_new(player, "alive", json_boolean(game->livingStatus[p]));

        json_array_append_new(players, player);
    }
    json_object_set_new(packet, "players", players);

    json_t* apples = json_array();
    for(int i = 0; i < game->appleCount; i++) {
        json_array_append_new(apples, json_integer(game->allAppleData[i]));
    }
    json_object_set_new(packet, "apples", apples);

    return packet;
}

void LoadGameStatePacket(GameManager* game, void* pck) {
    json_t* packet = (json_t*)pck;

    json_t* players = json_object_get(packet, "players");
    if (!players || !json_is_array(players)) return;

    size_t playerCount = json_array_size(players);
    if ((int)playerCount != game->playerCounts) return;

    for (size_t p = 0; p < playerCount; p++) {
        json_t* player = json_array_get(players, p);

        json_t* snakeArray = json_object_get(player, "snake");
        if (snakeArray && json_is_array(snakeArray)) {
            size_t snakeLen = json_array_size(snakeArray);
            game->snakeLens[p] = (int)snakeLen;

            for (size_t i = 0; i < snakeLen; i++) {
                uint16_t pos = (uint16_t)json_integer_value(json_array_get(snakeArray, i));
                game->snakeDataList[p][i] = pos;
            }
        }

        json_t* dir = json_object_get(player, "dir");
        if (dir) {
            game->moveDirections[p] = (uint16_t)json_integer_value(dir);
        }

        json_t* score = json_object_get(player, "score");
        if (score) game->scores[p] = (int)json_integer_value(score);

        json_t* alive = json_object_get(player, "alive");
        if (alive) game->livingStatus[p] = json_boolean_value(alive);
    }

    json_t* apples = json_object_get(packet, "apples");
    if (apples && json_is_array(apples)) {
        size_t appleCount = json_array_size(apples);
        game->appleCount = (uint16_t)appleCount;
        for (size_t i = 0; i < appleCount; i++) {
            game->allAppleData[i] = (uint16_t)json_integer_value(json_array_get(apples, i));
        }
    }
}