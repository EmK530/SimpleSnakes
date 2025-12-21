#ifndef RENDERMASTER_H
#define RENDERMASTER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "RenderMaster/SceneUtils.h"

typedef struct
{
    // Internals
    SDL_Window *window;
    SDL_Renderer* renderer;
    GenericScene** scenes;

    int scene;
    int lastScene;
    int currentFlags;
} RenderMaster;

void RenderMaster_Init();
RenderMaster* RenderMaster_Create(const char* title, int width, int height);
void RenderMaster_Destroy(RenderMaster** rm);
int RenderMaster_Work(RenderMaster* rm);

#endif // RENDERMASTER_H