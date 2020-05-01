#include "pre.h"

enum {
    OK,
    BAD
};

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
        preprocess(i, o, &error);
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