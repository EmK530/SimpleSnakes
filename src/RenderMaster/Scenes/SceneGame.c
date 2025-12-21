#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "mpapi.h"
#include "Utils.h"
#include "easings.h"
#include "GameManager.h"
#include "Multiplayer.h"
#include "SharedConfig.h"
#include "StaticConfig.h"
#include "Effects/TileBackground.h"

#include "RenderMaster/FontManager.h"
#include "RenderMaster/AudioManager.h"
#include "RenderMaster/SceneUtils.h"
#include "RenderMaster/Scenes/SceneGame.h"

static GameManager* game = NULL;
static Multiplayer* mp = NULL;
static SharedConfig* cfg = NULL;

InitializedScene* SceneGame_Init()
{
    SceneGame* scene = malloc(sizeof(SceneGame));
    scene->sceneInfo = malloc(sizeof(GenericScene));
    scene->sceneInfo->context = scene;
    scene->sceneInfo->prepFunction = SceneGame_Prepare;
    scene->sceneInfo->workFunction = SceneGame_Work;
    scene->sceneInfo->inputFunction = SceneGame_OnInput;

    cfg = SharedConfig_Get();
    mp = cfg->mp;

    InitializedScene* returnData = malloc(sizeof(InitializedScene));
    returnData->scene = scene->sceneInfo;
    returnData->sceneId = SCENE_GAME;
    returnData->sceneName = strdup("Game");

    return returnData;
}

