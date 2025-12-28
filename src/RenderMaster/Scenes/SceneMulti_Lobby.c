#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Utils.h"
#include "SharedConfig.h"
#include "LinkedList.h"
#include "StaticConfig.h"

#include "Effects/TrippyBackground.h"

#include "RenderMaster/AudioManager.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/SceneUtils.h"
#include "RenderMaster/Scenes/SceneMulti_Lobby.h"

static Multiplayer* mp = NULL;
static SharedConfig* cfg = NULL;

InitializedScene* SceneMulti_Lobby_Init()
{
    SceneMulti_Lobby* scene = malloc(sizeof(SceneMulti_Lobby));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneMulti_Lobby_Prepare;
    scene->sceneInfo->workFunction = SceneMulti_Lobby_Work;
    scene->sceneInfo->inputFunction = SceneMulti_Lobby_OnInput;

    cfg = SharedConfig_Get();
    mp = cfg->mp;

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_MULTI_LOBBY;
    returnData->sceneName = strdup("Multiplayer_Lobby");

    return returnData;
}

static void on_mpapi_event(const char *event, int64_t messageId, const char *clientId, json_t *data, void *context)
{
    SceneMulti_Lobby* self = (SceneMulti_Lobby*)context;
    self->lastPacket = 0;
    printf("[SceneMulti_Lobby] Event #%li received: %s\n", messageId, event);
    if(strcmp(event, "joined") == 0)
    {
        PlayerInfo* plr = CreatePlayer((char*)clientId);
        char* username = (char*)json_string_value(json_object_get(data, "name"));
        plr->playerName = strdup(username);
        LinkedList_append(mp->playerList, (void*)plr);
    } else if(strcmp(event, "game") == 0)
    {
        json_t* type = json_object_get(data, "packet");
        const char* tp = json_string_value(type);
        if(strcmp(tp, "ready") == 0) {
            PlayerInfo* info = FindPlayer(mp, clientId);
            if(info != NULL)
            {
                info->ready = 1;
            }
        } else if(strcmp(tp, "ack") == 0) {
            PlayerInfo* info = FindPlayer(mp, clientId);
            if(info != NULL)
            {
                info->ackDelay = 0;
            }
        } else if(strcmp(tp, "ping") == 0) {
            json_t* lobby = json_object();
            json_object_set_new(lobby, "packet", json_string("ack"));
            mpapi_game(mp->mpapi, lobby, clientId);
        } else if(strcmp(tp, "unready") == 0) {
            PlayerInfo* info = FindPlayer(mp, clientId);
            if(info != NULL)
            {
                info->ready = 0;
            }
        } else if(strcmp(tp, "start") == 0) {
            self->abandon = SCENE_GAME;
        } else if(strcmp(tp, "disconnect") == 0 || strcmp(tp, "kick") == 0) {
            const char* target = clientId;
            if(strcmp(tp, "kick") == 0)
                target = json_string_value(json_object_get(data, "id"));
            Node* node = mp->playerList->head;
            while(node != NULL)
            {
                PlayerInfo* info = (PlayerInfo*)node->item;
                if(strcmp(info->playerId, target) == 0 || strcmp(info->playerId, mp->curClientId) == 0)
                {
                    if(info->host)
                    {
                        mpapi_destroy(mp->mpapi);
                        mp->mpapi = mpapi_create(MP_API_HOST, MP_API_PORT, MP_API_IDENT);
                        mp->inLobby = 0;
                        mp->isHost = 0;
                        mp->timeout = strcmp(info->playerId, target) == 0 ? 2 : 3;
                        self->abandon = SCENE_MULTI;
                        break;
                    }
                    LinkedList_remove(mp->playerList, node, multiplayer_free_player);
                    break;
                }
                node = node->front;
            }
        } else if(strcmp(tp, "lobby") == 0)
        {
            if(mp->isHost)
            {
                // Send player list to client
                printf("[SceneMulti_Lobby] Sending lobby sync to clientId %s\n", clientId);
                json_t* response = json_object();
                json_object_set_new(response, "packet", json_string("lobbyRes"));
                json_object_set_new(response, "name", json_string(mp->lobbyName));
                json_t* innerData = json_array();

                LinkedList_foreach(mp->playerList, node)
                {
                    json_t* playerData = json_object();
                    PlayerInfo* info = (PlayerInfo*)node->item;
                    json_object_set_new(playerData, "id", json_string(info->playerId));
                    json_object_set_new(playerData, "ready", json_boolean(info->ready));
                    json_object_set_new(playerData, "host", json_boolean(info->host));
                    json_object_set_new(playerData, "playing", json_boolean(info->participating));
                    if(strcmp(info->playerId, clientId) == 0)
                    {
                        info->playerName = (char*)strdup(json_string_value(json_object_get(data, "self")));
                    }
                    json_object_set_new(playerData, "name", json_string(info->playerName));
                    json_array_append(innerData, playerData);
                }

                json_object_set_new(response, "players", innerData);
                mpapi_game(mp->mpapi, response, clientId);

                json_decref(response);
            }
        } else if(strcmp(tp, "lobbyRes") == 0)
        {
            if(!mp->isHost)
            {
                printf("[SceneMulti_Lobby] Reading lobby sync...\n");

                json_t *jname = json_object_get(data, "name");
                if (json_is_string(jname))
                {
                    free(mp->lobbyName);
                    mp->lobbyName = strdup(json_string_value(jname));
                }

                json_t *players = json_object_get(data, "players");
                if (!json_is_array(players))
                    return;

                LinkedList *newList = LinkedList_create();

                size_t index;
                json_t *value;
                json_array_foreach(players, index, value)
                {
                    const char *id = json_string_value(json_object_get(value, "id"));
                    if (!id)
                        continue;

                    PlayerInfo *plr = NULL;

                    LinkedList_foreach(mp->playerList, node)
                    {
                        PlayerInfo *existing = (PlayerInfo *)node->item;
                        if (strcmp(existing->playerId, id) == 0)
                        {
                            plr = existing;
                            break;
                        }
                    }

                    if (!plr)
                    {
                        plr = CreatePlayer((char *)id);
                    }

                    plr->ready = json_boolean_value(json_object_get(value, "ready"));
                    plr->host  = json_boolean_value(json_object_get(value, "host"));
                    plr->participating = json_boolean_value(json_object_get(value, "playing"));

                    const char *name = json_string_value(json_object_get(value, "name"));
                    if (name)
                    {
                        free(plr->playerName);
                        plr->playerName = strdup(name);
                    }

                    if (strcmp(id, mp->curClientId) == 0)
                    {
                        mp->selfPlayer = plr;
                    }

                    LinkedList_append(newList, plr);
                }

                LinkedList_dispose(&mp->playerList, NULL);
                mp->playerList = newList;
            }
        }
    }
    (void)self;
}

