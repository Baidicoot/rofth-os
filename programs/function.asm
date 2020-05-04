# don't use r15 (16-bit); this is left as a zero-register

#!stdout-loc d1022

#!func      s16 r1 r0 d0
#!return    l16 r1 r0 d0 ; lim r2 d-2 ; add r0 r0 r2 ; jal r1 r1 d0
#!call      adc r0 r0 r15 ; adc r0 r0 r15 ; jal r1 r1 d0

.boot
lim r0 d2000

lim r1 .putchar
lim r2 'h
@call

lim r1 .putchar
lim r2 'i
@call

lim r1 .putchar
lim r2 d10
@call

hlt

.putchar
@func

lim r3 @stdout-loc
s08 r4 r3 d1
s08 r4 r3 d0

@return
