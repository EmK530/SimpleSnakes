#ifndef TILEBACKGROUND_H
#define TILEBACKGROUND_H

#include <SDL2/SDL.h>
#include <math.h>
#include <stdlib.h>

#include "Utils.h"

// Taken from Syncade which is another project of mine
void DrawCellBackground(
    SDL_Renderer *renderer,
    double timeSeconds,
    double scale
);

#endif // TILEBACKGROUND_H