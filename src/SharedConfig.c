#include "SharedConfig.h"

static SharedConfig g_config = {
    960, // Window Width
    720, // Window Height
    0, // Resolution changed by game
    1, // First Run

    256, // Audio Buffer
    48000, // Audio Sample Rate
    1, // Vertical Sync
    1, // DisplayFPS
    14, // fpsFontSize
    0, // Mp music playing
    NULL, // public Multiplayer instance, to be set later
    0, // mpapi Listener Id
    0, // local Multiplayer
    NULL, // public Scoreboard instance, to be set later
    0, // score from game, to be set later
    NULL // render texture, to be set later
};

SharedConfig* SharedConfig_Get() {
    return &g_config;
}
