#ifndef TRIPPYBACKGROUND_H
#define TRIPPYBACKGROUND_H

#include <SDL2/SDL.h>
#include <math.h>
#include <stdlib.h>

#include "Utils.h"

// Taken from Syncade which is another project of mine
void DrawTrippyBackground(SDL_Renderer* renderer, double deltaTime);

#endif // TRIPPYBACKGROUND_H