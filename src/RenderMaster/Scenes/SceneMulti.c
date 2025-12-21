#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Utils.h"
#include "SharedConfig.h"
#include "LinkedList.h"

#include "Effects/TileBackground.h"

#include "RenderMaster/AudioManager.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/SceneUtils.h"
#include "RenderMaster/Scenes/SceneMulti.h"

static Multiplayer* mp = NULL;
static SharedConfig* cfg = NULL;

InitializedScene* SceneMulti_Init()
{
    SceneMulti* scene = malloc(sizeof(SceneMulti));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneMulti_Prepare;
    scene->sceneInfo->workFunction = SceneMulti_Work;
    scene->sceneInfo->inputFunction = SceneMulti_OnInput;

    cfg = SharedConfig_Get();
    mp = cfg->mp;

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_MULTI;
    returnData->sceneName = strdup("Multiplayer");

    return returnData;
}

int SceneMulti_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneMulti* self = (SceneMulti*)_self;

    mp->listStatus = -1;
    self->time = 0;
    self->selPos = 2;
    self->abandon = -1;
    self->typePos = strlen(mp->username);

    if(!cfg->isMpMusicPlaying)
    {
        cfg->isMpMusicPlaying = 1;
        AudioManager_PlayCh("assets/audio/mus/mus_multi.ogg", 0.25, 0, -1);
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)previousScene;
    return 1;
}

int SceneMulti_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneMulti* self = (SceneMulti*)_self;

    if(self->selPos == 1 && event->type == SDL_TEXTINPUT)
    {
        int textLen = strlen(event->text.text);
        if(textLen == 1 && self->typePos + textLen <= 20)
        {
            memcpy(mp->username+self->typePos, event->text.text, textLen);
            self->typePos += textLen;
            AudioManager_PlayCh("assets/audio/sfx/typing.ogg", 0.5, 2, 0);
        } else {
            AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 2, 0);
        }
    } else if (event->type == SDL_KEYDOWN)
    {
        switch(event->key.keysym.sym)
        {
            case ArrowUp:
                if(self->selPos == 2)
                    SDL_StartTextInput();
                if(self->selPos > 1) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    self->selPos--;
                }
                break;
            case ArrowDown:
                if(self->selPos == 1)
                    SDL_StopTextInput();
                if(self->selPos < 5) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    self->selPos++;
                }
                break;
            
            case Enter:
            {
                switch(self->selPos)
                {
                    case 2: // Create Lobby
                    {
                        self->abandon = SCENE_MULTI_CREATELOBBY;
                        AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                    case 3: // Join Public
                    {
                        self->abandon = SCENE_MULTI_ENTERPUBLIC;
                        AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                    case 4: // Join Private
                    {
                        self->abandon = SCENE_MULTI_ENTERPRIVATE;
                        AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                    case 5: // Back
                    {
                        self->abandon = SCENE_TITLE;
                        AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        break;
                    }
                }
                break;
            }

            case Backspace:
            {
                if(self->selPos == 1 && self->typePos > 0)
                {
                    mp->username[--self->typePos] = '\0';
                    AudioManager_PlayCh("assets/audio/sfx/typing.ogg", 0.5, 2, 0);
                }
            }
        }
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)event;
    return 1;
}

int SceneMulti_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    SceneMulti* self = (SceneMulti*)_self;
    self->time += deltaTime;

    if(self->abandon != -1)
    {
        mp->timeout = 0;
        return self->abandon;
    }

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    DrawCellBackground(renderer, self->time, 3);

    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/40, "WorldLink", Centered, Start, (SDL_Color){48, 48, 48, 255});
    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/48, "WorldLink", Centered, Start, (SDL_Color){192, 192, 255, 255});

    if(mp->timeout != 0)
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/4, (mp->timeout == 1 ? "Connection to the host was lost." : (mp->timeout == 2 ? "The host closed the lobby." : "You were kicked from the lobby.")), Centered, Centered, (SDL_Color){255, 164, 164, 255});

    FontManager_Render(renderer, "Monaco", height/16, width/2, height/2.95, "Username", Centered, Centered, (SDL_Color){255, 255, 255, 255});
    SDL_Rect inputRect = {width/2,height/2.65,height/1.7,height/22};
    inputRect.x -= inputRect.w/2;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &inputRect);
    int color = 255;
    if(self->selPos == 1)
        color = 164;
    OutlineRect(renderer, &inputRect, height/180, color, 255, color);

    if(self->typePos == 0)
    {
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/2.625, "Type here!", Centered, Start, (SDL_Color){96, 96, 96, 255});
    } else {
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/2.625, mp->username, Centered, Start, (SDL_Color){255, 255, 255, 255});
    }

    int start = height/2;

    int finalWidth = 0;
    int finalHeight = 0;
    SDL_Rect textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Create Lobby", Centered, Centered, (SDL_Color){164, 255, 164, 255});
    if(self->selPos == 2)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/12;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Join Public", Centered, Centered, (SDL_Color){255, 255, 164, 255});
    if(self->selPos == 3)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/20;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Join Private", Centered, Centered, (SDL_Color){255, 192, 128, 255});
    if(self->selPos == 4)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/12;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Back", Centered, Centered, (SDL_Color){255, 164, 164, 255});
    if(self->selPos == 5)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    
    // Render selector cursor
    if(self->selPos != 1)
    {
        int cursorOffset = height/48;
        if(fmod(self->time*1.5, 1) < 0.5)
            cursorOffset += height/128;
        FontManager_Render(renderer, "Silkscreen-Regular-mod", height/28, width/2-finalWidth/2-cursorOffset, finalHeight - height/256, ">", Centered, Centered, (SDL_Color){255, 255, 255, 255});
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}