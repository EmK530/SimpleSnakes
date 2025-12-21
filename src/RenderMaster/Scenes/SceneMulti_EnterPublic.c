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
#include "RenderMaster/Scenes/SceneMulti_EnterPublic.h"

static Multiplayer* mp = NULL;
static SharedConfig* cfg = NULL;

InitializedScene* SceneMulti_EnterPublic_Init()
{
    SceneMulti_EnterPublic* scene = malloc(sizeof(SceneMulti_EnterPublic));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneMulti_EnterPublic_Prepare;
    scene->sceneInfo->workFunction = SceneMulti_EnterPublic_Work;
    scene->sceneInfo->inputFunction = SceneMulti_EnterPublic_OnInput;

    cfg = SharedConfig_Get();
    mp = cfg->mp;

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_MULTI_ENTERPUBLIC;
    returnData->sceneName = strdup("Multiplayer_EnterPublic");

    return returnData;
}

int SceneMulti_EnterPublic_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneMulti_EnterPublic* self = (SceneMulti_EnterPublic*)_self;

    mp->listStatus = -1;
    self->deferredTimer = 0;
    self->listIndex = 0;
    self->selectingList = 0;
    self->topbarSelect = 0;
    self->abandon = 0;
    self->refreshCooldown = 0;
    self->time = 0;

    multiplayer_empty_list(mp);

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

int ScrollUp(SceneMulti_EnterPublic* self)
{
    if(!self->selectingList)
        return 1;
    if(self->listIndex == 0)
    {
        self->selectingList = 0;
    } else {
        self->listIndex--;
    }
    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.25, 2, 0);
    return 0;
}

int ScrollDown(SceneMulti_EnterPublic* self)
{
    if(!self->selectingList && mp->listStatus > 0 && mp->lobbyList->size != 0)
    {
        self->selectingList = 1;
    } else {
        if(self->listIndex >= (int)mp->lobbyList->size-1)
            return 1;
        self->listIndex++;
    }
    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.25, 2, 0);
    return 0;
}

int SceneMulti_EnterPublic_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneMulti_EnterPublic* self = (SceneMulti_EnterPublic*)_self;

    if (event->type == SDL_KEYDOWN)
    {
        switch(event->key.keysym.sym)
        {
            case W:
            case ArrowUp:
            {
                if(ScrollUp(self))
                    return 1;
                break;
            }

            case S:
            case ArrowDown:
            {
                if(ScrollDown(self))
                    return 1;
                break;
            }

            case A:
            case ArrowLeft:
            {
                if(!self->selectingList && self->topbarSelect == 1)
                {
                    self->topbarSelect = 0;
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.25, 2, 0);
                }
                break;
            }

            case D:
            case ArrowRight:
            {
                if(!self->selectingList && self->topbarSelect == 0)
                {
                    self->topbarSelect = 1;
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.25, 2, 0);
                }
                break;
            }

            case Escape:
            {
                if(!self->selectingList)
                    return 1;
                self->selectingList = 0;
                self->topbarSelect = 0;
                AudioManager_PlayCh("assets/audio/sfx/pause_close_esc.ogg", 0.5, 2, 0);
                break;
            }

            case Enter:
            {
                if(!self->selectingList)
                {
                    if(self->topbarSelect == 0)
                    {
                        AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.333, 2, 0);
                        self->abandon = SCENE_MULTI;;
                        return 1;
                    } else {
                        if(self->deferredTimer > 2 && mp->listStatus != -1 && self->time-self->refreshCooldown > 1)
                        {
                            self->deferredTimer = 0;
                            multiplayer_empty_list(mp);
                            AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.333, 2, 0);
                        } else {
                            AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 2, 0);
                        }
                    }
                } else {
                    LobbyInfo* nfo = (LobbyInfo*)LinkedList_get_index(mp->lobbyList, self->listIndex)->item;
                    if(nfo == NULL)
                    {
                        AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 2, 0);
                        return 1;
                    }
                    AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.333, 2, 0);
                    int result = multiplayer_join(mp, nfo->id, nfo->name, nfo->playerNames);
                    if(result != MPAPI_OK)
                    {
                        AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 2, 0);
                        LinkedList_pop(mp->lobbyList, self->listIndex, multiplayer_free_lobby);
                        int lobbyCount = (int)mp->lobbyList->size;
                        if(lobbyCount == 0)
                        {
                            self->selectingList = 0;
                        } else if(self->listIndex >= lobbyCount)
                        {
                            self->listIndex = lobbyCount-1;
                        }
                    } else {
                        self->abandon = SCENE_MULTI_LOBBY;
                        return 1;
                    }
                }
                break;
            }
        }
    } else if(event->type == SDL_MOUSEWHEEL)
    {
        if(event->wheel.y > 0)
        {
            ScrollUp(self);
        } else {
            ScrollDown(self);
        }
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)event;
    return 1;
}

