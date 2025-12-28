#include "math.h"
#include "string.h"

#include "SDL2/SDL.h"

#include "Utils.h"
#include "easings.h"
#include "SharedConfig.h"
#include "RenderMaster/RenderMaster.h"
#include "RenderMaster/FontManager.h"
#include "RenderMaster/Transitions.h"
#include "RenderMaster/AudioManager.h"

static TransitionData* data = NULL;
static SharedConfig* cfg = NULL;

void Transitions_Start(char* text, int R, int G, int B) {
    if(data == NULL)
    {
        data = (TransitionData*)malloc(sizeof(TransitionData));
        data->gotoText = NULL;
    }
    cfg = SharedConfig_Get();
    data->active = 1;
    data->time = 0;
    data->midway = 0;
    data->hasStepped = 0;
    data->R = R;
    data->G = G;
    data->B = B;
    data->delay = 0;
    if(data->gotoText != NULL)
        free(data->gotoText);
    data->gotoText = strdup(text);
    AudioManager_Play("assets/audio/sfx/transition.wav", 0.25, 0);
}

int Transitions_IsActive() {
    if(data == NULL)
        return 0;
    return data->active;
}

int Transitions_IsBusy() {
    if(data == NULL)
        return 0;
    return data->active && !data->midway;
}

int Transitions_HasStepped() {
    if(data == NULL)
        return 1;
    return data->hasStepped;
}

int Transitions_Step(double deltaTime) {
    if(data == NULL || !data->active)
        return 0;
    
    data->hasStepped = 1;
    if(deltaTime < 0.1 || data->delay == 10)
    {
        data->time += deltaTime;
    } else {
        data->delay++;
    }
    double vol = clampd(0.25-data->time, 0, 0.25);
    if(!data->midway)
        AudioManager_SetVolume(0, vol);

    if(data->time > 1.25 && !data->midway)
    {
        data->midway = 1;
        data->time = 0;
    }
    return data->midway;
}

void Transitions_Draw(SDL_Renderer* renderer)
{
    if (data == NULL || !data->active)
        return;

    data->hasStepped = 0;
    int width = 0, height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    if(!data->midway)
    {
        double anim = easeInQuart(clampd(data->time*1.5,0,1));

        if(anim < 0.999)
        {
            int maxSplits = 180;
            int stripHeight = (height + maxSplits - 1) / maxSplits;
            int splits = (height + stripHeight - 1) / stripHeight;

            int limit = clampd(width * data->time/2, 0, width);

            for (int i = 0; i < splits; i++) {
                int currentHeight = stripHeight;
                if (i == splits - 1)
                    currentHeight = height - i * stripHeight;

                int shift = rand() % (limit*2+1) - limit;

                SDL_Rect src = { 0, i * stripHeight, width, currentHeight };
                SDL_Rect dst = { shift, i * stripHeight, width, currentHeight };

                SDL_RenderCopy(renderer, cfg->gameTex, &src, &dst);
            }
        }

        if(data->time < 1)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, data->time*255);
            SDL_Rect r1 = (SDL_Rect){0, 0, width, height};
            SDL_RenderFillRect(renderer, &r1);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);

        // top
        SDL_Rect r2 = (SDL_Rect){0, 0, width/2*anim, height/2*anim};
        SDL_RenderFillRect(renderer, &r2);
        r2.x = width-r2.w;
        SDL_RenderFillRect(renderer, &r2);

        // bottom
        SDL_Rect r3 = (SDL_Rect){0, 0, width/2*anim, height/2*anim};
        r3.y = height-r3.h;
        SDL_RenderFillRect(renderer, &r3);
        r3.x = width-r3.w;
        SDL_RenderFillRect(renderer, &r3);

        if(data->time > 0.67)
        {
            double animTimer = fmin(((data->time)-0.666)*2,1);
            double textPos = height/2-((height/8)*(1-easeOutQuad(animTimer)));
            FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos+height/128, data->gotoText, Centered, Centered, (SDL_Color){48, 48, 48, animTimer*255});
            FontManager_RenderFixed(renderer, "MonacoVS", height/8, 26, width/2, textPos, data->gotoText, Centered, Centered, (SDL_Color){data->R, data->G, data->B, animTimer*255});
        }
    } else {
        SDL_RenderCopy(renderer, cfg->gameTex, NULL, NULL);
        double anim = 1-easeOutExpo(clampd(data->time*1.5,0,1));
        SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);

        // top
        SDL_Rect r2 = (SDL_Rect){0, 0, width/2*anim, height/2*anim};
        SDL_RenderFillRect(renderer, &r2);
        r2.x = width-r2.w;
        SDL_RenderFillRect(renderer, &r2);

        // bottom
        SDL_Rect r3 = (SDL_Rect){0, 0, width/2*anim, height/2*anim};
        r3.y = height-r3.h;
        SDL_RenderFillRect(renderer, &r3);
        r3.x = width-r3.w;
        SDL_RenderFillRect(renderer, &r3);

        if(anim < 0.001)
        {
            data->active = 0;
            free(data->gotoText);
            data->gotoText = NULL;
        }
    }
}
