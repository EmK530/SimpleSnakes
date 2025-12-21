#ifndef SCENESCOREBOARD_H
#define SCENESCOREBOARD_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    int abandon;
} SceneScoreboard;

InitializedScene* SceneScoreboard_Init();
int SceneScoreboard_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneScoreboard_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneScoreboard_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENESCOREBOARD_H