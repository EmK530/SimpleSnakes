#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <stdint.h>
#include "LinkedList.h"

#define SCOREBOARD_MAX_RECORDS 10
#define SCOREBOARD_NAME_MAX    32

typedef struct
{
    char name[SCOREBOARD_NAME_MAX];
    int score;
} ScoreRecord;

typedef struct
{
    LinkedList *records;     // Sorted descending
    int totalRecords;        // <= 10
    int highestScore;        // 0 if empty
    int lowestScore;         // 0 if empty
} Scoreboard;

Scoreboard *Scoreboard_create(const char *path);

int Scoreboard_save(Scoreboard *sb, const char *path);

void Scoreboard_destroy(Scoreboard **sb);

int Scoreboard_canSubmit(Scoreboard *sb, int score);

int Scoreboard_add(
    Scoreboard *sb,
    const char *name,
    int score,
    const char *path
);

#endif