#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Utils.h"
#include "SharedConfig.h"
#include "LinkedList.h"

#include "Effects/TrippyBackground.h"

#include "RenderMaster/AudioManager.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/SceneUtils.h"
#include "RenderMaster/Scenes/SceneRecord.h"

static Multiplayer* mp = NULL;
static SharedConfig* cfg = NULL;

InitializedScene* SceneRecord_Init()
{
    SceneRecord* scene = malloc(sizeof(SceneRecord));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneRecord_Prepare;
    scene->sceneInfo->workFunction = SceneRecord_Work;
    scene->sceneInfo->inputFunction = SceneRecord_OnInput;

    cfg = SharedConfig_Get();
    mp = cfg->mp;

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_RECORD;
    returnData->sceneName = strdup("Record");

    return returnData;
}

int SceneRecord_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneRecord* self = (SceneRecord*)_self;

    mp->listStatus = -1;
    self->time = 0;
    self->selPos = 1;
    self->abandon = -1;
    self->publicSetting = 1;
    self->typePos = 0;
    memset(self->nameInput, '\0', 25);
    
    AudioManager_PlayCh("assets/audio/mus/mus_record.ogg", 0.333, 0, -1);

    (void)self;
    (void)window;
    (void)renderer;
    (void)previousScene;
    return 1;
}

int SceneRecord_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneRecord* self = (SceneRecord*)_self;

    if(self->selPos == 1 && event->type == SDL_TEXTINPUT)
    {
        int textLen = strlen(event->text.text);
        if(textLen == 1 && self->typePos + textLen <= 24)
        {
            memcpy(self->nameInput+self->typePos, event->text.text, textLen);
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
                if(self->selPos < 3) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    self->selPos++;
                }
                break;

            case Enter:
            {
                switch(self->selPos)
                {
                    case 2: // Submit
                    {
                        if(strlen(self->nameInput) == 0)
                        {
                            AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 1, 0);
                            break;
                        }
                        Scoreboard_add(cfg->scoreboard, self->nameInput, cfg->scoreFromGame, "scoreboard.json");
                        AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        self->abandon = SCENE_TITLE;
                        break;
                    }
                    case 3: // Back
                    {
                        AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                        self->abandon = SCENE_TITLE;
                        break;
                    }
                }
                break;
            }

            case Backspace:
            {
                if(self->selPos == 1 && self->typePos > 0)
                {
                    self->nameInput[--self->typePos] = '\0';
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

int SceneRecord_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    SceneRecord* self = (SceneRecord*)_self;
    self->time += deltaTime;

    if(self->abandon != -1)
        return self->abandon;

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    DrawTrippyBackground(renderer, deltaTime);

    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/40, "New   Record!", Centered, Start, (SDL_Color){48, 64, 48, 255});
    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/48, "New   Record!", Centered, Start, (SDL_Color){192, 255, 192, 255});
    FontManager_Render(renderer, "Monaco", height/16, width/2, height/10, "Your score is qualified for the scoreboard!", Centered, Centered, (SDL_Color){255, 255, 255, 255});
    FontManager_Render(renderer, "Monaco", height/16, width/2, height/7, "Please enter a name to submit it.", Centered, Centered, (SDL_Color){255, 255, 255, 255});

    char format[32];
    snprintf(format, 32, "Your score: %i", cfg->scoreFromGame);
    FontManager_Render(renderer, "Monaco", height/16, width/2, height/4, format, Centered, Centered, (SDL_Color){255, 255, 255, 255});

    FontManager_Render(renderer, "Monaco", height/16, width/2, height/2.95, "Your Name", Centered, Centered, (SDL_Color){255, 255, 255, 255});
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
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/2.625, self->nameInput, Centered, Start, (SDL_Color){255, 255, 255, 255});
    }

    int start = height/2;

    int finalWidth = 0;
    int finalHeight = 0;
    SDL_Rect textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Submit record", Centered, Centered, (SDL_Color){164, 255, 164, 255});
    if(self->selPos == 2)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/12;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Do not submit", Centered, Centered, (SDL_Color){255, 164, 164, 255});
    if(self->selPos == 3)
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