int SceneMulti_Lobby_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneMulti_Lobby* self = (SceneMulti_Lobby*)_self;

    self->time = 0;
    self->abandon = -1;
    self->selPos = 1;
    self->allReady = 0;
    self->lastPacket = 0;

    if(mp->returning)
    {
        mp->returning = 0;
        LinkedList_foreach(mp->playerList, node)
        {
            PlayerInfo* info = (PlayerInfo*)node->item;
            info->ready = 0;
            info->participating = 0;
            info->loaded = 0;

            if(info->needsUpdate)
            {
                printf("[SceneMulti_Lobby] Sending lobby sync to waiting clientId %s\n", info->playerId);
                json_t* response = json_object();
                json_object_set_new(response, "packet", json_string("lobbyRes"));
                json_object_set_new(response, "name", json_string(mp->lobbyName));
                json_t* innerData = json_array();

                LinkedList_foreach(mp->playerList, node)
                {
                    json_t* playerData = json_object();
                    PlayerInfo* info = (PlayerInfo*)node->item;
                    json_object_set_new(playerData, "id", json_string(info->playerId));
                    json_object_set_new(playerData, "ready", json_boolean(info->ready));
                    json_object_set_new(playerData, "host", json_boolean(info->host));
                    json_object_set_new(playerData, "playing", json_boolean(info->participating));
                    json_object_set_new(playerData, "name", json_string(info->playerName));
                    json_array_append(innerData, playerData);
                }
                json_object_set_new(response, "players", innerData);
                mpapi_game(mp->mpapi, response, info->playerId);
            }

            info->needsUpdate = 0;
        }
    }

    LinkedList_foreach(mp->playerList, node)
    {
        PlayerInfo* info = (PlayerInfo*)node->item;
        info->ackDelay = 0;
    }

    cfg->listenerId = mpapi_listen(mp->mpapi, on_mpapi_event, self);

    if(!mp->isHost)
    {
        json_t* lobby = json_object();
        json_object_set_new(lobby, "packet", json_string("lobby"));
        json_object_set_new(lobby, "self", json_string(mp->username));
        mpapi_game(mp->mpapi, lobby, NULL);
    }

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

