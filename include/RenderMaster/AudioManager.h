#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SDL2/SDL_mixer.h>

typedef struct {
    int nextChannel;
    int opened;
} AudioManager;

typedef struct {
    char* path;
    Mix_Chunk* chunk;
} AudioCacheEntry;

void AudioManager_Init();
void AudioManager_Reload();
int  AudioManager_Play(const char* filePath, double volume, int loops);
int  AudioManager_PlayCh(const char* filePath, double volume, int channel, int loops);
void AudioManager_SetVolume(int channel, double volume);
void AudioManager_StopCh(int channel);
void AudioManager_StopAll();
void AudioManager_Quit();

#endif