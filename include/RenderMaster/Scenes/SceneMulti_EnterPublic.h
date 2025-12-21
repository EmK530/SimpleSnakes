#ifndef SCENEMULTI_ENTERPUBLIC_H
#define SCENEMULTI_ENTERPUBLIC_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    GenericScene* sceneInfo;
    int deferredTimer;
    int listIndex;
    int selectingList;
    int topbarSelect;
    int abandon;
    double refreshCooldown;
    double time;
} SceneMulti_EnterPublic;

InitializedScene* SceneMulti_EnterPublic_Init();
int SceneMulti_EnterPublic_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneMulti_EnterPublic_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneMulti_EnterPublic_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENEMULTI_ENTERPUBLIC_H