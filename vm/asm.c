/* VM ASSEMBLER */
#include "asm.h"

enum {
    OK,
    BAD,
};

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

enum INS_TKN {
    HLT,

    LIM,

    LD8,
    LD16,
    LD32,

    SV8,
    SV16,
    SV32,

    AND,
    OR,
    XOR,
    NOR,

    ADD,
    ADDC,
    SHIFTL,
    SHIFTR,

    LJAL,
    BEQ,
    BNE,
    BLT,
    BGT,
    INT
};

enum INS_TYPE {
    UNARY,
    REG_IMM16,
    REG2_IMM8,
    REG3,
    IMM8x2,
};

typedef struct {
    char op;
    char a1;
    char a2;
    char a3;
} Ins;

int insType[] = {
    HLT, UNARY,

    LIM, REG_IMM16,

    LD8, REG2_IMM8,
    LD16, REG2_IMM8,
    LD32, REG2_IMM8,

    SV8, REG2_IMM8,
    SV16, REG2_IMM8,
    SV32, REG2_IMM8,

    AND, REG3,
    OR, REG3,
    XOR, REG3,
    NOR, REG3,

    ADD, REG3,
    ADDC, REG3,
    SHIFTL, REG3,
    SHIFTR, REG3,

    LJAL, REG2_IMM8,
    BEQ, REG2_IMM8,
    BNE, REG2_IMM8,
    BLT, REG2_IMM8,
    BGT, REG2_IMM8,

    INT, IMM8x2
};

char* toks[] = {
    "hlt", HLT,
    "lim", LIM,
    "l08", LD8,
    "l16", LD16,
    "l32", LD32,
    "s08", SV8,
    "s16", SV16,
    "s32", SV32,

    "and", AND,
    "eth", OR,
    "xor", XOR,
    "nor", NOR,

    "add", ADD,
    "adc", ADDC,
    "shl", SHIFTL,
    "shr", SHIFTR,

    "jal", LJAL,
    "beq", BEQ,
    "bne", BNE,
    "blt", BLT,
    "bgt", BGT,
    "int", INT,
    "nop", 64,
};

int getInsType(int op) {
    for (int i = 0; i < sizeof(insType)/sizeof(insType[0]); i += 2) {
        if (op == insType[i]) return insType[i+1];
    }
    return UNARY;
}

int stoi(char* str, int* err) {
    *err = OK;
    int acc = 0;
    int neg_flag = 0;
    int i = 0;
    if (str[0] == '-') {
        neg_flag = 1;
        i = 1;
    }
    for (;;i++) {
        if (str[i] == 0) {
            break;
        }
        int d = str[i] - '0';
        if (d < 0 || d > 9) {
            *err = BAD;
            return 0;
        }
        acc = acc * 10 + d;
    }
    if (neg_flag) {
        return -acc;
    } else {
        return acc;
    }
}

int btoi(char* str, int* err) { // doesn't work??
    *err = OK;
    int acc = 0;
    for (int i = 0;;i++) {
        if (str[i] == 0) {
            break;
        }
        int d = str[i] - '0';
        if (d != 0 && d != 1) {
            *err = BAD;
            return 0;
        }
        acc = acc * 2 + d;
    }
    return acc;
}

int inseq(char* a, char* b) {
    return (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]);
}

int classifyIns(char a, char b, char c, int* err) {
    *err = OK;
    char str[] = {a, b, c};
    for (int i = 0; i < sizeof(toks)/sizeof(toks[0]); i += 2) {
        if (inseq(str, toks[i])) {
            return (int)toks[i+1];
        }
    }
    *err = BAD;
    return 0;
}

int parseRegister(char* op, int* err) {
    *err = OK;
    if (op[0] != 'r') {
        *err = 1;
        return 0;
    }
    int te = 0;
    int n = stoi(op + 1, &te);
    if (te != OK) {
        *err = BAD;
        return 0;
    }
    if (te < 0 || te > 31) {
        *err = BAD;
        return 0;
    }
    return n;
}

int parseImm(char* op, int* err) {
    *err = OK;
    if (op[0] == 'd') {
        return stoi(op+1, err);
    }
    if (op[0] == '\'') {
        return op[1];
    }
    if (op[0] == 'b') {
        return btoi(op+1, err);
    }
    *err = BAD;
    return 0;
}

