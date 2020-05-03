# don't use r15 (16-bit); this is left as a zero-register
.boot

#!stdout-loc d1022
#!ppt-loc d1024
#!prc-loc d1120
#!intprc-loc d1121
#!intret-loc d1122
#!intreq-loc d1124
#!inthand-loc d1125

lim r0 .int-handle
lim r1 @inthand-loc
s16 r0 r1 d5

.putstr # expects r0 to point to the string
lim r2 @stdout-loc
lim r3 d0

l08 r0 r6 d0

s08 r6 r2 d1
s08 r6 r2 d0

adc r0 r15 r0
bne r3 r15 d-24

lim r0 @intret-loc  #
l16 r0 r0 d0        # jump to return handler (stored in same page as program)
jal r0 r0 d0        #

.int-ret # return handler to be copied to the OS section of a program, with some bytes changed