int SceneMulti_Lobby_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneMulti_Lobby* self = (SceneMulti_Lobby*)_self;

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
                if(self->selPos < 2) {
                    AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                    self->selPos++;
                }
                break;
            case Enter:
                if(self->selPos == 1)
                {
                    if(mp->isHost)
                    {
                        if(self->allReady && mp->playerList->size <= 4)
                        {
                            self->abandon = SCENE_GAME;
                            AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.33, 1, 0);
                        } else {
                            AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 1, 0);
                        }
                    } else {
                        if(mp->selfPlayer != NULL)
                        {
                            mp->selfPlayer->ready = !mp->selfPlayer->ready;
                            if(mp->selfPlayer->ready)
                            {
                                // Ready
                                AudioManager_PlayCh("assets/audio/sfx/ready.ogg", 0.333, 1, 0);
                                json_t* lobby = json_object();
                                json_object_set_new(lobby, "packet", json_string("ready"));
                                mpapi_game(mp->mpapi, lobby, NULL);
                            } else {
                                // Unready
                                AudioManager_PlayCh("assets/audio/sfx/unready.ogg", 0.333, 1, 0);
                                json_t* lobby = json_object();
                                json_object_set_new(lobby, "packet", json_string("unready"));
                                mpapi_game(mp->mpapi, lobby, NULL);
                            }
                        } else {
                            AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 1, 0);
                        }
                    }
                } else {
                    AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.33, 1, 0);
                    json_t* lobby = json_object();
                    json_object_set_new(lobby, "packet", json_string("disconnect"));
                    mpapi_game(mp->mpapi, lobby, NULL);
                    mpapi_destroy(mp->mpapi);
                    mp->mpapi = mpapi_create(MP_API_HOST, MP_API_PORT, MP_API_IDENT);
                    mp->inLobby = 0;
                    mp->isHost = 0;
                    self->abandon = SCENE_MULTI;
                }
        }
    }

    (void)self;
    (void)window;
    (void)renderer;
    (void)event;
    return 1;
}

