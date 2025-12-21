#ifndef SCENEUTILS_H
#define SCENEUTILS_H

#include <SDL2/SDL.h>

typedef enum
{
    SCENE_INTRO,
    SCENE_TITLE,
    SCENE_CONFIG,
    SCENE_GAME,
    SCENE_MULTI_ENTERPUBLIC,
    SCENE_MULTI,
    SCENE_MULTI_CREATELOBBY,
    SCENE_MULTI_LOBBY,
    SCENE_RECORD,
    SCENE_SCOREBOARD,
    SCENE_MULTI_ENTERPRIVATE,
    _maxScene // This type exists to count the number of existing scenes in RenderMaster, do not use or OOB accesses will occur.
} SceneIDs;

typedef struct 
{
    void* context;
    int (*prepFunction)(void*, SDL_Window*, SDL_Renderer*, int);
    int (*workFunction)(void*, SDL_Window*, SDL_Renderer*, double);
    int (*inputFunction)(void*, SDL_Window*, SDL_Renderer*, SDL_Event*);
} GenericScene;

typedef struct 
{
    void* scene;
    int sceneId;
    char* sceneName;
} InitializedScene;

typedef enum
{
    ArrowUp = 1073741906,
    ArrowDown = 1073741905,
    ArrowLeft = 1073741904,
    ArrowRight = 1073741903,
    W = 119,
    A = 97,
    S = 115,
    D = 100,
    Enter = 13,
    Escape = 27,
    R = 114,
    Backspace = 8,
    I = 105,
    J = 106,
    K = 107,
    L = 108
} Button;

#endif // SCENEUTILS_H