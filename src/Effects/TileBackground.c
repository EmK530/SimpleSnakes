#include "Effects/TileBackground.h"
#include <math.h>
#include <stdlib.h>

#include "Utils.h"

static double *rndPool = NULL;
static int rndPoolSize = 0;

// Taken from Syncade which is another project of mine
void DrawCellBackground(
    SDL_Renderer *renderer,
    double timeSeconds,
    double scale
){
    if (!renderer) return;

    // ---- GRID SETUP ----
    int winW, winH;
    SDL_GetRendererOutputSize(renderer, &winW, &winH);

    int spriteSize = (int)(10.0 * scale);
    if (spriteSize < 1) spriteSize = 1;

    int countX = (winW + spriteSize - 1) / spriteSize;
    int countY = (winH + spriteSize - 1) / spriteSize;

    int pixels = countX * countY;

    // ---- ENSURE RANDOM POOL ----
    if (pixels > rndPoolSize) {
        rndPool = (double*)realloc(rndPool, sizeof(double) * pixels);
        for (int i = rndPoolSize; i < pixels; i++)
            rndPool[i] = (double)rand() / RAND_MAX;
        rndPoolSize = pixels;
    }

    double t = timeSeconds / 3.0;

    // ---- DRAW ----
    int idx = 0;
    for (int cy = 0; cy < countY; cy++) {
        for (int cx = 0; cx < countX; cx++, idx++) {
            double r = rndPool[idx];
            double phase = fmod(t - r, 1.0);
            if (phase < 0) phase += 1.0;
            double expo = pow(5.0, -5.0 * phase);
            int brightness = (int)(expo * 32.0);
            if (brightness < 1)
                continue;
            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            SDL_Rect rect = {
                cx * spriteSize,
                cy * spriteSize,
                spriteSize - 1,
                spriteSize - 1
            };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}