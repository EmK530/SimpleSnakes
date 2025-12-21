#ifndef SCENECONFIG_H
#define SCENECONFIG_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    int isWSL;
} SceneConfig;

InitializedScene* SceneConfig_Init();
int SceneConfig_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneConfig_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneConfig_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENECONFIG_H