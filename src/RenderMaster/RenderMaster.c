#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "StaticConfig.h"
#include "SharedConfig.h"
#include "RenderMaster/RenderMaster.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/AudioManager.h"
#include "RenderMaster/Transitions.h"

// All scenes
#include "RenderMaster/Scenes/SceneIntro.h"
#include "RenderMaster/Scenes/SceneTitle.h"
#include "RenderMaster/Scenes/SceneConfig.h"
#include "RenderMaster/Scenes/SceneGame.h"
#include "RenderMaster/Scenes/SceneMulti_EnterPublic.h"
#include "RenderMaster/Scenes/SceneMulti.h"
#include "RenderMaster/Scenes/SceneMulti_CreateLobby.h"
#include "RenderMaster/Scenes/SceneMulti_Lobby.h"
#include "RenderMaster/Scenes/SceneRecord.h"
#include "RenderMaster/Scenes/SceneScoreboard.h"
#include "RenderMaster/Scenes/SceneMulti_EnterPrivate.h"

static SharedConfig* cfg = NULL;

void RenderMaster_Init()
{
    cfg = SharedConfig_Get();
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    FontManager_Init();
    AudioManager_Init();
}

void LoadSceneIntoArray(RenderMaster* rm, InitializedScene* (*method)(void))
{
    InitializedScene* scene = method();
    printf("[RenderMaster] Initialized scene #%i \"%s\".\n", scene->sceneId+1, scene->sceneName);
    rm->scenes[scene->sceneId] = scene->scene;
    free(scene->sceneName);
    free(scene);
}

// For easy definition of new scenes
void LoadAllRenderMasterScenes(RenderMaster* rm)
{
    LoadSceneIntoArray(rm, SceneIntro_Init);
    LoadSceneIntoArray(rm, SceneTitle_Init);
    LoadSceneIntoArray(rm, SceneConfig_Init);
    LoadSceneIntoArray(rm, SceneGame_Init);
    LoadSceneIntoArray(rm, SceneMulti_EnterPublic_Init);
    LoadSceneIntoArray(rm, SceneMulti_Init);
    LoadSceneIntoArray(rm, SceneMulti_CreateLobby_Init);
    LoadSceneIntoArray(rm, SceneMulti_Lobby_Init);
    LoadSceneIntoArray(rm, SceneRecord_Init);
    LoadSceneIntoArray(rm, SceneScoreboard_Init);
    LoadSceneIntoArray(rm, SceneMulti_EnterPrivate_Init);
}

