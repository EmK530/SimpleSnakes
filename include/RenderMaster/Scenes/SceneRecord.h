#ifndef SCENERECORD_H
#define SCENERECORD_H

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
} SceneRecord;

InitializedScene* SceneRecord_Init();
int SceneRecord_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene);
int SceneRecord_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event);
int SceneRecord_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime);

#endif // SCENERECORD_H