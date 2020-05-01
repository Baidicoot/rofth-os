#include "pre.h"

enum {
    OK,
    BAD,
};

typedef struct _MacroMap{
    struct _MacroMap* next;
    char* defn;
    char* macro;
} MacroMap;

MacroMap* allocMacro(char* defnp, int dlen, char* macrop, int nlen, MacroMap* old) {
    MacroMap* tmp = malloc(sizeof(MacroMap) + dlen + nlen + 2);
    char* defn = (char*)(tmp + 1);
    char* macro = defn + dlen + 1;
    tmp->defn = defn;
    tmp->macro = macro;
    tmp->next = old;
    memcpy(defn, defnp, dlen);
    defn[dlen] = 0;
    memcpy(macro, macrop, nlen);
    macro[nlen] = 0;
    return tmp;
}

void freeMacro(MacroMap* map) {
    if (map->next != NULL) {
        freeMacro(map->next);
    }
    free(map);
}

char* indexMacro(char* name, MacroMap* map, int* err) {
    if (strcmp(name, map->macro) == 0) {
        return map->defn;
    } else if (map->next != NULL) {
        return indexMacro(name, map->next, err);
    } else {
        *err = BAD;
        return NULL;
    }
}

void debug_print_MacroMap(MacroMap* map) {
    printf("Name: \"%s\", Defn: \"%s\"\n", map->macro, map->defn);
    if (map->next != NULL) {
        debug_print_MacroMap(map->next);
    }
}

MacroMap* macros(char* buf, int* err) {
    MacroMap* macs = NULL;
    int i = 0;
    *err = OK;
    for (;;) {
        if (buf[i] == 0) {
            return macs;
        } else if (buf[i] == '\n') {
            i++;
            if (buf[i] != '#') continue;
            i++;
            if (buf[i] != '~') continue;
            i++;
            int npos = i;
            int nlen = 0;
            for (;!isspace(buf[i]);i++) nlen++;
            i++;
            int dpos = i;
            int dlen = 0;
            for (;buf[i] != '\n';i++) dlen++;
            macs = allocMacro(buf + dpos, dlen, buf + npos, nlen, macs);
        } else {
            i++;
        }
    }
}

void demacro(char* ibuf, FILE* out, MacroMap* macros, int* err) {
    *err = OK;
    int i = 0;
    for (;;) {
        if (ibuf[i] == 0) {
            return;
        } else if (ibuf[i] == '@') {
            char macro[256];
            i++;
            int mpos = i;
            int mlen = 0;
            for (;!isspace(ibuf[i]);i++) {
                macro[mlen] = ibuf[i];
                mlen++;
            }
            macro[mlen] = 0;
            char* defn = indexMacro(macro, macros, err);
            if (*err != OK) {
                printf("UNDEFINED MACRO \"%s\"\n", macro);
                return;
            }
            int c = 0;
            for(;defn[c] != 0;c++) putc(defn[c], out);
        } else {
            putc(ibuf[i], out);
            i++;
        }
    }
}

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

void preprocess(FILE* in, FILE* out, int* err) {
    fseek(in, 0, SEEK_END);
    long fsize = ftell(in);
    fseek(in, 0, SEEK_SET);
    char* mbuf = calloc(1, fsize + 1);
    fread(mbuf, 1, fsize, in);
    fclose(in);
    mbuf[fsize] = 0;

    MacroMap* macs = macros(mbuf, err);
    if (*err != OK) {
        printf("FATAL ERROR while counting macros\n");
        return;
    }

    if (macs != NULL) {
        printf("MACROS DETECTED:\n");
        debug_print_MacroMap(macs);
    }
    FILE* tmp = fopen("tmp.dmasm", "w+");
    demacro(mbuf, tmp, macs, err);
    if (*err != OK) {
        printf("FATAL ERROR while replacing macros\n");
        return;
    }
    fclose(tmp);

    tmp = fopen("tmp.dmasm", "r");
    fseek(tmp, 0, SEEK_END);
    fsize = ftell(tmp);
    fseek(tmp, 0, SEEK_SET);
    char* ibuf = calloc(1, fsize + 1);
    fread(ibuf, 1, fsize, tmp);
    fclose(tmp);
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