static void on_mpapi_event(const char *event, int64_t messageId, const char *clientId, json_t *data, void *context)
{
    SceneGame* self = (SceneGame*)context;
    self->lastPacket = 0;
    printf("[SceneGame] Event #%li received: %s\n", messageId, event);
    if(strcmp(event, "joined") == 0)
    {
        if(mp->isHost)
        {
            PlayerInfo* plr = CreatePlayer((char*)clientId);
            char* username = (char*)json_string_value(json_object_get(data, "name"));
            plr->playerName = strdup(username);
            LinkedList_append(mp->playerList, (void*)plr);
        }
    } else if(strcmp(event, "game") == 0)
    {
        json_t* type = json_object_get(data, "packet");
        const char* tp = json_string_value(type);
        if(strcmp(tp, "count") == 0) {
            self->lastCountId = -1;
            self->ready = 1;
        } else if(strcmp(tp, "loaded") == 0) {
            if(mp->isHost)
            {
                self->loadedCount++;
                if(self->loadedCount >= game->playerCounts)
                {
                    self->ready = 1; // TODO wait for all participants
                    json_t* packet = (json_t*)CreateGameStatePacket(game);
                    mpapi_game(mp->mpapi, packet, NULL);
                }
            }
        } else if(strcmp(tp, "tick") == 0) {
            LoadGameStatePacket(game, data);
            if(!self->mpHasDied)
            {
                if(!game->livingStatus[self->myPlayerIndex])
                {
                    self->mpHasDied = 1;
                    AudioManager_PlayCh("assets/audio/sfx/collide.ogg", 0.25, 3, 0);
                }
            }
            if(self->mpLastLen != game->snakeLens[self->myPlayerIndex])
            {
                AudioManager_PlayCh("assets/audio/sfx/apple.ogg", 0.75, 2, 0);
                self->mpLastLen = game->snakeLens[self->myPlayerIndex];
            }
        } else if(strcmp(tp, "ended") == 0) {
            AudioManager_StopCh(0);
            self->gameEnded = 1;
            self->winnerId = json_integer_value(json_object_get(data, "winner"));
        } else if(strcmp(tp, "disconnect") == 0 || strcmp(tp, "kick") == 0) {
            const char* target = clientId;
            if(strcmp(tp, "kick") == 0)
                target = json_string_value(json_object_get(data, "id"));
            Node* node = mp->playerList->head;
            while(node != NULL)
            {
                PlayerInfo* info = (PlayerInfo*)node->item;
                if(strcmp(info->playerId, target) == 0)
                {
                    if(info->host)
                    {
                        mpapi_destroy(mp->mpapi);
                        mp->mpapi = mpapi_create(MP_API_HOST, MP_API_PORT, MP_API_IDENT);
                        mp->inLobby = 0;
                        mp->isHost = 0;
                        mp->timeout = 2;
                        self->abandon = SCENE_MULTI;
                        break;
                    }
                    LinkedList_remove(mp->playerList, node, multiplayer_free_player);
                    break;
                }
                node = node->front;
            }
        } else if(strcmp(tp, "lobby") == 0) {
            if(mp->isHost)
            {
                // Send player list to client
                printf("[SceneGame] Sending lobby sync to clientId %s\n", clientId);
                PlayerInfo* plr = FindPlayer(mp, clientId);
                plr->needsUpdate = 1;
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
        } else if(strcmp(tp, "input") == 0) {
            if(mp->isHost)
            {
                // find requester index god I hate this design lololol
                int index = 0;
                int realIndex = -1;
                LinkedList_foreach(mp->playerList, node)
                {
                    PlayerInfo* info = (PlayerInfo*)node->item;
                    if(strcmp(info->playerId, clientId) == 0)
                    {
                        realIndex = index;
                        break;
                    }
                    if(info->participating)
                        index++;
                }
                if(realIndex != -1)
                {
                    if (game->inputCounts[realIndex] < 2)
                    {
                        game->inputQueues[realIndex][game->inputCounts[realIndex]++] = (int16_t)json_integer_value(json_object_get(data, "direction"));
                    }
                }
            }
        }
    }

    (void)self;
}

int SceneGame_Prepare(void* _self, SDL_Window* window, SDL_Renderer* renderer, int previousScene)
{
    SceneGame* self = (SceneGame*)_self;

    self->gameStarted = 0;
    self->gamePaused = 0;
    self->gameEnded = 0;
    self->winnerId = -1;
    self->loseTimer = 0;
    self->playedLoseSound = 0;
    self->lastPacket = 0;

    cfg->isMpMusicPlaying = 0;

    self->mpLastLen = 4;
    self->mpHasDied = 0;
    self->countId = 0;
    self->lastCountId = 0;
    self->countDelta = -0.25;
    self->loadedCount = 1;
    self->abandon = 0;
    self->framesPlayed = 0;
    self->pauseSel = 0;

    int participants = 0;
    self->multiplayerNames = calloc(4, sizeof(char*));
    if(mp->inLobby)
    {
        int index = 0;
        LinkedList_foreach(mp->playerList, node)
        {
            PlayerInfo* info = (PlayerInfo*)node->item;
            if(strcmp(info->playerId, mp->curClientId) == 0)
            {
                self->myPlayerIndex = index;
            }
            if(info->ready || info->host)
            {
                self->multiplayerNames[index] = strdup(info->playerName);
                info->participating = 1;
                participants++;
                index++;
            }
        }
        self->ready = 0;
        mpapi_unlisten(mp->mpapi, cfg->listenerId);
        cfg->listenerId = mpapi_listen(mp->mpapi, on_mpapi_event, self);
        if(mp->isHost)
        {
            //self->myPlayerIndex = 0;
            json_t* lobby = json_object();
            json_object_set_new(lobby, "packet", json_string("start"));
            mpapi_game(mp->mpapi, lobby, NULL);
        } else {
            json_t* lobby = json_object();
            json_object_set_new(lobby, "packet", json_string("loaded"));
            mpapi_game(mp->mpapi, lobby, NULL);
        }
    } else {
        self->ready = 1;
        self->myPlayerIndex = 0;
        participants = cfg->localMultiplayer ? 2 : 1;
    }

    if(cfg->localMultiplayer)
    {
        for(int i = 0; i < participants; i++)
        {
            char* temp = malloc(sizeof(char) * 18);
            snprintf(temp, sizeof(char) * 18, "Player %i", i+1);
            self->multiplayerNames[i] = temp;
        }
    }
    
    int size = 14;
    if(cfg->localMultiplayer || mp->inLobby)
        size = 16 + 2 * participants;
    game = GameManager_create(size, size, participants);

    AudioManager_StopAll();
    char battleTheme[64];
    snprintf(battleTheme, 64, "assets/audio/mus/mus_battle%i.ogg", (rand()%4)+1);
    //snprintf(battleTheme, 64, "assets/audio/mus/mus_battle%i.ogg", 4);
    AudioManager_PlayCh(battleTheme, 0.33, 0, -1);

    (void)self;
    (void)window;
    (void)renderer;
    (void)previousScene;
    return 1;
}

int SceneGame_OnInput(void* _self, SDL_Window* window, SDL_Renderer* renderer, SDL_Event* event)
{
    SceneGame* self = (SceneGame*)_self;

    if (event->type == SDL_KEYDOWN)
    {
        int16_t newDir = -30000;

        if(self->gamePaused)
        {
            switch(event->key.keysym.sym)
            {
                case A:
                case ArrowLeft:
                {
                    if(self->pauseSel > 0)
                    {
                        self->pauseSel--;
                        AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.5, 2, 0);
                    }
                    break;
                }
                case D:
                case ArrowRight:
                {
                    if(self->pauseSel < 2)
                    {
                        self->pauseSel++;
                        AudioManager_PlayCh("assets/audio/sfx/config_cursor.ogg", 0.5, 2, 0);
                    }
                    break;
                }
                case Enter:
                {
                    AudioManager_PlayCh("assets/audio/sfx/config_select.ogg", 0.5, 2, 0);
                    if(self->pauseSel == 0) {
                        self->gamePaused = 0;
                        AudioManager_SetVolume(0, 0.33);
                    } else if(self->pauseSel == 1) {
                        self->abandon = SCENE_GAME;
                    } else if(self->pauseSel == 2) {
                        self->abandon = SCENE_TITLE;
                    }
                    return 1;
                }
            }
        }

        int player = self->myPlayerIndex;
        switch (event->key.keysym.sym)
        {
            case ArrowUp: case W:    newDir = EncodeDelta(0, -1); break;
            case ArrowDown: case S:  newDir = EncodeDelta(0,  1); break;
            case ArrowLeft: case A:  newDir = EncodeDelta(-1, 0); break;
            case ArrowRight: case D: newDir = EncodeDelta( 1, 0); break;
            case I: if(cfg->localMultiplayer) { newDir = EncodeDelta(0, -1); player = 1; } break;
            case K: if(cfg->localMultiplayer) { newDir = EncodeDelta(0,  1); player = 1; } break;
            case J: if(cfg->localMultiplayer) { newDir = EncodeDelta(-1, 0); player = 1; } break;
            case L: if(cfg->localMultiplayer) { newDir = EncodeDelta( 1, 0); player = 1; } break;
            case Escape:
            {
                if(!mp->inLobby)
                {
                    self->gamePaused = !self->gamePaused;
                    if(self->gamePaused) {
                        self->pauseSel = 0;
                        AudioManager_PlayCh("assets/audio/sfx/pause_open_esc.ogg", 0.5, 2, 0);
                        AudioManager_SetVolume(0, 0.05);
                    } else {
                        AudioManager_PlayCh("assets/audio/sfx/pause_close_esc.ogg", 0.5, 2, 0);
                        AudioManager_SetVolume(0, 0.33);
                    }
                } else {
                    AudioManager_PlayCh("assets/audio/sfx/denied.ogg", 0.5, 2, 0);
                }
                return 1;
            }
            case R:
            {
                if(!mp->inLobby && self->framesPlayed > 3)
                {
                    self->abandon = SCENE_GAME;
                    return 1;
                }
                break;
            }
            default:
                return 1;
        }

        if(!self->gameStarted || newDir == -30000)
        {
            return 1;
        }

        // Reject reverse against current or queued direction
        int16_t lastDir = (game->inputCounts[player] > 0)
            ? game->inputQueues[player][game->inputCounts[player] - 1]
            : game->moveDirections[player];

        if ((!mp->inLobby || mp->isHost) && IsOpposite(newDir, lastDir))
            return 1;

        // Enqueue if space
        if(!mp->inLobby || mp->isHost)
        {
            if (game->inputCounts[player] < 2)
            {
                game->inputQueues[player][game->inputCounts[player]++] = newDir;
            }
        } else {
            json_t* lobby = json_object();
            json_object_set_new(lobby, "packet", json_string("input"));
            json_object_set_new(lobby, "direction", json_integer(newDir));
            mpapi_game(mp->mpapi, lobby, NULL);
        }
    }

    (void)self;
    (void)window;
    (void)renderer;
    return 1;
}

