.boot
lim r8 .printStr
lim r9 d1125
s16 r8 r9 d0
lim r0 .handler-data
int d0 d0

.printStr
lim r1 d0
lim r2 d1022
# clean r3
lim r3 d0
# load char to start of r3
l08 r0 r6 d0
# putchar
s08 r6 r2 d1
s08 r6 r2 d0
# inc + loop
adc r0 r1 r0
bne r3 r1 d-24

.handler-data
"multiple of four chars long\0"