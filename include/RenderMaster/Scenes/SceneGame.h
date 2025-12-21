#ifndef SceneGame_H
#define SceneGame_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    int gameStarted;
    int gamePaused;
    int gameEnded;
    int winnerId;
    int loadedCount;
    double loseTimer;
    double lastPacket; // for mp
    int playedLoseSound;
    int countId;
    int lastCountId; // for mp
    double countDelta;
    int abandon;
    int framesPlayed;
    int pauseSel;
    int ready;
    int myPlayerIndex;
    int mpLastLen;
    int mpHasDied;
    char** multiplayerNames;
} SceneGame;

InitializedScene* SceneGame_Init();
int SceneGame_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneGame_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneGame_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SceneGame_H