void DrawSnake(void* _self, SDL_Renderer* renderer, SDL_Rect stageRect, int width, int height)
{
    SceneGame* self = (SceneGame*)_self;

    int cellW = stageRect.w / game->gameWidth;
    int cellH = stageRect.h / game->gameHeight;
    int gridThickness = cellH / 6;

    int reduceSegments = 4;
    for(int a = 0; a < game->playerCounts; a++)
    {
        double colorReduce = 0.5 / (double)clampi(game->snakeLens[a],1,reduceSegments);

        uint16_t* pos = game->snakeDataList[a];
        uint16_t lastPos = 65535;

        double deadDiv = 1.0;
        if(!game->livingStatus[a])
            deadDiv = 2.0;

        for(int i = 0; i < game->snakeLens[a]; i++)
        {
            uint16_t posValue = *pos;
            int x = posValue & 0xFF;
            int y = posValue >> 8;

            int rx = stageRect.x + x * cellW + gridThickness;
            int ry = stageRect.y + y * cellH + gridThickness;
            int rw = cellW - gridThickness * 2;
            int rh = cellH - gridThickness * 2;

            double reduceMul = colorReduce * (double)clampi(game->snakeLens[a]-1-i,0,reduceSegments);
            int R = 255;
            int G = 255;
            int B = 255;
            if(a != self->myPlayerIndex || cfg->localMultiplayer)
            {
                switch(a)
                {
                    case 0:
                        R = 255; G = 164; B = 164; break;
                    case 1:
                        R = 164; G = 255; B = 164; break;
                    case 2:
                        R = 164; G = 164; B = 255; break;
                    case 3:
                        R = 255; G = 255; B = 164; break;
                }
            }
            SDL_SetRenderDrawColor(renderer, (R-R*(reduceMul))/deadDiv, (G-G*(reduceMul))/deadDiv, (B-B*(reduceMul))/deadDiv, 255);

            if(lastPos == 65535)
            {
                SDL_Rect r = {rx, ry, rw, rh};
                SDL_RenderFillRect(renderer, &r);
            } else {
                int lastX = lastPos & 0xFF;
                int lastY = lastPos >> 8;
                SDL_Rect r;
                if(lastX < x) {
                    r = (SDL_Rect){rx - cellW, ry, rw + cellW, rh};
                } else if(lastX > x) {
                    r = (SDL_Rect){rx, ry, rw + cellW, rh};
                } else if(lastY < y) {
                    r = (SDL_Rect){rx, ry - cellH, rw, rh + cellH};
                } else if(lastY > y) {
                    r = (SDL_Rect){rx, ry, rw, rh + cellH};
                }
                SDL_RenderFillRect(renderer, &r);
            }
            lastPos = *pos;
            pos++;
        }
    }

    (void)width;
    (void)height;
}

