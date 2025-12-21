#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "StaticConfig.h"
#include "LinkedList.h"
#include "RenderMaster/FontManager.h"

static FontManager* fm = NULL;

void FontManager_Init()
{
    if(fm != NULL)
        return;
    
    FontManager* newFM = malloc(sizeof(FontManager));
    newFM->list = LinkedList_create();
    newFM->currentFrame = 0;
    fm = newFM;
}

int FontManager_Debug_Count()
{
    return fm->list->size;
}

static FM_FontEntry* fm_find(const char* path, int size)
{
    LinkedList_foreach(fm->list, node)
    {
        FM_FontEntry* e = (FM_FontEntry*)node->item;
        if (e->size == size && strcmp(e->path, path) == 0)
            return e;
    }
    return NULL;
}

TTF_Font* FontManager_Get(const char* path, int size)
{
    if(fm == NULL)
        return NULL;

    FM_FontEntry* entry = fm_find(path, size);
    if (entry)
    {
        entry->lastUsed = fm->currentFrame;
        return entry->font;
    }

    TTF_Font* font = TTF_OpenFont(path, size);
    if (!font)
    {
        printf("[FontManager] Failed to load %s size %d: %s\n", path, size, TTF_GetError());
        return NULL;
    }

    entry = malloc(sizeof(FM_FontEntry));
    entry->path = strdup(path);
    entry->size = size;
    entry->font = font;
    entry->lastUsed = fm->currentFrame;

    LinkedList_append(fm->list, entry);

    return font;
}

static SDL_Rect emptyRect = {0, 0, 0, 0};

SDL_Rect FontManager_Render(SDL_Renderer* renderer, const char* font, int size, int x, int y, const char* text, FM_TextAlign horizontalAlign, FM_TextAlign verticalAlign, SDL_Color color)
{
    char fullPath[128];
    snprintf(fullPath, 128, "%s%s%s", "assets/ttf/", font, ".ttf");
    TTF_Font* fnt = FontManager_Get(fullPath, size);
    if(fnt == NULL)
        return emptyRect;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Surface* textSurface = TTF_RenderText_Solid(fnt, text, color);
    if(textSurface == NULL)
        return emptyRect;
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_SetTextureScaleMode(textTexture, SDL_ScaleModeNearest);
    if(horizontalAlign == Centered)
    {
        x -= textSurface->w/2;
    } else if(horizontalAlign == End) {
        x -= textSurface->w;
    }
    if(verticalAlign == Centered)
    {
        y -= textSurface->h/2;
    } else if(verticalAlign == End) {
        y -= textSurface->h;
    }
    SDL_Rect dstRect = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &dstRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    return dstRect;
}

SDL_Rect FontManager_RenderFixed(SDL_Renderer* renderer, const char* font, int size, int trueSize, int x, int y, const char* text, FM_TextAlign horizontalAlign, FM_TextAlign verticalAlign, SDL_Color color)
{
    char fullPath[128];
    snprintf(fullPath, 128, "%s%s%s", "assets/ttf/", font, ".ttf");
    TTF_Font* fnt = FontManager_Get(fullPath, trueSize);
    if(fnt == NULL)
        return emptyRect;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Surface* textSurface = TTF_RenderText_Solid(fnt, text, color);
    if(textSurface == NULL)
        return emptyRect;
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_SetTextureScaleMode(textTexture, SDL_ScaleModeNearest);
    double scale = ((double)size/(double)trueSize);
    int w = textSurface->w * scale;
    int h = textSurface->h * scale;
    if(horizontalAlign == Centered)
    {
        x -= w/2;
    } else if(horizontalAlign == End) {
        x -= w;
    }
    if(verticalAlign == Centered)
    {
        y -= h/2;
    } else if(verticalAlign == End) {
        y -= h;
    }
    SDL_Rect dstRect = {x, y, w, h};
    SDL_RenderCopy(renderer, textTexture, NULL, &dstRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    return dstRect;
}

static void fm_free_entry(void* ptr)
{
    FM_FontEntry* entry = (FM_FontEntry*)ptr;
    if (!entry) return;
    if (entry->font)
        TTF_CloseFont(entry->font);
    free(entry->path);
    free(entry);
}

void FontManager_Work()
{
    if(fm == NULL)
        return;
    fm->currentFrame++;
    Node* node = fm->list->head;
    while (node)
    {
        Node* next = node->front;
        FM_FontEntry* entry = (FM_FontEntry*)node->item;
        int age = fm->currentFrame - entry->lastUsed;
        if (age > FM_FONTLIFETIME)
            LinkedList_remove(fm->list, node, fm_free_entry);
        node = next;
    }
}