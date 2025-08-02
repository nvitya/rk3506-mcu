# rk3506-mcu
Tools and notes how to use the Cortex-M0 MCU core on the Rockchip rk3506

# Status: Help Needed!
The remote processor driver is still not working, it is unable to start the FW on the Cortex-M0 core.

# Introduction

The Rochchip rk3506 is a very promising Single-Linux SoC and there are multiple low-cost boards available with this chip.
It is advertised with the Cortex-M0 Core running at 200 MHz (?) along with the three ARM-A7 Linux cores.

I want to drive an external sPI ADC with 64 kHz sampling rate. This is not possible with the A7 Cores from Linux userspace, 
but would be very easy using the separated Cortex-M0 Core. 

I've already implemented this external ADC sampling on a Milk-V Duo. For the Milk-V Duo, the manufacturer(s) provide a 
remote processor driver for their linux. With this remote processor driver, the "MCU" bare-metal code can be loaded, started and stopped
using the kernel file interface at `/sys/class/remoteproc/remoteproc0`. This is the recommended way controlling an extra processor, implemented by big names like ST, NXP or Ti.

# Remote Processor Driver for rk3506

to be continued

(I bougt a Luckfox Lyra Plus for the testing.)
