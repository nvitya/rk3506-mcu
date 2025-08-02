# Status: Help Needed!
[My remote processor driver for the rk3506](rk3506_rproc) is still not working, it can not start the FW on the Cortex-M0 core.
I've also created a very simple [MCU Test FW](mcu_firmware) to test it.

## What can be wrong?
* Maybe additional clocks are missing
* Additional resets must be controlled
* Additional trusted FW calls are missing
* u-boot configuration is missing?
* kernel configuration is missing?

**If you have some idea or question please create an issue hier on this github page.**

# Introduction

The Rochchip rk3506 is a very promising Single-Linux SoC and there are multiple low-cost boards available with this chip.
It is advertised with the Cortex-M0 Core running at 200 MHz (?) along with the three ARM-A7 Linux cores. 
Theoretically you can even debug the Cortex-M0 code over SWD, which is amazing for the development.

I want to drive an external SPI ADC with 64 kHz sampling rate. This is not possible with the A7 Cores from Linux userspace, 
but would be very easy using the separated Cortex-M0 Core. 

I've already implemented this external ADC sampling on a Milk-V Duo. For the Milk-V Duo, the manufacturer(s) provide a 
remote processor driver for their linux. With this remote processor driver, the "MCU" bare-metal code can be loaded, started and stopped
using the kernel file interface at `/sys/class/remoteproc/remoteproc0`. This is the recommended way controlling an extra processor, implemented also by big names like ST, NXP or Ti.

# Remote Processor Driver for the rk3506

Unfortunately neither the chip manufacturer (Rockchip) nor the board manufacturer provide remote processor driver for the rk3506 (or any Rockchip SoCs).
Unfortunately I could not find any working example using the MCU Core. There are some traces in the rockchip u-boot source code which loads a special amp.img 
but I don't know how to get it working.

Further examining u-boot code, I've seen that some Rockchip provided closed-source ARM Trusted code calls might be necessary to control special part of the rk3506,
especially the SRAM mapping (the integrated SRAM can be used as TCM Memory for the MCU Core). So **it seems that the remote processor kernel driver is the only way
to load/start/stop MCU code on a running Linux**.

I've seen, that these remote processor drivers are pretty simple and short so I decided to create one for the rk3506. This is the main purpose of this this github project. The ChatGPT could even generate me the base skeleton, and observing the Cvitek driver (Milk-V Duo) I made some adjustments, like ELF loading. 
I added clock and reset signal handling using device-tree support. The source code and some infos is in the [rk3506_rproc subdirectory](rk3506_rproc).

# Testing

I bougt a Luckfox Lyra Plus for the testing.
I included the MCU Test firmware in the subdirectory [mcu_firmware](mcu_firmware).
I included some description how I set up my rk3506 linux in the subdirectory [test-configs](test-configs).

