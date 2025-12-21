#ifndef SCENEMULTI_ENTERPRIVATE_H
#define SCENEMULTI_ENTERPRIVATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    double time;
    int selPos;
    int abandon;
    int typePos;
    char codeInput[7];
} SceneMulti_EnterPrivate;

InitializedScene* SceneMulti_EnterPrivate_Init();
int SceneMulti_EnterPrivate_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneMulti_EnterPrivate_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneMulti_EnterPrivate_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENEMULTI_ENTERPRIVATE_H