void DrawApples(SDL_Renderer* renderer, SDL_Rect stageRect, int width, int height)
{
    int gridThickness = height/160;
    int cellW = stageRect.w / game->gameWidth;
    int cellH = stageRect.h / game->gameHeight;
    SDL_SetRenderDrawColor(renderer, 255, 32, 32, 255);
    uint16_t* pos = game->allAppleData;

    for(int i = 0; i < game->appleCount; i++)
    {
        uint16_t posValue = *pos++;
        int x = posValue & 0xFF;
        int y = posValue >> 8;

        int rx = stageRect.x + x * cellW + gridThickness;
        int ry = stageRect.y + y * cellH + gridThickness;
        int rw = cellW - gridThickness * 1.5;
        int rh = cellH - gridThickness * 1.5;

        SDL_Rect r = {rx, ry, rw, rh};
        SDL_RenderFillRect(renderer, &r);
    }

    (void)width;
}

int RenderGame(void* _self, SDL_Renderer* renderer, int width, int height, double deltaTime)
{
    SceneGame* self = (SceneGame*)_self;

    double doubleWCells = (double)game->gameWidth;
    double doubleHCells = (double)game->gameHeight;
    double aspectRatio = doubleWCells/doubleHCells;
    int StageSizeH = floor((((double)height)/1.08)/doubleHCells)*doubleHCells;
    int StageSizeW = ((double)StageSizeH)*aspectRatio;

    int lineThickness = height/80;
    SDL_Rect stageRect = {width/2-StageSizeW/2, height-StageSizeH-lineThickness, StageSizeW, StageSizeH};
    OutlineRect(renderer, &stageRect, lineThickness, 255, 255, 255);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 8, 8, 255);
    SDL_RenderFillRect(renderer, &stageRect);
    int gridThickness = height/320;
    for(int i = 1; i < game->gameWidth; i++)
    {
        int posOffset = i*(StageSizeW/game->gameWidth);
        SDL_Rect rect = {stageRect.x+posOffset, stageRect.y, gridThickness, stageRect.h};
        SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
    for(int i = 1; i < game->gameHeight; i++)
    {
        int posOffset = i*(StageSizeH/game->gameHeight);
        SDL_Rect rect = {stageRect.x, stageRect.y+posOffset, stageRect.w, gridThickness};
        SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    DrawSnake(self, renderer, stageRect, width, height);

    DrawApples(renderer, stageRect, width, height);

    (void)deltaTime;

    return StageSizeH;
}

// https://gamedev.stackexchange.com/a/210948
bool HasWindowFocus(SDL_Window* window)
{
    uint32_t flags = SDL_GetWindowFlags(window);
    return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

int SceneGame_Work(void* _self, SDL_Window* window, SDL_Renderer* renderer, double deltaTime)
{
    static double time = 0;
    time += deltaTime;

    SceneGame* self = (SceneGame*)_self;

    if(self->abandon > 0)
    {
        if(mp->inLobby)
            mpapi_unlisten(mp->mpapi, cfg->listenerId);
        GameManager_destroy(&game);
        return self->abandon;
    }

    if(!mp->inLobby && !self->gamePaused)
    {
        if(!HasWindowFocus(window))
        {
            self->gamePaused = true;
            self->pauseSel = 0;
            AudioManager_SetVolume(0, 0.05);
        }
    }

    DrawCellBackground(renderer, time, 3);

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    if(mp->inLobby && !mp->isHost && self->framesPlayed > 3)
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
    }

    if(!self->gamePaused) {
        if(self->gameStarted && (!mp->inLobby || mp->isHost)) {
            if(!self->gameEnded)
            {
                int state = GameManager_tick(game, deltaTime);
                if(state != 0 && mp->inLobby && mp->isHost)
                {
                    json_t* packet = (json_t*)CreateGameStatePacket(game);
                    mpapi_game(mp->mpapi, packet, NULL);
                }

                // Evaluate if game should end
                if(state != 0)
                {
                    if (game->playerCounts == 1)
                    {
                        // Singleplayer: game ends if the only player is dead
                        if (!game->livingStatus[self->myPlayerIndex])
                        {
                            self->gameEnded = 1;
                            self->winnerId = self->myPlayerIndex;
                            AudioManager_StopCh(0);
                        }
                    }
                    else
                    {
                        int aliveCount = 0;
                        int lastAlive = -1;
                        int maxScore = -1;
                        int maxScorePlayer = -1;

                        // Count alive players and track max score
                        for (int p = 0; p < game->playerCounts; p++)
                        {
                            if (game->livingStatus[p])
                            {
                                aliveCount++;
                                lastAlive = p;
                            }
                            if (game->scores[p] > maxScore)
                            {
                                maxScore = game->scores[p];
                                maxScorePlayer = p;
                            }
                        }

                        if (aliveCount == 0)
                        {
                            // All dead -> the player with max score wins if not tied
                            self->gameEnded = 1;

                            // Check for ties
                            int tie = 0;
                            for (int p = 0; p < game->playerCounts; p++)
                            {
                                if (p != maxScorePlayer && game->scores[p] == maxScore)
                                {
                                    tie = 1;
                                    break;
                                }
                            }

                            self->winnerId = tie ? -1 : maxScorePlayer;
                            AudioManager_StopCh(0);
                        }
                        else if (aliveCount == 1)
                        {
                            int survivorScore = game->scores[lastAlive];
                            int deadTie = 0;

                            // Check if any dead player is tied with the survivor
                            for (int p = 0; p < game->playerCounts; p++)
                            {
                                if (!game->livingStatus[p] && game->scores[p] == survivorScore)
                                {
                                    deadTie = 1;
                                    break;
                                }
                            }

                            // Survivor wins ONLY if they are strictly ahead of all dead players
                            if (!deadTie && maxScorePlayer == lastAlive)
                            {
                                self->gameEnded = 1;
                                self->winnerId = lastAlive;
                                AudioManager_StopCh(0);
                            }
                        }

                        // Notify lobby if host
                        if(self->gameEnded && mp->inLobby && mp->isHost)
                        {
                            json_t* lobby = json_object();
                            json_object_set_new(lobby, "packet", json_string("ended"));
                            json_object_set_new(lobby, "winner", json_integer(self->winnerId));
                            mpapi_game(mp->mpapi, lobby, NULL);
                        }
                    }
                }
            }
        } else if(self->framesPlayed > 3) {
            if(self->ready)
            {
                if(!mp->inLobby || mp->isHost)
                    self->countDelta += deltaTime;
                if(self->countDelta > 0.5 || self->lastCountId != self->countId)
                {
                    self->countDelta -= 0.5;
                    if(self->countId < 3)
                    {
                        char countAudio[64];
                        snprintf(countAudio, 64, "assets/audio/sfx/count_%i.wav", 3-self->countId);
                        AudioManager_PlayCh(countAudio, 0.33, 1, 0);
                        self->countId++;
                    } else {
                        AudioManager_PlayCh("assets/audio/sfx/ready.ogg", 0.25, 1, 0);
                        self->gameStarted = 1;
                    }
                    if(mp->inLobby && mp->isHost)
                    {
                        json_t* lobby = json_object();
                        json_object_set_new(lobby, "packet", json_string("count"));
                        mpapi_game(mp->mpapi, lobby, NULL);
                    }
                    self->lastCountId = self->countId;
                }
            }
        }
    }

    int StageSizeH = RenderGame(self, renderer, width, height, deltaTime);

    if(!self->gameStarted)
    {
        switch(self->countId)
        {
            case 0:
                FontManager_RenderFixed(renderer, "MonacoVS", height/12, 26, width/2, height/2, "Ready?", Centered, Centered, (SDL_Color){255, 255, 255, 255});
                break;
            default:
                char countdown[16];
                snprintf(countdown, 16, "%i", 4-self->countId);
                FontManager_RenderFixed(renderer, "MonacoVS", height/6, 26, width/2, height/2, countdown, Centered, Centered, (SDL_Color){255, 255, 255, 255});
                break;
        }
    }

    int gap = height-StageSizeH;
    if(!mp->inLobby && !cfg->localMultiplayer)
    {
        char scoreText[64];
        snprintf(scoreText, 64, "Score:   %i     |     Best:   %i", game->scores[self->myPlayerIndex], cfg->scoreboard->highestScore);
        FontManager_RenderFixed(renderer, "MonacoVS", gap/1.25, 26, width/2, gap/2.5, scoreText, Centered, Centered, (SDL_Color){255, 255, 255, 255});
    } else {
        int temp = self->myPlayerIndex;
            if(cfg->localMultiplayer)
                self->myPlayerIndex = -1;
        char player1Text[64];
        char player2Text[64];
        char player3Text[64];
        char player4Text[64];
        switch(game->playerCounts)
        {
            case 2:
                snprintf(player1Text, 64, "%s (%i)", self->multiplayerNames[0], game->scores[0]);
                FontManager_Render(renderer, "Monaco", gap/1.25, width/4, gap/2.5, player1Text, Centered, Centered, (self->myPlayerIndex == 0 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){255, 164, 164, 255}));
                snprintf(player2Text, 64, "%s (%i)", self->multiplayerNames[1], game->scores[1]);
                FontManager_Render(renderer, "Monaco", gap/1.25, width-width/4, gap/2.5, player2Text, Centered, Centered, (self->myPlayerIndex == 1 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){164, 255, 164, 255}));
                break;
            case 3:
                snprintf(player1Text, 64, "%s (%i)", self->multiplayerNames[0], game->scores[0]);
                FontManager_Render(renderer, "Monaco", gap/1.5, height/80, gap/2.5, player1Text, Start, Centered, (self->myPlayerIndex == 0 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){255, 164, 164, 255}));
                snprintf(player2Text, 64, "%s (%i)", self->multiplayerNames[1], game->scores[1]);
                FontManager_Render(renderer, "Monaco", gap/1.5, width/2, gap/2.5, player2Text, Centered, Centered, (self->myPlayerIndex == 1 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){164, 255, 164, 255}));
                snprintf(player3Text, 64, "%s (%i)", self->multiplayerNames[2], game->scores[2]);
                FontManager_Render(renderer, "Monaco", gap/1.5, width-height/80, gap/2.5, player3Text, End, Centered, (self->myPlayerIndex == 2 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){164, 164, 255, 255}));
                break;
            case 4:
                snprintf(player1Text, 64, "%s (%i)", self->multiplayerNames[0], game->scores[0]);
                FontManager_Render(renderer, "Monaco", gap/1.75, width/5, gap/2.5, player1Text, Centered, Centered, (self->myPlayerIndex == 0 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){255, 164, 164, 255}));
                snprintf(player2Text, 64, "%s (%i)", self->multiplayerNames[1], game->scores[1]);
                FontManager_Render(renderer, "Monaco", gap/1.75, (width/5)*2, gap/2.5, player2Text, Centered, Centered, (self->myPlayerIndex == 1 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){164, 255, 164, 255}));
                snprintf(player3Text, 64, "%s (%i)", self->multiplayerNames[2], game->scores[2]);
                FontManager_Render(renderer, "Monaco", gap/1.75, (width/5)*3, gap/2.5, player3Text, Centered, Centered, (self->myPlayerIndex == 2 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){164, 164, 255, 255}));
                snprintf(player4Text, 64, "%s (%i)", self->multiplayerNames[3], game->scores[3]);
                FontManager_Render(renderer, "Monaco", gap/1.75, (width/5)*4, gap/2.5, player4Text, Centered, Centered, (self->myPlayerIndex == 3 ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){255, 255, 164, 255}));
                break;
        }
        self->myPlayerIndex = temp;
    }

    if(self->gameEnded)
    {
        self->loseTimer += deltaTime;
        if(self->loseTimer > 1)
        {
            if(!self->playedLoseSound)
            {
                self->playedLoseSound = 1;
                if(((!mp->inLobby && game->scores[self->myPlayerIndex] <= cfg->scoreboard->highestScore) || self->winnerId != self->myPlayerIndex) && (!cfg->localMultiplayer || self->winnerId == -1))
                {
                    AudioManager_PlayCh("assets/audio/sfx/lose.ogg", 1, 1, 0);
                } else {
                    AudioManager_PlayCh("assets/audio/sfx/win.ogg", 0.5, 1, 0);
                }
            }
            double animTimer = fmin((self->loseTimer-1)*2,1);
            double textPos = height/2-((height/8)*(1-easeOutQuad(animTimer)));
            if(!mp->inLobby && !cfg->localMultiplayer)
            {
                if(game->scores[self->myPlayerIndex] <= cfg->scoreboard->highestScore)
                {
                    FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos+height/128, "Game   Over", Centered, Centered, (SDL_Color){64, 32, 32, animTimer*255});
                    FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos, "Game   Over", Centered, Centered, (SDL_Color){255, 128, 128, animTimer*255});
                } else {
                    FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos+height/128, "New   Best!", Centered, Centered, (SDL_Color){32, 64, 32, animTimer*255});
                    FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos, "New   Best!", Centered, Centered, (SDL_Color){164, 255, 164, animTimer*255});
                }
            } else {
                if(self->winnerId != self->myPlayerIndex && !cfg->localMultiplayer)
                {
                    FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos+height/128, "You   Lose...", Centered, Centered, (SDL_Color){64, 32, 32, animTimer*255});
                    FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos, "You   Lose...", Centered, Centered, (SDL_Color){255, 128, 128, animTimer*255});
                } else {
                    if(cfg->localMultiplayer)
                    {
                        if(self->winnerId == -1)
                        {
                            FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos+height/128, "Tied...", Centered, Centered, (SDL_Color){64, 32, 32, animTimer*255});
                            FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos, "Tied...", Centered, Centered, (SDL_Color){255, 128, 128, animTimer*255});
                        } else {
                            char tmp[64];
                            snprintf(tmp, 64, "%s   Wins!", self->multiplayerNames[self->winnerId]);
                            FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos+height/128, tmp, Centered, Centered, (SDL_Color){32, 64, 32, animTimer*255});
                            FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos, tmp, Centered, Centered, (SDL_Color){164, 255, 164, animTimer*255});
                        }
                    } else {
                        FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos+height/128, "You   Win!", Centered, Centered, (SDL_Color){32, 64, 32, animTimer*255});
                        FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos, "You   Win!", Centered, Centered, (SDL_Color){164, 255, 164, animTimer*255});
                    }
                }
            }
        }
        if(self->loseTimer > ((cfg->localMultiplayer || mp->inLobby || game->scores[self->myPlayerIndex] > cfg->scoreboard->highestScore) ? 4 : 3))
        {
            if(mp->inLobby)
            {
                mp->returning = 1;
                self->abandon = SCENE_MULTI_LOBBY;
            } else {
                if(cfg->localMultiplayer || !Scoreboard_canSubmit(cfg->scoreboard, game->scores[self->myPlayerIndex]))
                {
                    self->abandon = SCENE_TITLE;
                } else {
                    cfg->scoreFromGame = game->scores[self->myPlayerIndex];
                    self->abandon = SCENE_RECORD;
                }
            }
        }
    }

    if(self->gamePaused)
    {
        SDL_Rect rect = {0, 0, width, height};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
        SDL_RenderFillRect(renderer, &rect);
        int boxW = height/1.8;
        int boxH = height/8;
        SDL_Rect pauseBox = {width/2-boxW/2, height/2-boxH/2, boxW, boxH};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &pauseBox);
        OutlineRect(renderer, &pauseBox, height/90, 255, 255, 255);
        SDL_Rect option1 = FontManager_Render(renderer, "Monaco", height/14, width/2-height/12, height/2, "Resume", End, Centered, (SDL_Color){255, 255, 255, 255});
        SDL_Rect option2 = FontManager_Render(renderer, "Monaco", height/14, width/2+height/25, height/2, "Restart", Centered, Centered, (SDL_Color){255, 255, 255, 255});
        SDL_Rect option3 = FontManager_Render(renderer, "Monaco", height/14, width/2+height/6.25, height/2, "Exit", Start, Centered, (SDL_Color){255, 255, 255, 255});
        FontManager_RenderFixed(renderer, "MonacoVS", height/10, 26, width/2, height/2.6, "Paused", Centered, Centered, (SDL_Color){255, 255, 255, 255});

        SDL_Rect* targetRect = NULL;
        switch(self->pauseSel)
        {
            case 0:
                targetRect = &option1;
                break;
            case 1:
                targetRect = &option2;
                break;
            case 2:
                targetRect = &option3;
                break;
        }
        if(targetRect != NULL)
        {
            // Alignment fix
            int adjustment = height/180;
            targetRect->x -= adjustment*1.5; targetRect->w += adjustment*2;
            targetRect->y -= adjustment; targetRect->h += adjustment*1.75;
            OutlineRect(renderer, targetRect, height/180, 255, 255, 255);
        }

        //FontManager_Render(renderer, "Monaco", height/16, width/2, height/1.9, "Press ESC to unpause.", Centered, Centered, (SDL_Color){255, 255, 255, 255});
    }

    self->framesPlayed++;

    (void)self;
    (void)window;
    (void)renderer;
    (void)deltaTime;
    return -1;
}