int splitChar(char delim, char* str, char** strs) {
    for (int i = 0, l = 0, c = 0;;i++) {
        if (str[i] == 0) {
            strs[l][c] = 0;
            break;
        }
        if (str[i] == delim) {
            strs[l][c] = 0;
            l++;
            i++;
            c = 0;
        }
        strs[l][c] = str[i];
        c++;
    }
    return OK;
}

Ins parseIns(char* line, int* err) {
    Ins tmp = {0, 0, 0, 0};
    tmp.op = classifyIns(line[0], line[1], line[2], err);
    if (*err == BAD) return tmp;
    char op1[256];
    char op2[256];
    char op3[256];
    char* operands[] = {
        op1,
        op2,
        op3,
    };
    *err = splitChar(' ', line + 4, operands);
    if (*err == BAD) return tmp;
    switch (getInsType(tmp.op))
    {
    case UNARY:
        return tmp;
    
    case REG_IMM16:
        tmp.a1 = parseRegister(operands[0], err);
        if (*err == BAD) return tmp;
        i16 imm[1];
        imm[0] = parseImm(operands[1], err);
        i8* imm8 = imm;
        tmp.a2 = imm8[0];
        tmp.a3 = imm8[1];
        return tmp;
    
    case REG2_IMM8:
        tmp.a1 = parseRegister(operands[0], err);
        if (*err == BAD) return tmp;
        tmp.a2 = parseRegister(operands[1], err);
        if (*err == BAD) return tmp;
        tmp.a3 = parseImm(operands[2], err);
        return tmp;

    case REG3:
        tmp.a1 = parseRegister(operands[0], err);
        if (*err == BAD) return tmp;
        tmp.a2 = parseRegister(operands[1], err);
        if (*err == BAD) return tmp;
        tmp.a3 = parseRegister(operands[2], err);
        return tmp;
    
    case IMM8x2:
        tmp.a1 = parseImm(operands[0], err);
        if (*err == BAD) return tmp;
        tmp.a2 = parseImm(operands[1], err);
        return tmp;

    default:
        printf("Something has gone seriously wrong...\n");
        *err = BAD;
        return tmp;
    }
}

int parseStrData(char* str, char* buf, int* err) {
    *err = OK;
    if (str[0] != '"') {
        *err = BAD;
        return 0;
    }
    int c = 0;
    for(int i = 1;;i++) {
        if (str[i] == '"') {
            break;
        } else if (str[i] == 0) {
            *err = BAD;
            return 0;
        } else if (str[i] == '\\') {
            i++;
            switch(str[i]) {
            case '0':
                buf[c] = 0; break;
            case 'n':
                buf[c] = '\n'; break;
            default:
                buf[c] = str[i]; break;
            }
        } else {
            buf[c] = str[i];
        }
        c++;
    }
    return c;
}

int parseLine(char* line, char* buf, int* err) {
    *err = OK;
    if (line[0] == '"') {
        return parseStrData(line, buf, err);
    } else {
        Ins tmp = parseIns(line, err);
        if (*err != OK) {
            return 0;
        }
        buf[0] = tmp.op;
        buf[1] = tmp.a1;
        buf[2] = tmp.a2;
        buf[3] = tmp.a3;
        return 4;
    }
}

void assemble(FILE* in, FILE* out, int* err) {
    *err = OK;
    int read = 0;
    char readc = 0;

    char eof_flag = 0;

    int i = 0;
    for (;;i++) {
        int read;
        char readc;
        for (;;) {
            read = getc(in);
            if (read == EOF) {
                eof_flag = 1;
                break;
            }
            readc = (char)read;
            if (readc != '\n') {
                break;
            }
        }

        if (eof_flag) {
            break;
        }

        char linebuf[256];
        int linelen = 0;
        
        for (;;) {
            if (read == EOF) {
                eof_flag = 1;
                break;
            }
            if (readc == '\n') {
                break;
            }
            
            linebuf[linelen] = readc;
            linelen++;
            read = getc(in);
            readc = (char)read;
        }

        linebuf[linelen] = 0;
        char outbuf[256];
        int outlen = parseLine(linebuf, outbuf, err);
        if (*err != OK) {
            return;
        }

        for(int i = 0; i < outlen;i++) {
            putc(outbuf[i], out);
        }
    }
    *err = OK;
}