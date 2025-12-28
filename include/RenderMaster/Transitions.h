#ifndef TRANSITIONS_H
#define TRANSITIONS_H

#include "SDL2/SDL.h"

typedef struct
{
    double time;
    int midway;
    int active;
    int hasStepped;
    char* gotoText;
    int R;
    int G;
    int B;
    int delay;
} TransitionData;

void Transitions_Start(char* text, int R, int G, int B);
int Transitions_IsActive();
int Transitions_IsBusy();
int Transitions_HasStepped();
void Transitions_Draw(SDL_Renderer* renderer);
int Transitions_Step(double deltaTime);

#endif // TRANSITIONS_H