int SceneMulti_EnterPublic_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    static double menuScroll = 0;
    SceneMulti_EnterPublic* self = (SceneMulti_EnterPublic*)_self;
    self->time += deltaTime;

    if(self->abandon)
        return self->abandon;

    self->deferredTimer++;
    if(self->deferredTimer == 2)
    {
        self->refreshCooldown = self->time;
        multiplayer_refresh_list(mp);
    }

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    DrawCellBackground(renderer, self->time, 3);

    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/40, "WorldLink", Centered, Start, (SDL_Color){48, 48, 48, 255});
    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/48, "WorldLink", Centered, Start, (SDL_Color){192, 192, 255, 255});

    int midX = width/2;
    int midY = height/2;

    int menuH = height/1.2;
    int menuW = menuH;

    SDL_Rect LobbyMenu = {midX-menuW/2,midY-menuH/2+height/30,menuW,menuH};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &LobbyMenu);
    OutlineRect(renderer, &LobbyMenu, height/90, 255, 255, 255);
    SDL_Rect backRect = FontManager_Render(renderer, "Monaco", height/16, LobbyMenu.x+height/48, LobbyMenu.y+height/56, "Back", Start, Start, (SDL_Color){255, 255, 255, 255});
    SDL_Rect refreshRect = FontManager_Render(renderer, "Monaco", height/16, LobbyMenu.x+LobbyMenu.w-height/64, LobbyMenu.y+height/56, "Refresh", End, Start, (SDL_Color){255, 255, 255, 255});

    if(!self->selectingList)
    {
        SDL_Rect* targetRect = &backRect;
        if(self->topbarSelect == 1)
            targetRect = &refreshRect;
        
        int adjustment = height/180;
        targetRect->x -= adjustment*1.5; targetRect->w += adjustment*2;
        targetRect->y -= adjustment; targetRect->h += adjustment*1.75;
        OutlineRect(renderer, targetRect, height/180, 255, 255, 255);
    }

    char lobbyCount[32];
    snprintf(lobbyCount, 32, "Lobbies: %zu", mp->lobbyList->size);
    FontManager_Render(renderer, "Monaco", height/16, LobbyMenu.x+LobbyMenu.w/2, LobbyMenu.y+height/50, lobbyCount, Centered, Start, (SDL_Color){255, 255, 255, 255});

    SDL_Rect LobbyList = {LobbyMenu.x, LobbyMenu.y+height/12, LobbyMenu.w, LobbyMenu.h-height/12};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &LobbyList);
    OutlineRect(renderer, &LobbyList, height/90, 255, 255, 255);

    int lobbyId = 0;
    int lobbyHeight = height/8;
    int offset = height/32;
    SDL_RenderSetClipRect(renderer, &LobbyList);

    double targetScroll = fmin(fmax(self->listIndex-2, 0), fmax(mp->lobbyList->size-4.75, 0)) * (lobbyHeight+offset);
    menuScroll += (targetScroll - menuScroll) * (1 - exp(-20 * deltaTime));

    LinkedList_foreach(mp->lobbyList, node)
    {
        LobbyInfo* nfo = (LobbyInfo*)node->item;
        SDL_Rect lobbyEntry = {LobbyList.x+height/48, LobbyList.y+height/48+((lobbyHeight+offset)*lobbyId)-menuScroll, LobbyList.w-height/24, lobbyHeight};
        if(lobbyEntry.y+lobbyEntry.h > 0)
        {
            // Draw lobby box
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &lobbyEntry);
            if(self->selectingList && lobbyId == self->listIndex)
            {
                OutlineRect(renderer, &lobbyEntry, height/180, 255, 192, 128);
            } else {
                OutlineRect(renderer, &lobbyEntry, height/180, 96, 96, 96);
            }

            // Draw lobby info
            char playerCount[32];
            snprintf(playerCount, 32, "Players: %i/4", nfo->playerCount);
            FontManager_Render(renderer, "Monaco", height/16, lobbyEntry.x+height/128, lobbyEntry.y+height/192, nfo->name, Start, Start, (SDL_Color){192, 255, 192, 255});
            FontManager_Render(renderer, "Monaco", height/16, lobbyEntry.x+lobbyEntry.w-height/256, lobbyEntry.y+height/192, playerCount, End, Start, (SDL_Color){192, 192, 255, 255});
            FontManager_Render(renderer, "Monaco", height/24, lobbyEntry.x+lobbyEntry.w-height/256, lobbyEntry.y+lobbyEntry.h-height/192, "Press Enter to join!", End, End, (SDL_Color){255, 255, 255, 255});
            FontManager_Render(renderer, "Monaco", height/24, lobbyEntry.x+height/192, lobbyEntry.y+lobbyEntry.h, nfo->id, Start, End, (SDL_Color){64, 64, 64, 255});
        }
        lobbyId++;
        if(lobbyEntry.y > height)
            break;
    }
    SDL_RenderSetClipRect(renderer, NULL);

    if(mp->listStatus > 1)
    {
        char errorCode[48];
        char* text = multiplayer_err_stringify(mp->listStatus-1);
        snprintf(errorCode, 48, "MpApi Error: %s", text);
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/2, errorCode, Centered, Centered, (SDL_Color){255, 128, 128, 255});
    } else if(mp->listStatus == -1)
    {
        FontManager_Render(renderer, "Monaco", height/12, width/2, height/2, "Connecting...", Centered, Centered, (SDL_Color){255, 255, 255, 255});
        return -1;
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}