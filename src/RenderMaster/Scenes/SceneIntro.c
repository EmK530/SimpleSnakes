#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "easings.h"
#include "StaticConfig.h"
#include "RenderMaster/AudioManager.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/SceneUtils.h"
#include "RenderMaster/Scenes/SceneIntro.h"

InitializedScene* SceneIntro_Init()
{
    SceneIntro* scene = malloc(sizeof(SceneIntro));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneIntro_Prepare;
    scene->sceneInfo->workFunction = SceneIntro_Work;
    scene->sceneInfo->inputFunction = SceneIntro_OnInput;

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_INTRO;
    returnData->sceneName = strdup("Intro");

    return returnData;
}

int SceneIntro_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneIntro* self = (SceneIntro*)_self;

    (void)self;
    (void)window;
    (void)renderer;
    (void)previousScene;
    return 1;
}

int SceneIntro_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneIntro* self = (SceneIntro*)_self;

    //printf("%i\n", event->type);

    (void)self;
    (void)window;
    (void)renderer;
    (void)event;
    return 1;
}

int SceneIntro_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    static double time = 0;
    static int tick = 0;
    time += deltaTime;

    if(time < 0.5)
        return -1;

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    SceneIntro* self = (SceneIntro*)_self;

    int alpha = 255;
    double runtime = 0.15;
    int color = 128;
    double sizeDiv = 30;
    int targetHeight = height/2;
    char* text = tick == 2 ? "THIS INTRO IS SCRAPPED LOL" : "GLOBAL SNAKE";
    if(time < 0.5+(runtime))
    {
        double frac = 1-easeOutQuad((time-0.5)*(1/runtime));
        targetHeight -= 40*frac;
        alpha -= 255*frac;
        if(tick == 0)
        {
            tick = 1;
            AudioManager_Play("assets/audio/sfx/intro_tick.ogg", 0.5, 0);
        }
    } else if(time > 1.5 && time < 1.5+runtime) {
        double frac = 1-easeOutQuad((time-1.5)*(1/runtime));
        targetHeight -= 40*frac;
        alpha -= 255*frac;
        if(tick == 1)
        {
            tick = 2;
            AudioManager_Play("assets/audio/sfx/intro_tick.ogg", 0.5, 0);
        }
    } else if(time >= 2.5) {
        return SCENE_TITLE;
    }
    FontManager_Render(renderer, "Silkscreen-Regular-mod", height/sizeDiv, width/2, targetHeight, text, Centered, Centered, (SDL_Color){color, color, color, alpha});

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}