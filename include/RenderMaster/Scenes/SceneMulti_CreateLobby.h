#ifndef SCENEMULTI_CREATELOBBY_H
#define SCENEMULTI_CREATELOBBY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    double time;
    int selPos;
    int abandon;
    int publicSetting;
    int typePos;
    char nameInput[25];
} SceneMulti_CreateLobby;

InitializedScene* SceneMulti_CreateLobby_Init();
int SceneMulti_CreateLobby_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneMulti_CreateLobby_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneMulti_CreateLobby_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENEMULTI_CREATELOBBY_H