# Test Firmware for the Cortex-M0 Core

I created a simple FW to test if the Cortex-M0 core is running.
The "mcu_min_asm" is a super-minimalistic test program for the Cortex-M0 core, written in assembly.
Actually it runs on any Cortex-M0 core, I verified it on a RP2040.

## rk3506_min_asm.elf Structure
This simple firmware starts right at the beginning, the entry address is 0x0001 (the +1 comes from thumb code request).
The code (.text) segment is 28 Bytes long, placed at 0x0000.
The data (.data) segment is 12 Bytes long, placed at 0x0100.

The data segment consists of three 32-bit words:
```
0x0100: marker = 0x12345678
0x0104: gcounter_inc: incrementing counter
0x0108: gcounter_dec: decrementing counter
```
As the MCU runs from the SRAM at 0xFFF84000, you can examine the data segment from the Linux using these addresses: 
```
0xFFF84100: marker = 0x12345678
0xFFF84104: gcounter_inc: incrementing counter
0xFFF84108: gcounter_dec: decrementing counter
```
I recommend to use the `devmem` utility.

## Main Source Code
```ASM
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
```

## Loading
The rk3506_min_asm.elf must be loaded into the Cortex-M0 core, for example via the remote processor driver of this this github repo. It must be copied into the /lib/firmware first and a symlink can be made to it with a name "rk3506-m0.elf" for the simplicity.

Alternativelly you can generate a binary version:

`arm-none-eabi-objcopy -I ihex -O binary RK3506_M0/rk3506_min_asm.hex rk3506_min_asm.bin`

## Compiling
The "mcu_min_asm" is a Eclipse project, you need only the Eclipse Embedded CDT Plugin with an Eclipse CDT to open it.
