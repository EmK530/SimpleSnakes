#ifndef SCENEMULTI_H
#define SCENEMULTI_H

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
} SceneMulti;

InitializedScene* SceneMulti_Init();
int SceneMulti_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneMulti_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneMulti_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENEMULTI_H