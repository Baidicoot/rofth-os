#include "pre.h"

enum {
    OK,
    BAD,
};

typedef struct _LabelMap{
    struct _LabelMap* next;
    int inspos;
    char label;
} LabelMap;

LabelMap* allocLabel(char* buf, int len, int pos, LabelMap* old) {
    LabelMap* tmp = malloc(sizeof(LabelMap*) + sizeof(int) + len + 1);
    tmp->inspos = pos;
    tmp->next = old;
    memcpy(&tmp->label, buf, len);
    (&tmp->label)[len] = 0;
    return tmp;
}

void freeLabel(LabelMap* map) {
    if (map->next != NULL) {
        freeLabel(map->next);
    }
    free(map);
}

int indexLabel(char* str, LabelMap* map, int* err) {
    if (strcmp(str, &map->label) == 0) {
        return map->inspos;
    } else if (map->next != NULL) {
        return indexLabel(str, map->next, err);
    } else {
        *err = BAD;
        return 0;
    }
}

void debup_print_LabelMap(LabelMap* map) {
    printf("Pos: %i, Label: \"%s\"\n", map->inspos, &map->label);
    if (map->next != NULL) {
        debup_print_LabelMap(map->next);
    }
}

LabelMap* count(char* buf, int* len, int* err) {
    int i = 0;
    int c = 0;
    LabelMap* map = NULL;
    for (;;) {
        while (isspace(buf[i])) {
            i++;
        }
        if (buf[i] == 0) {
            *len = c;
            return map;
        }
        if (buf[i] == '"') {
            i++;
            for (;;) {
                if (buf[i] == 0) {
                    *err = BAD;
                    printf("FATAL: ILLEGAL EOF IN STR LITERAL.\n");
                    return 0;
                }
                if (buf[i] == '\\') {
                    i += 2;
                    c++;
                } else if (buf[i] == '"') {
                    i++;
                    break;
                } else {
                    i++;
                    c++;
                }
            }
        } else if (buf[i] == '#') {
            for (;buf[i] != '\n';i++) continue;
        } else if (buf[i] == '.') {
            const int istart = i+1;
            while (buf[i] != '\n' && !isspace(buf[i])) {
                if (buf[i] == 0) {
                    *err = BAD;
                    printf("FATAL: ILLEGAL EOF IN LABEL.\n");
                    return 0;
                }
                i++;
            }
            int len = i - istart;
            if (len > 255) {
                *err = BAD;
                printf("FATAL: LABEL TOO LONG.\n");
                return 0;
            }
            map = allocLabel(buf + istart, len, c, map);
        } else {
            c += 4;
            while (buf[i] != '\n') {
                if (buf[i] == 0) {
                    *len = c;
                    return map;
                }
                i++;
            }
        }
    }
}

void delabel(char* in, char* out, LabelMap* labels, int* err) {
    int i = 0;
    int c = 0;
    if (in[i] == '.') {
        for (;in[i] != '\n' && !isspace(in[i]);i++) continue;
    } else if (in[i] == '#') {
        for (;in[i] != '\n';i++) continue;
    }
    for (;;) {
        if (in[i] == 0) {
            out[c] = 0;
            return;
        } else if (in[i] == '\n') {
            i++;
            out[c] = '\n';
            c++;
            if (in[i] == '.') {
                for (;in[i] != '\n' && !isspace(in[i]);i++) continue;
            } else if (in[i] == '#') {
                for (;in[i] != '\n';i++) continue;
            }
        } else if (in[i] == '.') {
            char label[256];
            int label_len = 0;
            i++;
            while(in[i] != '\n' && !isspace(in[i])) {
                label[label_len] = in[i];
                label_len++;
                i++;
            }
            label[label_len] = 0;
            int pos = indexLabel(label, labels, err);
            if (*err != OK) {
                printf("FATAL: LABEL \"%s\" NOT FOUND.\n", label);
                return;
            }
            char posstr[13];
            sprintf(posstr, "d%i", pos);
            for (int posstr_i = 0; posstr[posstr_i] != 0; posstr_i++) {
                out[c] = posstr[posstr_i];
                c++;
            }
        } else {
            out[c] = in[i];
            i++;
            c++;
        }
    }
}

void demacro(FILE* in, FILE* out, int* err) {
    fseek(in, 0, SEEK_END);
    long fsize = ftell(in);
    fseek(in, 0, SEEK_SET);
    char* ibuf = calloc(1, fsize + 1);
    fread(ibuf, 1, fsize, in);
    fclose(in);
    ibuf[fsize] = 0;

    char* obuf = calloc(1, fsize + 1);

    int len = 0;

    LabelMap* map = count(ibuf, &len, err);
    if (*err != OK) {
        return;
    }
    printf("OUTPUT FILE SIZE (bytes): %i\n", len);
    if (map != NULL) {
        printf("LABELS DETECTED:\n");
        debup_print_LabelMap(map);
    }
    delabel(ibuf, obuf, map, err);

    for (int i = 0; obuf[i] != 0; i++) putc(obuf[i], out);
    fclose(out);
}
/*
int main(int argc, char** argv) {
    if (argc < 2) {
        printf("not enough arguments");
        return -1;
    }

    if (argv[1][0] == '-' && argv[1][1] == 'o') {
        if (argc < 4) {
            printf("not enough arguments");
            return -1;
        }

        FILE* i = fopen(argv[3], "r");
        FILE* o = fopen(argv[2], "w");
        int error = OK;
        demacro(i, o, &error);
        if (error != OK) {
            printf("errored during compilation with code %i", error);
            return 1;
        }
        return 0;
    } else {
        printf("unrecognised command %s", argv[1]);
        return 2;
    }
}
*/