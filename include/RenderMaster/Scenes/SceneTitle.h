#ifndef SCENETITLE_H
#define SCENETITLE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    int selPos;
    int abandon;
} SceneTitle;

InitializedScene* SceneTitle_Init();
int SceneTitle_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneTitle_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneTitle_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENETITLE_H