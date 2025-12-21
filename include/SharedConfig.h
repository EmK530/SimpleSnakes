#ifndef SHAREDCONFIG_H
#define SHAREDCONFIG_H

#include "Multiplayer.h"
#include "Scoreboard.h"

typedef struct {
    int screenWidth;
    int screenHeight;
    int resolutionWasChanged;
    int firstRun;
    
    int audioBuffer;
    int audioSample;
    int verticalSync;
    int displayFps;
    int fpsFontSize;
    int isMpMusicPlaying;
    Multiplayer* mp;
    int listenerId;
    int localMultiplayer;
    Scoreboard* scoreboard;
    int scoreFromGame;
} SharedConfig;

SharedConfig* SharedConfig_Get();

#endif // SHAREDCONFIG_H