#include <stdio.h>

#include "SharedConfig.h"
#include "RenderMaster/AudioManager.h"

static AudioManager am = {};
static SharedConfig* cfg = NULL;

void AudioManager_Init() {
    cfg = SharedConfig_Get();
    if (Mix_OpenAudio(cfg->audioSample, MIX_DEFAULT_FORMAT, 2, cfg->audioBuffer) < 0) {
        printf("SDL_mixer error: %s\n", Mix_GetError());
        return;
    }

    Mix_AllocateChannels(16);
    am.nextChannel = 0;
    am.opened = 1;
}

void AudioManager_Reload() {
    if(!am.opened)
        return;
    
    AudioManager_StopAll();
    AudioManager_Quit();

    if (Mix_OpenAudio(cfg->audioSample, MIX_DEFAULT_FORMAT, 2, cfg->audioBuffer) < 0) {
        printf("SDL_mixer error: %s\n", Mix_GetError());
        return;
    }

    Mix_AllocateChannels(16);
    am.nextChannel = 0;
    am.opened = 1;
}

int AudioManager_PlayCh(const char* filePath, double volume, int channel, int loops) {
    if(!am.opened)
        return -1;
    
    Mix_Chunk* chunk = Mix_LoadWAV(filePath);
    if (!chunk) {
        printf("Failed to load %s: %s\n", filePath, Mix_GetError());
        return -1;
    }

    Mix_Volume(channel, (int)(volume*128));
    Mix_PlayChannel(channel, chunk, loops);

    return channel;
}

int AudioManager_Play(const char* filePath, double volume, int loops) {
    if(!am.opened)
        return -1;
    int channel = am.nextChannel % 8;
    am.nextChannel++;
    return AudioManager_PlayCh(filePath, volume, channel+8, loops);
}

void AudioManager_SetVolume(int channel, double volume)
{
    if(!am.opened)
        return;
    Mix_Volume(channel, (int)(volume*128));
}

void AudioManager_StopCh(int channel) {
    if(!am.opened)
        return;
    Mix_HaltChannel(channel);
}

void AudioManager_StopAll() {
    if(!am.opened)
        return;
    Mix_HaltChannel(-1);
}

void AudioManager_Quit() {
    if(!am.opened)
        return;
    Mix_CloseAudio();
}