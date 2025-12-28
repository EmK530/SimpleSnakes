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
#include "RenderMaster/Scenes/SceneScoreboard.h"

static SharedConfig* cfg = NULL;

InitializedScene* SceneScoreboard_Init()
{
    SceneScoreboard* scene = malloc(sizeof(SceneScoreboard));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneScoreboard_Prepare;
    scene->sceneInfo->workFunction = SceneScoreboard_Work;
    scene->sceneInfo->inputFunction = SceneScoreboard_OnInput;

    cfg = SharedConfig_Get();

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_SCOREBOARD;
    returnData->sceneName = strdup("Scoreboard");

    return returnData;
}

int SceneScoreboard_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneScoreboard* self = (SceneScoreboard*)_self;

    self->abandon = 0;

    AudioManager_PlayCh("assets/audio/mus/mus_record.ogg", 0.333, 0, -1);

    (void)self;
    (void)window;
    (void)renderer;
    (void)previousScene;
    return 1;
}

int SceneScoreboard_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneScoreboard* self = (SceneScoreboard*)_self;

    if (event->type == SDL_KEYDOWN)
    {
        switch(event->key.keysym.sym)
        {
            case Enter:
            {
                AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.333, 2, 0);
                self->abandon = SCENE_TITLE;
                return 1;
            }
        }
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)event;
    return 1;
}

int SceneScoreboard_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    static double time = 0;
    time += deltaTime;
    SceneScoreboard* self = (SceneScoreboard*)_self;

    if(self->abandon)
        return self->abandon;

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    DrawTrippyBackground(renderer, deltaTime);

    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/40, "Scoreboard", Centered, Start, (SDL_Color){64, 48, 32, 255});
    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/48, "Scoreboard", Centered, Start, (SDL_Color){255, 192, 128, 255});

    int midX = width/2;
    int midY = height/2;

    int menuH = height/1.2;
    int menuW = menuH;

    SDL_Rect LobbyMenu = {midX-menuW/2,midY-menuH/2+height/30,menuW,menuH};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &LobbyMenu);
    OutlineRect(renderer, &LobbyMenu, height/90, 255, 255, 255);
    SDL_Rect backRect = FontManager_Render(renderer, "Monaco", height/16, width/2, LobbyMenu.y+height/56, "Back", Centered, Start, (SDL_Color){255, 255, 255, 255});

    int adjustment = height/180;
    backRect.x -= adjustment*1.5; backRect.w += adjustment*2;
    backRect.y -= adjustment; backRect.h += adjustment*1.75;
    OutlineRect(renderer, &backRect, height/180, 255, 255, 255);

    SDL_Rect LobbyList = {LobbyMenu.x, LobbyMenu.y+height/12, LobbyMenu.w, LobbyMenu.h-height/12};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &LobbyList);
    OutlineRect(renderer, &LobbyList, height/90, 255, 255, 255);

    if(cfg->scoreboard->records->size == 0)
    {
        FontManager_Render(renderer, "Monaco", height/16, width/2, height/2, "No records found!", Centered, Start, (SDL_Color){255, 255, 255, 255});
    } else {
        int start = LobbyList.y + height/80;
        int index = 0;
        LinkedList_foreach(cfg->scoreboard->records, node)
        {
            index++;
            ScoreRecord* record = (ScoreRecord*)node->item;
            char entry[76];
            snprintf(entry, 76, "[#%i] %s: %i Score", index, record->name, record->score);
            SDL_Rect r1 = FontManager_Render(renderer, "Monaco", height/16, LobbyList.x+height/80, start, entry, Start, Start, (SDL_Color){255, 255, 255, 255});
            start += r1.h + height/80;
        }
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}