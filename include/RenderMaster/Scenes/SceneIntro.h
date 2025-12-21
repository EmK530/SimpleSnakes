#ifndef SCENEINTRO_H
#define SCENEINTRO_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
} SceneIntro;

InitializedScene* SceneIntro_Init();
int SceneIntro_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneIntro_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneIntro_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENEINTRO_H