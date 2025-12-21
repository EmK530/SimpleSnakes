#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdlib.h>
#include <SDL2/SDL_render.h>

bool QueryWSL();
int clampi(int v, int min, int max);
void OutlineRect(SDL_Renderer* renderer, SDL_Rect* rect, int size, int R, int G, int B);

#endif // UTILS_H