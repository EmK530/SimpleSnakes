#include "Scoreboard.h"
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

static void free_record(void *ptr)
{
    free(ptr);
}

static void recompute_stats(Scoreboard *sb)
{
    sb->totalRecords = (int)sb->records->size;

    if (sb->totalRecords == 0)
    {
        sb->highestScore = 0;
        sb->lowestScore = 0;
        return;
    }

    ScoreRecord *first = sb->records->head->item;
    ScoreRecord *last  = sb->records->tail->item;

    sb->highestScore = first->score;
    sb->lowestScore  = last->score;
}

Scoreboard *Scoreboard_create(const char *path)
{
    Scoreboard *sb = calloc(1, sizeof(Scoreboard));
    sb->records = LinkedList_create();

    FILE *f = fopen(path, "rb");
    if (!f) return sb;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = 0;
    fclose(f);

    // did you know i hate you addresssanitizer
    json_error_t err;
    json_t *root = json_loads(buffer, 0, &err);
    free(buffer);
    if (!root)
        return sb;

    json_t *arr = json_object_get(root, "records");
    if (!json_is_array(arr))
    {
        json_decref(root);
        return sb;
    }

    size_t i;
    json_t *entry;
    json_array_foreach(arr, i, entry)
    {
        if (sb->records->size >= SCOREBOARD_MAX_RECORDS)
            break;

        json_t *jname  = json_object_get(entry, "name");
        json_t *jscore = json_object_get(entry, "score");

        if (!json_is_string(jname) || !json_is_integer(jscore))
            continue;

        ScoreRecord *rec = malloc(sizeof(ScoreRecord));
        strncpy(rec->name, json_string_value(jname), SCOREBOARD_NAME_MAX - 1);
        rec->name[SCOREBOARD_NAME_MAX - 1] = 0;
        rec->score = (int)json_integer_value(jscore);

        LinkedList_append(sb->records, rec);
    }

    json_decref(root);
    recompute_stats(sb);
    return sb;
}

int Scoreboard_save(Scoreboard *sb, const char *path)
{
    json_t *root = json_object();
    json_t *arr  = json_array();

    LinkedList_foreach(sb->records, node)
    {
        ScoreRecord *rec = node->item;
        json_t *obj = json_object();
        json_object_set_new(obj, "name", json_string(rec->name));
        json_object_set_new(obj, "score", json_integer(rec->score));
        json_array_append_new(arr, obj);
    }

    json_object_set_new(root, "records", arr);

    int rc = json_dump_file(root, path, JSON_INDENT(2));
    json_decref(root);
    return rc == 0 ? 0 : -1;
}

void Scoreboard_destroy(Scoreboard **sb)
{
    if (!sb || !*sb)
        return;

    LinkedList_dispose(&(*sb)->records, free_record);
    free(*sb);
    *sb = NULL;
}

int Scoreboard_canSubmit(Scoreboard *sb, int score)
{
    if(score < 1)
        return 0;
    
    if (sb->totalRecords < SCOREBOARD_MAX_RECORDS)
        return 1;

    return score > sb->lowestScore;
}

int Scoreboard_add(Scoreboard *sb, const char *name, int score, const char *path)
{
    if (!Scoreboard_canSubmit(sb, score))
        return 0;

    ScoreRecord *rec = malloc(sizeof(ScoreRecord));
    strncpy(rec->name, name, SCOREBOARD_NAME_MAX - 1);
    rec->name[SCOREBOARD_NAME_MAX - 1] = 0;
    rec->score = score;

    size_t index = 0;
    LinkedList_foreach(sb->records, node)
    {
        ScoreRecord *cur = node->item;
        if (score > cur->score)
            break;
        index++;
    }

    LinkedList_insert(sb->records, index, rec);

    while (sb->records->size > SCOREBOARD_MAX_RECORDS)
    {
        LinkedList_pop(sb->records, sb->records->size - 1, free_record);
    }

    recompute_stats(sb);
    Scoreboard_save(sb, path);
    return 1;
}