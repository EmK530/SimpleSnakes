#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Utils.h"
#include "easings.h"
#include "StaticConfig.h"
#include "SharedConfig.h"
#include "RenderMaster/AudioManager.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/SceneUtils.h"
#include "RenderMaster/Scenes/SceneConfig.h"

static SharedConfig* cfg = NULL;

InitializedScene* SceneConfig_Init()
{
    cfg = SharedConfig_Get();

    SceneConfig* scene = malloc(sizeof(SceneConfig));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneConfig_Prepare;
    scene->sceneInfo->workFunction = SceneConfig_Work;
    scene->sceneInfo->inputFunction = SceneConfig_OnInput;

    scene->isWSL = QueryWSL();

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_CONFIG;
    returnData->sceneName = strdup("Config");

    return returnData;
}

int SceneConfig_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneConfig* self = (SceneConfig*)_self;

    AudioManager_Play("assets/audio/sfx/config_init.ogg", 0.75, 0);
    AudioManager_PlayCh("assets/audio/mus/mus_config.ogg", 0.25, 0, 1024);

    (void)self;
    (void)window;
    (void)renderer;
    (void)previousScene;
    return 1;
}

static int selPos = 1;
static int exiting = 0;
int SceneConfig_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneConfig* self = (SceneConfig*)_self;

    if(event->type == SDL_KEYDOWN)
    {
        int32_t key = event->key.keysym.sym;
        switch(key)
        {
            case W:
            case ArrowUp:
                if(selPos > 1) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    selPos--;
                }
                break;
            case S:
            case ArrowDown:
                if(selPos < 4) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    selPos++;
                }
                break;
            case A:
            case ArrowLeft:
                {
                    if(selPos == 1)
                    {
                        switch(cfg->audioBuffer)
                        {
                            case 256:
                                cfg->audioBuffer = 128; break;
                            case 512:
                                cfg->audioBuffer = 256; break;
                            case 1024:
                                cfg->audioBuffer = 512; break;
                            case 2048:
                                cfg->audioBuffer = 1024; break;
                            default:
                                return 1;
                        }
                        AudioManager_Reload();
                        AudioManager_PlayCh("assets/audio/mus/mus_config.ogg", 0.25, 0, -1);
                    } else if(selPos == 2)
                    {
                        switch(cfg->audioSample)
                        {
                            case 11025:
                                cfg->audioSample = 8000; break;
                            case 16000:
                                cfg->audioSample = 11025; break;
                            case 22050:
                                cfg->audioSample = 16000; break;
                            case 44100:
                                cfg->audioSample = 22050; break;
                            case 48000:
                                cfg->audioSample = 44100; break;
                            case 88200:
                                cfg->audioSample = 48000; break;
                            case 96000:
                                cfg->audioSample = 88200; break;
                            case 176400:
                                cfg->audioSample = 96000; break;
                            case 352800:
                                cfg->audioSample = 176400; break;
                            case 384000:
                                cfg->audioSample = 352800; break;
                            default:
                                return 1;
                        }
                        AudioManager_Reload();
                        AudioManager_PlayCh("assets/audio/mus/mus_config.ogg", 0.25, 0, -1);
                    } else if(selPos == 3)
                    {
                        if(!cfg->verticalSync)
                            return 1;
                        cfg->verticalSync = 0;
                    } else {
                        return 1;
                    }
                    AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                    break;
                }
            case D:
            case ArrowRight:
                {
                    if(selPos == 1)
                    {
                        switch(cfg->audioBuffer)
                        {
                            case 128:
                                cfg->audioBuffer = 256; break;
                            case 256:
                                cfg->audioBuffer = 512; break;
                            case 512:
                                cfg->audioBuffer = 1024; break;
                            case 1024:
                                cfg->audioBuffer = 2048; break;
                            default:
                                return 1;
                        }
                        AudioManager_Reload();
                        AudioManager_PlayCh("assets/audio/mus/mus_config.ogg", 0.25, 0, -1);
                            
                    } else if(selPos == 2)
                    {
                        switch(cfg->audioSample)
                        {
                            case 8000:
                                cfg->audioSample = 11025; break;
                            case 11025:
                                cfg->audioSample = 16000; break;
                            case 16000:
                                cfg->audioSample = 22050; break;
                            case 22050:
                                cfg->audioSample = 44100; break;
                            case 44100:
                                cfg->audioSample = 48000; break;
                            case 48000:
                                cfg->audioSample = 88200; break;
                            case 88200:
                                cfg->audioSample = 96000; break;
                            case 96000:
                                cfg->audioSample = 176400; break;
                            case 176400:
                                cfg->audioSample = 352800; break;
                            case 352800:
                                cfg->audioSample = 384000; break;
                            default:
                                return 1;
                        }
                        AudioManager_Reload();
                        AudioManager_PlayCh("assets/audio/mus/mus_config.ogg", 0.25, 0, -1);
                    } else if(selPos == 3)
                    {
                        if(cfg->verticalSync)
                            return 1;
                        cfg->verticalSync = 1;
                    } else {
                        return 1;
                    }
                    AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                    break;
                }
            case Enter:
                if(exiting || selPos != 4)
                    return 1;
                exiting = 1;
                AudioManager_PlayCh("assets/audio/sfx/config_exit.ogg", 0.33, 1, 0);
                break;
        }
        //printf("%i\n", event->key.keysym.sym);
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)event;
    return 1;
}