RenderMaster* RenderMaster_Create(const char* title, int width, int height)
{
    if(width == 0)
        width = cfg->screenWidth;
    if(height == 0)
        height = cfg->screenHeight;

    SDL_Window *win = SDL_CreateWindow(title, 100, 100, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win) {
        printf("RenderMaster_Create failed: SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }
    int flags = SDL_RENDERER_ACCELERATED;
    if(cfg->verticalSync)
        flags |= SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, flags);
    if (!ren) {
        SDL_DestroyWindow(win);
        printf("RenderMaster_Create failed: SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }
    cfg->gameTex = SDL_CreateTexture(
        ren,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height
    );
    SDL_SetRenderTarget(ren, cfg->gameTex);

    RenderMaster* rm = (RenderMaster*)malloc(sizeof(RenderMaster));
    if (!rm)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        printf("RenderMaster_Create failed: malloc failed\n");
        SDL_Quit();
        return NULL;
    }

    //SDL_SetWindowResizable(win, SDL_FALSE);

    // Load Scenes
    rm->scenes = malloc(sizeof(GenericScene*) * _maxScene);
    LoadAllRenderMasterScenes(rm);

    rm->currentFlags = flags;

    rm->window = win;
    rm->renderer = ren;
    rm->lastScene = -1;

    #ifdef DEV_BUILD
        rm->scene = SCENE_TITLE;
    #else
        rm->scene = SCENE_CONFIG;
    #endif

    return rm;
}

int RenderMaster_Work(RenderMaster* rm)
{
    static Uint64 lastCounter = 0;
    static double fps = 0;

    if (!rm)
        return 0;

    int flags = SDL_RENDERER_ACCELERATED;
    if(cfg->verticalSync)
        flags |= SDL_RENDERER_PRESENTVSYNC;
    if(flags != rm->currentFlags)
    {
        rm->currentFlags = flags;
        SDL_DestroyRenderer(rm->renderer);
        SDL_Renderer *ren = SDL_CreateRenderer(rm->window, -1, flags);
        if (!ren) {
            SDL_DestroyWindow(rm->window);
            printf("[RenderMaster] ERROR: Could not recreate SDL Renderer. SDLError: %s\n", SDL_GetError());
            SDL_Quit();
            return 0;
        }
        rm->renderer = ren;
    }

    GenericScene* scene = rm->scenes[rm->scene];
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return 0;
        }
        scene->inputFunction(scene->context, rm->window, rm->renderer, &e);
    }

    Uint64 currentCounter = SDL_GetPerformanceCounter();
    double delta = 0;
    if (lastCounter != 0) {
        delta = (double)(currentCounter - lastCounter) / SDL_GetPerformanceFrequency();
        fps = 1.0 / delta;
    }
    lastCounter = currentCounter;

    int updated = 0;
    if(cfg->resolutionWasChanged)
    {
        printf("[RenderMaster] Applying new resolution: %ix%i\n",cfg->screenWidth,cfg->screenHeight);
        SDL_SetWindowSize(rm->window, cfg->screenWidth, cfg->screenHeight);
        updated = 1;
    } else {
        int curW = 0;
        int curH = 0;
        SDL_GetRendererOutputSize(rm->renderer, &curW, &curH);
        if(curW != cfg->screenWidth || curH != cfg->screenHeight)
        {
            printf("[RenderMaster] Resolution updating: %ix%i\n",curW,curH);
            cfg->screenWidth = curW;
            cfg->screenHeight = curH;
            updated = 1;
        }
    }
    if(cfg->gameTex != NULL)
    {
        if(updated)
        {
            SDL_DestroyTexture(cfg->gameTex);
            cfg->gameTex = SDL_CreateTexture(
                rm->renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                cfg->screenWidth,
                cfg->screenHeight
            );
        }
        //SDL_SetRenderTarget(rm->renderer, cfg->gameTex);
    }

    if(Transitions_IsActive())
        SDL_SetRenderTarget(rm->renderer, cfg->gameTex);

    SDL_SetRenderDrawColor(rm->renderer, 0, 0, 0, 255);
    SDL_RenderClear(rm->renderer);

    // Invoke Scene
    while(1)
    {
        scene = rm->scenes[rm->scene];
        if(scene == NULL)
        {
            printf("[RenderMaster] ERROR: Attempt to jump to unknown scene #%i\n", rm->scene+1);
            return 0;
        }
        if(rm->lastScene != rm->scene)
        {
            printf("[RenderMaster] Jumped to scene #%i\n", rm->scene+1);
            scene->prepFunction(scene->context, rm->window, rm->renderer, rm->lastScene);
            rm->lastScene = rm->scene;
        }
        int result = scene->workFunction(scene->context, rm->window, rm->renderer, delta);
        if(result == -1)
        {
            break;
        }
        if(result >= 0)
        {
            rm->scene = result;
            rm->lastScene = -1;
        } else {
            return 0;
        }
    }

    SDL_SetRenderTarget(rm->renderer, NULL);

    if(!Transitions_IsActive())
    {
        //SDL_RenderCopy(rm->renderer, cfg->gameTex, NULL, NULL);
    } else {
        if(!Transitions_HasStepped())
            Transitions_Step(delta);
        Transitions_Draw(rm->renderer);
    }

    // Display FPS
    if(cfg->displayFps)
    {
        char tempText[32];
        snprintf(tempText, sizeof(tempText), "FPS: %.0lf", fps);
        FontManager_Render(rm->renderer, RM_FPS_FONT, cfg->fpsFontSize, 9, 3, tempText, Start, Start, (SDL_Color){48, 48, 48, 255});
        FontManager_Render(rm->renderer, RM_FPS_FONT, cfg->fpsFontSize, 7, 1, tempText, Start, Start, (SDL_Color){0, 158, 47, 255});

        #ifdef DEV_BUILD
        snprintf(tempText, sizeof(tempText), "FONT_ALLOC: %i", FontManager_Debug_Count());
        FontManager_Render(rm->renderer, "acme_7_wide_xtnd", cfg->fpsFontSize, 9, 5+cfg->fpsFontSize, tempText, Start, Start, (SDL_Color){48, 48, 48, 255});
        FontManager_Render(rm->renderer, "acme_7_wide_xtnd", cfg->fpsFontSize, 7, 3+cfg->fpsFontSize, tempText, Start, Start, (SDL_Color){0, 158, 47, 255});
        #endif
    }

    SDL_RenderPresent(rm->renderer);

    FontManager_Work();

    return 1;
}

void RenderMaster_Destroy(RenderMaster** rm)
{
    if(!rm || !*rm)
        return;
    RenderMaster* rm_ptr = *rm;
    if(rm_ptr->renderer)
        SDL_DestroyRenderer(rm_ptr->renderer);
    if(cfg->gameTex != NULL)
    {
        SDL_DestroyTexture(cfg->gameTex);
        cfg->gameTex = NULL;
    }
    if(rm_ptr->window)
        SDL_DestroyWindow(rm_ptr->window);
    SDL_Quit();
    free(rm_ptr);
    *rm = NULL;
}