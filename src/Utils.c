#include <stdbool.h>
#include <stdlib.h>
#include <SDL2/SDL_render.h>

#include "Utils.h"

bool QueryWSL() {
    return getenv("WSL_INTEROP") != NULL || getenv("WSL_DISTRO_NAME") != NULL;
}

int clampi(int v, int min, int max) {
    return (v < min ? min : (v > max ? max : v));
}

void OutlineRect(SDL_Renderer* renderer, SDL_Rect* rect, int size, int R, int G, int B)
{
    SDL_SetRenderDrawColor(renderer, R, G, B, 255);
    SDL_Rect top = {rect->x-size, rect->y-size, rect->w+size*2, size};
    SDL_RenderFillRect(renderer, &top);
    SDL_Rect bottom = {rect->x-size, rect->y+rect->h, rect->w+size*2, size};
    SDL_RenderFillRect(renderer, &bottom);
    SDL_Rect left = {rect->x-size, rect->y, size, rect->h};
    SDL_RenderFillRect(renderer, &left);
    SDL_Rect right = {rect->x+rect->w, rect->y, size, rect->h};
    SDL_RenderFillRect(renderer, &right);
}