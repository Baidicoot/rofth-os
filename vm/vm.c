#include <stdint.h>
#include <stdio.h>

//#define DEBUG

#define SEG_SIZE 2048

#define READONLY_SIZE 32
#define READONLY SEG_SIZE - READONLY_SIZE

#define PPT 1024
#define MAX_PROC 16

#define PRC PPT + 6*MAX_PROC    // one byte for the current process
#define INT_PRC PRC + 1         // one byte for the return process
#define INT_RET INT_PRC + 1     // two bytes for the return address
#define INT_REQ INT_RET + 2     // one byte for the requested interrupt code
#define INT_HAND INT_REQ + 1    // two byte pointer to the interrupt handler

char mem[65536];

#define INT(code, offset) \
*(uint16_t*)(mem + INT_RET) = pc + offset; \
mem[INT_PRC] = mem[PRC]; \
mem[PRC] = 0; \
pc = *(uint16_t*)(mem + INT_HAND);

#define OFFSET(prc) *(uint16_t*)(mem + PPT + 6*prc)

#define MEMEXCEPT INT(0, 4)

int isReadable(uint16_t addr, uint8_t prc) {
    uint32_t legality = *(uint32_t*)(mem + PPT + 2 + 6*prc);
    return (((legality >> 31) | (legality >> (addr / SEG_SIZE))) & 1);
}

int isWriteable(uint16_t unaddr, uint16_t addr, uint8_t prc) {
    uint32_t legality = *(uint32_t*)(mem + PPT + 2 + 6*prc);
    return ((legality >> 31) | ((legality >> (addr / SEG_SIZE)) & (unaddr < READONLY)) & 1);
}

/*
MEMORY PROTECTION
Memory is divided into 32-2KB 'segments', and processes are only allowed to access certain segments.
segment 0 is for the bootloader/OS, and can only be accessed by the OS itself (process 0).
The 31 other segments are for general use by programs, and adresses are automatically translated to their physical counterpart.

In segment 0, at address 1024, lies the 'process permission table' (PPT). This details which segments are accessable to which processes, and also the relative offset of each process.
A maximum of 16 processes are supported, along with 2 process permission levels (PPLs).

The PPT is organised like: (in bits)
offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
00      of of of of of of of of of of of of of of of of
01      pm sg sg sg sg sg sg sg sg sg sg sg sg sg sg sg
02      sg sg sg sg sg sg sg sg sg sg sg sg sg sg sg sg

where
of = offset bit
pm = permission bit
sg = segment allowed bit

for a total of 6 bytes per process, or 16 * 6 = 96 bytes overall

the current process is stored as a byte at address 1120, and so can only be managed by the OS.

calling OS processes is done with the INT instruction, in the form:
INT code offset

*/

enum INS {
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

    INT,
};

