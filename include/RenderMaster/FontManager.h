#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "LinkedList.h"

typedef struct {
    char* path;
    int size;
    TTF_Font* font;
    int lastUsed;
} FM_FontEntry;

typedef struct {
    LinkedList* list;
    int currentFrame;
} FontManager;

typedef enum {
    Start, Centered, End
} FM_TextAlign;

void FontManager_Init();
int FontManager_Debug_Count();
TTF_Font* FontManager_Get(const char* path, int size);
SDL_Rect FontManager_Render(SDL_Renderer* renderer, const char* font, int size, int x, int y, const char* text, FM_TextAlign horizontalAlign, FM_TextAlign verticalAlign, SDL_Color color);
SDL_Rect FontManager_RenderFixed(SDL_Renderer* renderer, const char* font, int size, int trueSize, int x, int y, const char* text, FM_TextAlign horizontalAlign, FM_TextAlign verticalAlign, SDL_Color color);
void FontManager_Work();

#endif // FONTMANAGER_H