#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "SharedConfig.h"

#include "Effects/TrippyBackground.h"

#include "RenderMaster/Transitions.h"
#include "RenderMaster/AudioManager.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/SceneUtils.h"
#include "RenderMaster/Scenes/SceneTitle.h"

static SharedConfig* cfg = NULL;

InitializedScene* SceneTitle_Init()
{
    SceneTitle* scene = malloc(sizeof(SceneTitle));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneTitle_Prepare;
    scene->sceneInfo->workFunction = SceneTitle_Work;
    scene->sceneInfo->inputFunction = SceneTitle_OnInput;

    cfg = SharedConfig_Get();

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_TITLE;
    returnData->sceneName = strdup("Title");

    return returnData;
}

int SceneTitle_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneTitle* self = (SceneTitle*)_self;

    self->selPos = 1;
    self->abandon = -1;
    cfg->isMpMusicPlaying = 0;
    AudioManager_PlayCh("assets/audio/mus/mus_main.ogg", 0.25, 0, -1);

    (void)self;
    (void)window;
    (void)renderer;
    (void)previousScene;
    return 1;
}

int SceneTitle_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneTitle* self = (SceneTitle*)_self;

    if(Transitions_IsBusy())
        return 1;

    if (event->type == SDL_KEYDOWN)
    {
        switch(event->key.keysym.sym)
        {
            case W:
            case ArrowUp:
                if(self->selPos > 1) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    self->selPos--;
                }
                break;
            case S:
            case ArrowDown:
                if(self->selPos < 4) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    self->selPos++;
                }
                break;
            
            case Enter:
            {
                switch(self->selPos)
                {
                    case 1: // Singleplayer
                    {
                        cfg->localMultiplayer = 0;
                        self->abandon = SCENE_GAME;
                        Transitions_Start("Singleplayer", 164, 255, 164);
                        //AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                    case 2: // Local Multiplayer
                    {
                        cfg->localMultiplayer = 1;
                        self->abandon = SCENE_GAME;
                        Transitions_Start("Local   Multiplayer", 255, 255, 164);
                        //AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                    case 3: // Online Multiplayer
                    {
                        cfg->localMultiplayer = 0;
                        self->abandon = SCENE_MULTI;
                        Transitions_Start("WorldLink", 164, 164, 255);
                        //AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                    case 4: // Scoreboard
                    {
                        self->abandon = SCENE_SCOREBOARD;
                        Transitions_Start("Scoreboard", 255, 192, 128);
                        //AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                }
                break;
            }
        }
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)event;
    return 1;
}

int SceneTitle_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    static double time = 0;
    time += deltaTime;

    SceneTitle* self = (SceneTitle*)_self;

    if(self->abandon != 1)
    {
        int mid = Transitions_Step(deltaTime);
        if(mid)
            return self->abandon;
    }

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    DrawTrippyBackground(renderer, deltaTime);

    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/40, "Simple   Snakes", Centered, Start, (SDL_Color){48, 48, 48, 255});
    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/48, "Simple   Snakes", Centered, Start, (SDL_Color){192, 255, 192, 255});
    FontManager_Render(renderer, "Monaco", height/16, width/2, height/12, "Made in 2 weeks for Chas Academy", Centered, Start, (SDL_Color){127, 127, 127, 255});

    int start = height/2;

    int finalWidth = 0;
    int finalHeight = 0;
    SDL_Rect textRect = FontManager_RenderFixed(renderer, "MonacoVS", height/20, 26, width/2, start, "Singleplayer", Centered, Centered, (SDL_Color){164, 255, 164, 255});
    if(self->selPos == 1)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/16;
    textRect = FontManager_RenderFixed(renderer, "MonacoVS", height/20, 26, width/2, start, "Local   Multiplayer", Centered, Centered, (SDL_Color){255, 255, 164, 255});
    if(self->selPos == 2)
    {
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/5, "Player 1 controls: WASD", Centered, Start, (SDL_Color){255, 164, 164, 255});
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/4, "Player 2 controls: IJKL", Centered, Start, (SDL_Color){164, 255, 164, 255});
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/16;
    textRect = FontManager_RenderFixed(renderer, "MonacoVS", height/20, 26, width/2, start, "Online   Multiplayer", Centered, Centered, (SDL_Color){164, 164, 255, 255});
    if(self->selPos == 3)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/16;
    textRect = FontManager_RenderFixed(renderer, "MonacoVS", height/20, 26, width/2, start, "Scoreboard", Centered, Centered, (SDL_Color){255, 192, 128, 255});
    if(self->selPos == 4)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }

    int cursorOffset = height/48;
    if(fmod(time*1.5, 1) < 0.5)
        cursorOffset += height/128;
    FontManager_Render(renderer, "Silkscreen-Regular-mod", height/28, width/2-finalWidth/2-cursorOffset, finalHeight - height/256, ">", Centered, Centered, (SDL_Color){255, 255, 255, 255});

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}