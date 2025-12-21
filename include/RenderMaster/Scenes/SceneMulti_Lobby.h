#ifndef SCENEMULTI_LOBBY_H
#define SCENEMULTI_LOBBY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    double time;
    int abandon;
    int selPos;
    int allReady;
    double lastPacket;
} SceneMulti_Lobby;

InitializedScene* SceneMulti_Lobby_Init();
int SceneMulti_Lobby_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneMulti_Lobby_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneMulti_Lobby_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENEMULTI_LOBBY_H