int SceneConfig_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    static double time = 0;
    static double exitTime = 0;
    time += deltaTime;
    if(exiting)
        exitTime += deltaTime;
    
    if(exitTime >= 1)
    {
        return SCENE_TITLE;
    }

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    SceneConfig* self = (SceneConfig*)_self;

    // Render Game Setup
    FontManager_RenderFixed(renderer, "MonacoVS", height/14, 40, width/2, height/76, "GAME SETUP", Centered, Start, (SDL_Color){255, 192, 128, 255});
    int start = height/14;
    FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Welcome to the Simple Snakes preview!", Centered, Start, (SDL_Color){192, 192, 192, 255}); start += height/28;
    FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Please configure the following important settings.", Centered, Start, (SDL_Color){192, 192, 192, 255}); start += height/28;
    FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Up/Down arrows to navigate, Left/Right arrows to edit, Enter to select", Centered, Start, (SDL_Color){192, 255, 192, 255});

    // Show setting explanations
    start = height/4;
    switch(selPos)
    {
        case 1:
            FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Lowering the audio buffer size will reduce latency", Centered, Start, (SDL_Color){255, 255, 255, 255}); start += height/28;
            FontManager_Render(renderer, "Monaco", height/18, width/2, start, "but may cause quality loss or total silence.", Centered, Start, (SDL_Color){255, 255, 255, 255});
            break;
        case 2:
            FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Sample Rate affects the quality of audio playback.", Centered, Start, (SDL_Color){255, 255, 255, 255}); start += height/28;
            FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Try switching between 44100Hz and 48000Hz if the music is crackling.", Centered, Start, (SDL_Color){255, 255, 255, 255});
            break;
        case 3:
            FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Locks the framerate to your monitor refresh rate", Centered, Start, (SDL_Color){255, 255, 255, 255}); start += height/28;
            FontManager_Render(renderer, "Monaco", height/18, width/2, start, "Disabling greatly increases FPS but uses more CPU.", Centered, Start, (SDL_Color){255, 255, 255, 255});
            break;
    }

    // Render settings
    start = height/2.5;
    char tempRenderStorage[64];
    snprintf(tempRenderStorage, 64, "Audio Buffer: %i", cfg->audioBuffer);
    int finalWidth = 0;
    int finalHeight = 0;
    SDL_Rect textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, tempRenderStorage, Centered, Centered, (SDL_Color){192, 192, 255, 255});
    if(selPos == 1)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    snprintf(tempRenderStorage, 64, "Sample Rate: %iHz", cfg->audioSample); start += height/20;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, tempRenderStorage, Centered, Centered, (SDL_Color){192, 192, 255, 255});
    if(selPos == 2)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    snprintf(tempRenderStorage, 64, "Vertical Sync: %s", cfg->verticalSync==1 ? "On" : "Off"); start += height/20;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, tempRenderStorage, Centered, Centered, (SDL_Color){192, 192, 255, 255});
    if(selPos == 3)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/8;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Start Game", Centered, Centered, (SDL_Color){192, 255, 192, 255});
    if(selPos == 4)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    
    // Render selector cursor
    int cursorOffset = height/48;
    if(fmod(time*1.5, 1) < 0.5)
        cursorOffset += height/128;
    FontManager_Render(renderer, "Silkscreen-Regular-mod", height/28, width/2-finalWidth/2-cursorOffset, finalHeight - height/256, ">", Centered, Centered, (SDL_Color){255, 255, 255, 255});

    if(self->isWSL)
        FontManager_Render(renderer, "Monaco", height/18, width/2, height-height/76, "WSL detected, audio quality may be degraded.", Centered, End, (SDL_Color){255, 127, 127, 255});

    if(exiting)
    {
        SDL_Rect rect = {0, 0, width, height};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        AudioManager_SetVolume(0, 0.25*(1-exitTime));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (int)(exitTime*255));
        SDL_RenderFillRect(renderer, &rect);
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}