#include "Effects/TrippyBackground.h"
#include <math.h>
#include <stdlib.h>

#include "Utils.h"

void ShiftEB(SDL_Renderer* renderer, SDL_Texture* tex, int maxShift, double rate, double offset)
{
    int width, height;
    SDL_QueryTexture(tex, NULL, NULL, &width, &height);

    SDL_Texture* tmpTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET, width, height);

    SDL_SetRenderTarget(renderer, tmpTex);
    SDL_RenderCopy(renderer, tex, NULL, NULL);

    int maxStrips = 180;
    int stripHeight = (height + maxStrips - 1) / maxStrips;
    int strips = (height + stripHeight - 1) / stripHeight;

    SDL_SetRenderTarget(renderer, tex);

    for (int i = 0; i < strips; i++)
    {
        int y = i * stripHeight;
        int currentHeight = (i == strips - 1) ? (height - y) : stripHeight;

        double sinAdd = (i % 2) ? M_PI : 0;
        int shift = (int)(sin((double)i * rate + offset + sinAdd) * maxShift);

        SDL_Rect src = { 0, y, width, currentHeight };
        SDL_Rect dst = { shift, y, width, currentHeight };

        SDL_RenderCopy(renderer, tmpTex, &src, &dst);
    }

    SDL_DestroyTexture(tmpTex);
}

static double time = 0;
void DrawTrippyBackground(SDL_Renderer* renderer, double deltaTime)
{
    time += deltaTime;
    int winW = 0, winH = 0;
    SDL_GetRendererOutputSize(renderer, &winW, &winH);

    SDL_Texture* bgTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET, winW, winH);
    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, bgTex);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int pattern[][2] = { {5,6}, {5,12}, {5,18}, {5,6}, {5,6}, {5,18}, {5,12}, {5,6} };
    int patternCount = sizeof(pattern)/sizeof(pattern[0]);

    double refW = 320.0;
    double refH = 180.0;
    double scaleX = (double)winW / refW;
    double scaleY = (double)winH / refH;
    double scale = (scaleX + scaleY) * 0.5;

    double patternWidth = 60.0 * scale;
    double offset = fmod(time * 20.0 * scale, patternWidth);
    double x = -offset;

    while (x < winW)
    {
        for (int i = 0; i < patternCount; i++)
        {
            int w = (int)(pattern[i][0] * scale);
            int c = pattern[i][1];

            SDL_SetRenderDrawColor(renderer, c, c, c, 255);
            SDL_Rect rect = { (int)x, 0, w, winH };
            SDL_RenderFillRect(renderer, &rect);

            x += w;
        }
        x += (int)(20 * scale);
    }

    ShiftEB(renderer, bgTex, pow(winH, 1.25)/36, 0.02+sin(time*1.5)*0.005,time*3);

    SDL_SetRenderTarget(renderer, oldTarget);
    SDL_RenderCopy(renderer, bgTex, NULL, NULL);
    SDL_DestroyTexture(bgTex);
}