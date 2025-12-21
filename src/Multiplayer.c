#include <time.h>
#include <string.h>

#include "SDL2/SDL.h"
#include "Multiplayer.h"
#include "StaticConfig.h"
#include "LinkedList.h"

Multiplayer* multiplayer_create()
{
    Multiplayer* mp = (Multiplayer*)malloc(sizeof(Multiplayer));

    mp->mpapi = mpapi_create(MP_API_HOST, MP_API_PORT, MP_API_IDENT);
    if (!mp->mpapi)
	{
		printf("Failed to create mpapi instance!\n");
	}

    memset(mp->username, '\0', 22);
    snprintf(mp->username, 22, "Guest %i", rand());
    mp->lobbyList = LinkedList_create();
    mp->listStatus = -1;
    mp->listResponse = NULL;
    mp->inLobby = 0;
    mp->timeout = 0;
    mp->isHost = 0;
    mp->returning = 0;
    mp->ping = SDL_GetTicks();

    return mp;
}

char* multiplayer_err_stringify(int id)
{
    switch(id)
    {
        case 0:
            return "MPAPI_OK";
        case 1:
            return "MPAPI_ERR_ARGUMENT";
        case 2:
            return "MPAPI_ERR_STATE";
        case 3:
            return "MPAPI_ERR_CONNECT";
        case 4:
            return "MPAPI_ERR_PROTOCOL";
        case 5:
            return "MPAPI_ERR_IO";
        case 6:
            return "MPAPI_ERR_REJECTED";
        default:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

void multiplayer_tick(Multiplayer* mp)
{
    if(mp->inLobby && mp->isHost && (int)SDL_GetTicks() > mp->ping + 1000)
    {
        mp->ping = SDL_GetTicks();
        json_t* lobby = json_object();
        json_object_set_new(lobby, "packet", json_string("ping"));
        mpapi_game(mp->mpapi, lobby, NULL);
    }
    (void)mp;
}

void multiplayer_free_lobby(void* object)
{
    LobbyInfo* lobby = (LobbyInfo*)object;
    free(lobby->id);
    free(lobby->name);
    free(lobby);
}

void multiplayer_free_player(void* object)
{
    PlayerInfo* player = (PlayerInfo*)object;
    free(player->playerId);
    free(player->playerName);
    free(player);
}

void multiplayer_refresh_list(Multiplayer* mp)
{
    if(mp->listStatus == 0)
        return;
    multiplayer_empty_list(mp);
    int status = mpapi_list(mp->mpapi, &mp->listResponse);
    mp->listStatus = status+1;
    if(status == MPAPI_OK)
    {
        if(!mp->listResponse)
            return;

        size_t index;
        json_t *lobby;

        size_t prefixLen = strlen(WORLDLINK_NAME_PREFIX);
        json_array_foreach(mp->listResponse, index, lobby)
        {
            json_t *id      = json_object_get(lobby, "id");
            json_t *name    = json_object_get(lobby, "name");
            json_t *clients = json_object_get(lobby, "clients");

            const char *id_str   = json_string_value(id);
            const char *name_str = json_string_value(name);

            if(strlen(name_str) > prefixLen && strncmp(name_str, WORLDLINK_NAME_PREFIX, prefixLen) == 0)
            {
                LobbyInfo* nfo = (LobbyInfo*)malloc(sizeof(LobbyInfo));
                nfo->id = (char*)strdup(id_str);
                nfo->name = (char*)strdup(name_str+=prefixLen);
                nfo->playerCount = json_array_size(clients);

                nfo->playerNames = LinkedList_create();
                size_t idx;
                json_t *player;
                json_array_foreach(clients, idx, player)
                {
                    LinkedList_append(nfo->playerNames, (char*)json_string_value(player));
                }

                LinkedList_append(mp->lobbyList, nfo);
            }
        }
    }
}

PlayerInfo* CreatePlayer(char* playerId)
{
    PlayerInfo* plr = malloc(sizeof(PlayerInfo));
    plr->playerId = strdup(playerId);
    plr->playerName = strdup("Loading...");
    plr->ready = 0;
    plr->host = 0;
    plr->participating = 0;
    plr->loaded = 0;
    plr->inGameIndex = -1;
    plr->ackDelay = 0;
    plr->needsUpdate = 0;
    return plr;
}

PlayerInfo* FindPlayer(Multiplayer* mp, const char* playerId)
{
    LinkedList_foreach(mp->playerList, node)
    {
        PlayerInfo* info = (PlayerInfo*)node->item;
        if(strcmp(info->playerId, playerId) == 0)
        {
            return info;
        }
    }
    return NULL;
}

int multiplayer_host(Multiplayer* mp, char* lobbyName, int isPrivate)
{
    int prefixLen = strlen(WORLDLINK_NAME_PREFIX);
    char* finalName = calloc(1, strlen(lobbyName)+prefixLen+1);
    strcpy(finalName, WORLDLINK_NAME_PREFIX);
    strcpy(finalName+prefixLen, lobbyName);
    json_t* lobbyData = json_object();
    json_object_set_new(lobbyData, "private", json_boolean(isPrivate)); // Lobby private
    json_object_set_new(lobbyData, "name", json_string(finalName));     // Lobby name
    json_object_set_new(lobbyData, "maxClients", json_integer(4));      // Max players: 4
    json_object_set_new(lobbyData, "hostMigration", json_boolean(0));   // No migration when host leaves
    int result = mpapi_host(mp->mpapi, lobbyData, &mp->curSession, &mp->curClientId, &mp->joinData);
    if(result == MPAPI_OK)
    {
        mp->inLobby = 1;
        mp->isHost = 1;
        mp->ping = SDL_GetTicks();
        mp->lobbyName = strdup(lobbyName);
        mp->playerList = LinkedList_create();
        PlayerInfo* self = CreatePlayer(mp->curClientId);
        self->host = 1;
        self->playerName = strdup(mp->username);
        mp->selfPlayer = self;
        LinkedList_append(mp->playerList, (void*)self);
    }
    return result;
}

int multiplayer_join(Multiplayer* mp, char* lobbyId, char* lobbyName, LinkedList* lobbyPlayers)
{
    json_t* userData = json_object();
    json_object_set_new(userData, "name", json_string(mp->username));

    int result = mpapi_join(mp->mpapi, lobbyId, userData, &mp->curSession, &mp->curClientId, &mp->joinData);
    if(result == MPAPI_OK)
    {
        mp->inLobby = 1;
        mp->isHost = 0;
        mp->ping = SDL_GetTicks();
        mp->lobbyName = strdup(lobbyName);
        mp->playerList = LinkedList_create();
        mp->selfPlayer = NULL;
        if(lobbyPlayers != NULL)
        {
            LinkedList_foreach(lobbyPlayers, node)
            {
                PlayerInfo* plr = CreatePlayer((char*)node->item);
                LinkedList_append(mp->playerList, (void*)plr);
            }
            PlayerInfo* self = CreatePlayer((char*)mp->curClientId);
            mp->selfPlayer = self;
            self->playerName = strdup(mp->username);
            LinkedList_append(mp->playerList, (void*)self);
        }
    }
    return result;
}

void multiplayer_empty_list(Multiplayer* mp)
{
    if(mp->listResponse != NULL)
        json_decref(mp->listResponse);
    mp->listResponse = NULL;
    LinkedList_clear(mp->lobbyList, multiplayer_free_lobby);
    mp->listStatus = -1;
    mp->listResponse = NULL;
}

void multiplayer_destroy(Multiplayer** mp)
{
    Multiplayer* multi = *mp;
    LinkedList_dispose(&multi->lobbyList, multiplayer_free_lobby);
    free(*mp);
    *mp = NULL;
}