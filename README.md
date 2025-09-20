# Status
[My remote processor driver for the rk3506](rk3506_rproc) is already useable.

It loads the MCU FW properly to the SRAM 0xFFF84000, the Cortex-M0 core starts executing the code.
Start, stop, restart also works.

## Not Working
* Unloading: the MCU code can not be changed after it started, because the SRAM beacomes unaccessible to the Linux system, and I don't know how to make it accessible again. For the MCU unloading probably there is a special trusted FW call, but I don't know which...
* The SWD (Serial Wire Debugging) still doesn't work. I assume there is a switch for it in an undocumented/uncaccessible HW.

**If you have some idea or question please create an issue hier on this github page.**

## TODO
* Processing the resource table to be able to load the MCU code to 0xFFF88000 too (the last SRAM segment). This would allow to use 0xFFF81000 - 0xFFF87FFF SRAM addresses for data exchange.

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
There was an AMP example from Luckfox, which I could get running, and examined it. At this example the MCU code loaded and started by the u-boot.
This is pretty developer unfriendly. Examining the u-boot code, I've seen that some Rockchip provided closed-source ARM Trusted code calls might be necessary to control special part of the rk3506,
especially the SRAM mapping (the integrated SRAM can be used as TCM Memory for the MCU Core). So **it seems that the remote processor kernel driver is the only way
to load/start/stop MCU code on a running Linux**.

I've seen, that these remote processor drivers are pretty simple and short so I decided to create one for the rk3506. This is the main purpose of this this github project.
I added clock and reset signal handling using device-tree support. The source code and some infos is in the [rk3506_rproc subdirectory](rk3506_rproc).

# VIHAL Drivers

As I see my remote processor driver working I started developing [VIHAL](https://github.com/nvitya/vihal) drivers for the RK3506 for embedded development, mainly focusing on Cortex-M0 tasks.

I already integrated the RK3506 into the "blinky" and "uart" tests in the [vihaltests](https://github.com/nvitya/vihaltests) repository.

# Testing

I bought a Luckfox Lyra Plus for the testing.
I included the MCU Test firmware in the subdirectory [mcu_firmware](mcu_firmware).
I included some description how I set up my rk3506 linux in the subdirectory [test-configs](test-configs).

