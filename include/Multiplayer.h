#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include "mpapi.h"
#include "LinkedList.h"

typedef struct
{
    char* id;
    char* name;
    int playerCount;
    LinkedList* playerNames;
} LobbyInfo;

typedef struct
{
    char* playerId;
    char* playerName;
    int ready;
    int host;
    int participating;
    int loaded;
    int inGameIndex;
    double ackDelay;
    int needsUpdate;
} PlayerInfo;

typedef struct
{
    mpapi* mpapi;
    char username[22];

    LinkedList* lobbyList;
    LinkedList* playerList;
    PlayerInfo* selfPlayer;

    int listStatus;
    json_t* listResponse;

    int returning;
    int inLobby;
    int isHost;
    int timeout;
    int ping;
    char* lobbyName;
    char* curSession;
    char* curClientId;
    json_t* joinData;
} Multiplayer;


Multiplayer* multiplayer_create();
char* multiplayer_err_stringify(int id);
void multiplayer_tick(Multiplayer* mp);
void multiplayer_free_lobby(void* object);
void multiplayer_free_player(void* object);
void multiplayer_refresh_list(Multiplayer* mp);
PlayerInfo* CreatePlayer(char* playerId);
PlayerInfo* FindPlayer(Multiplayer* mp, const char* playerId);
int multiplayer_host(Multiplayer* mp, char* lobbyName, int isPrivate);
int multiplayer_join(Multiplayer* mp, char* lobbyId, char* lobbyName, LinkedList* lobbyPlayers);
void multiplayer_empty_list(Multiplayer* mp);
void multiplayer_destroy(Multiplayer** mp);

#endif // MULTIPLAYER_H