void run() {
    mem[1120] = 0;
    *(uint32_t*)(mem + 1026) = 1 << 31;
    uint16_t pc = 0;
    uint8_t op;
    uint8_t reg8[32]; // 32 8-bit registers, 16 16-bit registers, 8 32-bit registers
    uint16_t* reg16 = reg8;
    uint32_t* reg32 = reg8;
    do {
        if (isReadable(pc, mem[PRC])) {
            op = mem[pc];
        } else {
            printf("FATAL: INSTRUCTION OVERFLOW\n");
            return;
        }

        #ifdef DEBUG
        printf("%i:\n", pc);
        #endif

        switch (op)
        {
        case LIM: {
            uint8_t reg = mem[pc + 1];
            uint16_t val = *(uint16_t*)(mem + pc + 2);
            reg16[reg] = val;
            break;
        }
        
        case LD8: {
            uint8_t reg = mem[pc + 1];
            uint8_t addrreg = mem[pc + 2];
            int8_t offset = mem[pc+3];

            uint8_t prc = mem[PRC];
            uint16_t addr = OFFSET(prc) + offset + reg16[addrreg];
            if (isReadable(addr, prc)) {
                #ifdef DEBUG
                printf("loaded addr %i into 8r%i\n", addr, reg);
                #endif
                reg8[reg] = mem[addr];
            } else {
                MEMEXCEPT
            }
            break;
        }

        case LD16: {
            uint8_t reg = mem[pc + 1];
            uint8_t addrreg = mem[pc + 2];
            int8_t offset = mem[pc+3];

            uint8_t prc = mem[PRC];
            uint16_t addr = OFFSET(prc) + offset + reg16[addrreg];
            if (isReadable(addr, prc)) {
                #ifdef DEBUG
                printf("loaded addr %i into 16r%i\n", addr, reg);
                #endif
                reg16[reg] = *(uint16_t*)(mem + addr);
            } else {
                MEMEXCEPT
            }
            break;
        }

        case LD32: {
            uint8_t reg = mem[pc + 2];
            uint8_t addrreg = mem[pc + 1];
            int8_t offset = mem[pc+3];
            
            uint8_t prc = mem[PRC];
            uint16_t addr = OFFSET(prc) + offset + reg16[addrreg];
            if (isReadable(addr, prc)) {
                #ifdef DEBUG
                printf("loaded addr %i into 16r%i\n", addr, reg);
                #endif
                reg32[reg] = *(uint32_t*)(mem + addr);
            } else {
                MEMEXCEPT
            }
            break;
        }
        
        case SV8: {
            uint8_t reg = mem[pc + 1];
            uint8_t addrreg = mem[pc + 2];
            int8_t offset = mem[pc+3];
            
            uint8_t prc = mem[PRC];
            uint16_t unaddr = offset + reg16[addrreg];
            uint16_t addr = OFFSET(prc) + unaddr;
            if (isWriteable(unaddr, addr, prc)) {
                #ifdef DEBUG
                printf("wrote: 8x%i to: %i\n", reg8[reg], addr);
                #endif
                mem[addr] = reg8[reg];
            } else {
                MEMEXCEPT
            }
            break;
        }

        case SV16: {
            uint8_t reg = mem[pc + 1];
            uint8_t addrreg = mem[pc + 2];
            int8_t offset = mem[pc+3];
            
            uint8_t prc = mem[PRC];
            uint16_t unaddr = offset + reg16[addrreg];
            uint16_t addr = OFFSET(prc) + unaddr;
            if (isWriteable(unaddr, addr, prc)) {
                #ifdef DEBUG
                printf("wrote: 16x%i to: %i\n", reg16[reg], addr);
                #endif
                *(uint16_t*)(mem + addr) = reg16[reg];
            } else {
                MEMEXCEPT
            }
            break;
        }

        case SV32: {
            uint8_t reg = mem[pc + 1];
            uint8_t addrreg = mem[pc + 2];
            int8_t offset = mem[pc+3];
            
            uint8_t prc = mem[PRC];
            uint16_t unaddr = offset + reg16[addrreg];
            uint16_t addr = OFFSET(prc) + unaddr;
            if (isWriteable(unaddr, addr, prc)) {
                #ifdef DEBUG
                printf("wrote: 32x%i to: %i\n", reg32[reg], addr);
                #endif
                *(uint32_t*)(mem + addr) = reg32[reg];
            } else {
                MEMEXCEPT
            }
            break;
        }

        case AND: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("r%i & r%i -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = reg16[srcreg1] & reg16[srcreg2];
            break;
        }
        
        case OR: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("r%i | r%i -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = reg16[srcreg1] | reg16[srcreg2];
            break;
        }

        case XOR: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("r%i ^ r%i -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = reg16[srcreg1] ^ reg16[srcreg2];
            break;
        }

        case NOR: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("~(r%i | r%i) -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = ~(reg16[srcreg1] | reg16[srcreg2]);
            break;
        }

        case ADD: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("r%i + r%i -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = reg16[srcreg1] + reg16[srcreg2];
            break;
        }

        case ADDC: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("r%i + r%i + 1 -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = reg16[srcreg1] + reg16[srcreg2] + 1;
            break;
        }

        case SHIFTL: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("r%i << r%i -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = reg16[srcreg1] << reg16[srcreg2];
            break;
        }

        case SHIFTR: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg1 = mem[pc+2];
            uint8_t srcreg2 = mem[pc+3];
            #ifdef DEBUG
            printf("r%i >> r%i -> r%i\n", srcreg1, srcreg2, destreg);
            #endif
            reg16[destreg] = reg16[srcreg1] >> reg16[srcreg2];
            break;
        }

        case LJAL: {
            uint8_t destreg = mem[pc+1];
            uint8_t srcreg = mem[pc+2];
            int8_t offset = mem[pc+3];

            uint8_t prc = mem[1120];
            uint16_t newpos = reg16[srcreg] - 4;
            reg16[destreg] = pc + offset + 4;
            if (isReadable(newpos, prc)) {
                #ifdef DEBUG
                printf("LJAL from %i to %i\n", reg16[destreg], newpos + 4);
                #endif
                pc = newpos;
            } else {
                MEMEXCEPT
            }
            break;
        }

        case BEQ: {
            uint8_t srcreg1 = mem[pc+1];
            uint8_t srcreg2 = mem[pc+2];
            int8_t offset = mem[pc+3];
            if (reg16[srcreg1] == reg16[srcreg2]) {
                uint8_t prc = mem[1120];
                uint16_t newpos = pc + offset;
                if (isReadable(newpos, prc)) {
                    #ifdef DEBUG
                    printf("r%i == r%i; pc <- %i\n", srcreg1, srcreg2, newpos);
                    #endif
                    pc = newpos;
                } else {
                    MEMEXCEPT
                }
            }
            break;
        }

        case BNE: {
            uint8_t srcreg1 = mem[pc+1];
            uint8_t srcreg2 = mem[pc+2];
            int8_t offset = mem[pc+3];
            if (reg16[srcreg1] != reg16[srcreg2]) {
                uint8_t prc = mem[1120];
                uint16_t newpos = pc + offset;
                if (isReadable(newpos, prc)) {
                    #ifdef DEBUG
                    printf("r%i == r%i; pc <- %i\n", srcreg1, srcreg2, newpos);
                    #endif
                    pc = newpos;
                } else {
                    MEMEXCEPT
                }
            }
            break;
        }
        
        case BLT: {
            uint8_t srcreg1 = mem[pc+1];
            uint8_t srcreg2 = mem[pc+2];
            int8_t offset = mem[pc+3];
            if (reg16[srcreg1] < reg16[srcreg2]) {
                uint8_t prc = mem[1120];
                uint16_t newpos = pc + offset;
                if (isReadable(newpos, prc)) {
                    #ifdef DEBUG
                    printf("r%i == r%i; pc <- %i\n", srcreg1, srcreg2, newpos);
                    #endif
                    pc = newpos;
                } else {
                    MEMEXCEPT
                }
            }
            break;
        }
        
        case BGT: {
            uint8_t srcreg1 = mem[pc+1];
            uint8_t srcreg2 = mem[pc+2];
            int8_t offset = mem[pc+3];
            if (reg16[srcreg1] > reg16[srcreg2]) {
                uint8_t prc = mem[1120];
                uint16_t newpos = pc + offset;
                if (isReadable(newpos, prc)) {
                    #ifdef DEBUG
                    printf("r%i == r%i; pc <- %i\n", srcreg1, srcreg2, newpos);
                    #endif
                    pc = newpos;
                } else {
                    MEMEXCEPT
                }
            }
            break;
        }

        case INT: {
            uint8_t code = mem[pc+1];
            int8_t offset = mem[pc+2];

            #ifdef DEBUG
            printf("interrupt; c:%i, o:%i\n", code, offset);
            #endif

            INT(code, offset)
        }

        default: break;
        }
        pc += 4;

        /* IO */

        #ifdef DEBUG
        printf("stack: ptr: %i, top: %i\n", reg16[0], *(uint16_t*)(mem + reg16[0]));
        #endif

        // mem[1022] is the write flag
        if (mem[1022] != 0) {
            fputc(mem[1023], stdout);
            mem[1022] = 0;
        }

    } while (op != HLT);
    return;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("not enough arguments");
        return -1;
    }

    if (argv[1][0] == '-' && argv[1][1] == 'l') {
        if (argc < 3) {
            printf("not enough arguments");
            return -1;
        }

        FILE* f = fopen(argv[2], "r");
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        rewind(f);
        fread(mem, 1, fsize, f);
        fclose(f);
        run();
        return 1;
    } else {
        printf("unrecognised command %s", argv[1]);
        return 2;
    }
}