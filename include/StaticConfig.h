#ifndef STATICCONFIG_H
#define STATICCONFIG_H

#define BUILD_VERSION "v1.0.0"
#ifdef DEV_BUILD
    #define BUILD_STRING BUILD_VERSION " DEV BUILD"
#else
    #define BUILD_STRING BUILD_VERSION
#endif

#ifdef DEV_BUILD
    #define GAMETITLE "RenderMaster Window"
    #define GAMETITLE_VER GAMETITLE " [" BUILD_STRING "]"
#else
    #define GAMETITLE "Simple Snakes"
    #define GAMETITLE_VER GAMETITLE " [" BUILD_STRING "]"
#endif

// Multiplayer API

#define MP_API_HOST "kontoret.onvo.se"
#define MP_API_PORT 9001
//#define MP_API_IDENT "23e9f2f2-4b13-4595-93c4-d6635c63c596"
//#define MP_API_IDENT "WorldLink-beta-9879f588734e4b0c74b25"
#define MP_API_IDENT "WorldLink-prod-17d0e5031dea278a51a65"
#define MP_API_TIMEOUT 3.0

#define WORLDLINK_NAME_PREFIX "WL-" // Exists due to MpApi list bug

// Render Master

#define RM_FPS_FONT "acme_7_wide_xtnd"

// Font Manager

#define FM_FONTLIFETIME 60 // Frames

#endif // STATICCONFIG_H