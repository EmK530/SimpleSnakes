#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Scoreboard.h"
#include "StaticConfig.h"
#include "SharedConfig.h"
#include "RenderMaster/RenderMaster.h"

SharedConfig* cfg = NULL;

#ifdef _WIN32
int main(int argc, char *argv[]) {
#else
int main() {
#endif
    srand(time(NULL));

    cfg = SharedConfig_Get();

    cfg->mp = multiplayer_create();
    cfg->scoreboard = Scoreboard_create("scoreboard.json");

    RenderMaster_Init();

    printf("If a window is successfully created, DO NOT CLOSE THIS CONSOLE.\n");
    printf("To ensure a graceful close, please press the X on the game window instead.\n\n");

    RenderMaster* rm = RenderMaster_Create(GAMETITLE_VER, 0, 0);
    if (!rm) {
        printf("Failed to create RenderMaster\n");
        return 1;
    }

    while(RenderMaster_Work(rm)) { multiplayer_tick(cfg->mp); }

    RenderMaster_Destroy(&rm);

    return 0;
}