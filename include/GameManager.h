#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <stdint.h>

typedef struct
{
    int gameWidth;
    int gameHeight;
    int playerCounts;

    uint16_t* shuffledPositions;
    int shuffledCount;
    int shuffleCursor;

    int16_t** inputQueues;
    int* inputCounts;

    double clock;
    double tickRate;
    int* snakeLens;
    int* scores;

    uint16_t* snakePositions;
    uint16_t** snakeDataList;
    int* livingStatus;

    uint16_t appleCount;
    uint16_t* allAppleData;
    int16_t* moveDirections;
} GameManager;

GameManager* GameManager_create(int w, int h, int playerCount);
void GameManager_destroy(GameManager** game);
int GameManager_tick(GameManager* game, double deltaTime);

int16_t EncodeDelta(int8_t dx, int8_t dy);
int8_t DecodeDX(int16_t encoded);
int8_t DecodeDY(int16_t encoded);
int IsOpposite(int16_t a, int16_t b);

// I had to void this due to import errors
void* CreateGameStatePacket(GameManager* game);
void LoadGameStatePacket(GameManager* game, void* packet);

#endif // GAMEMANAGER_H