// Minimal CORTEX-M0 code to test if a remote CPU is running

    .thumb

    .section .text
    .align   2

    .thumb_func
    .type    _start, %function
    .globl   _start
    .fnstart
_start:

    ldr      r2, =gcounter_inc
    ldr      r3, =gcounter_dec
    mov      r0, #0
    mov      r1, #0

.main_loop:
    add      r0, r0, #1
    sub      r1, r1, #1

    str      r0, [r2]
    str      r1, [r3]
    b .main_loop

    .fnend

//-----------------------------------------------------

    .section  .data
    .align    4
g_version_marker:
    .long     0x12345678
gcounter_inc:
    .long     0
gcounter_dec:
    .long     0

    .end