int SceneMulti_Lobby_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    SceneMulti_Lobby* self = (SceneMulti_Lobby*)_self;
    self->time += deltaTime;

    if(!mp->inLobby)
        return SCENE_MULTI;

    if(self->abandon != -1)
        return self->abandon;

    if(!mp->isHost)
    {
        self->lastPacket += deltaTime;
        if(self->lastPacket > MP_API_TIMEOUT)
        {
            json_t* lobby = json_object();
            json_object_set_new(lobby, "packet", json_string("disconnect"));
            mpapi_game(mp->mpapi, lobby, NULL);
            mpapi_destroy(mp->mpapi);
            mp->mpapi = mpapi_create(MP_API_HOST, MP_API_PORT, MP_API_IDENT);
            mp->inLobby = 0;
            mp->isHost = 0;
            mp->timeout = 1;
            return SCENE_MULTI;
        }
    } else {
        Node* node = mp->playerList->head;
        while(node)
        {
            Node* nextItem = node->front;
            PlayerInfo* info = (PlayerInfo*)node->item;
            info->ackDelay += deltaTime;
            if(!info->host && info->ackDelay > MP_API_TIMEOUT)
            {
                json_t* lobby = json_object();
                json_object_set_new(lobby, "packet", json_string("kick"));
                json_object_set_new(lobby, "id", json_string(info->playerId));
                mpapi_game(mp->mpapi, lobby, NULL);
                LinkedList_remove(mp->playerList, node, multiplayer_free_player);
            }
            node = nextItem;
        }
    }

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    DrawTrippyBackground(renderer, deltaTime);

    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/40, mp->lobbyName, Centered, Start, (SDL_Color){48, 48, 48, 255});
    FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/48, mp->lobbyName, Centered, Start, (SDL_Color){255, 192, 128, 255});
    char code[32];
    snprintf(code, 32, "Code: %s", mp->curSession);
    FontManager_Render(renderer, "Monaco", height/16, width/2, height/12, code, Centered, Start, (SDL_Color){127, 127, 127, 255});

    char playerText[32];
    snprintf(playerText, 32, "Players (%zu):", mp->playerList->size);
    SDL_Rect rect = FontManager_Render(renderer, "Monaco", height/16, height/45, height/8, playerText, Start, Centered, (SDL_Color){255, 255, 255, 255});
    int yPos = rect.y + rect.h + height/45;
    self->allReady = mp->playerList->size > 1;
    LinkedList_foreach(mp->playerList, node)
    {
        PlayerInfo* plr = (PlayerInfo*)node->item;
        if(!plr->participating)
        {
            if(plr->host)
            {
                rect = FontManager_Render(renderer, "Monaco", height/16, height/45, yPos, "[Host]", Start, Centered, (SDL_Color){255, 192, 128, 255});
            } else {
                if(!plr->ready)
                {
                    self->allReady = 0;
                    rect = FontManager_Render(renderer, "Monaco", height/16, height/45, yPos, "[Not Ready]", Start, Centered, (SDL_Color){255, 192, 192, 255});
                } else {
                    rect = FontManager_Render(renderer, "Monaco", height/16, height/45, yPos, "[Ready]", Start, Centered, (SDL_Color){192, 255, 192, 255});
                }
            }
        } else {
            rect = FontManager_Render(renderer, "Monaco", height/16, height/45, yPos, "[In-Game]", Start, Centered, (SDL_Color){164, 164, 255, 255});
        }
        rect = FontManager_Render(renderer, "Monaco", height/16, height/30 + rect.w, yPos, plr->playerName, Start, Centered, (SDL_Color){255, 255, 255, 255});
        yPos += rect.h + height/90;
    }

    int start = height/2;

    int finalWidth = 0;
    int finalHeight = 0;
    SDL_Rect textRect;
    if(mp->isHost)
    {
        int R = 255;
        int G = 164;
        int B = 164;
        if(self->allReady && mp->playerList->size <= 4)
        {
            R = 164;
            G = 255;
            B = 164;
        }
        textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Start Game", Centered, Centered, (SDL_Color){R, G, B, 255});
    } else {
        if(mp->selfPlayer != NULL)
        {
            if(mp->selfPlayer->ready)
            {
                textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Unready", Centered, Centered, (SDL_Color){255, 164, 164, 255});
            } else {
                textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, "Ready", Centered, Centered, (SDL_Color){164, 255, 164, 255});
            }
        }
    }
    if(self->selPos == 1)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }
    start += height/20;
    textRect = FontManager_Render(renderer, "Monaco", height/16, width/2, start, ((mp->selfPlayer != NULL && mp->selfPlayer->host) ? "Close Lobby" : "Leave Lobby"), Centered, Centered, (SDL_Color){255, 255, 255, 255});
    if(self->selPos == 2)
    {
        finalWidth = textRect.w;
        finalHeight = start;
    }

    int cursorOffset = height/48;
    if(fmod(self->time*1.5, 1) < 0.5)
        cursorOffset += height/128;
    FontManager_Render(renderer, "Silkscreen-Regular-mod", height/28, width/2-finalWidth/2-cursorOffset, finalHeight - height/256, ">", Centered, Centered, (SDL_Color){255, 255, 255, 255});

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}