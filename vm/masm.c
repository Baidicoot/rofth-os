#include "asm.h"
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
        FILE* tmp = fopen("tmp.rasm", "w+");
        int error = OK;
        preprocess(i, tmp, &error);
        if (error != OK) {
            printf("errored during demacro with code %i", error);
            return 1;
        }
        fclose(tmp);
        fclose(i);

        FILE* tmp2 = fopen("tmp.rasm", "r");
        FILE* o = fopen(argv[2], "w+");
        assemble(tmp2, o, &error);
        if (error != OK) {
            printf("errored during assembly with code %i", error);
            return 1;
        }
        fclose(tmp2);
        fclose(o);
        
        return 0;
    } else {
        printf("unrecognised command %s", argv[1]);
        return 2;
    }
}