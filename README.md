# Status
[My remote processor driver for the rk3506](rk3506_rproc) is already usable.

It successfully loads the MCU firmware to SRAM at address 0xFFF84000, the Cortex-M0 core begins executing the loaded code.
Start, stop, restart also works.

## Not Working
* **Hot-swapping the MCU FW**: once the MCU code is running, the SRAM becomes inaccessible and cannot be modified by the Linux system (until the next restart), and I don't know how to make it accessible again. For the SRAM unmapping probably there is a special trusted FW call, but I don't know which...
* **SWD (Serial Wire Debugging)**: I assume there is a switch for it in an undocumented/uncaccessible HW. (Maybe SGRF_MPU 0xff960000 <= 0x00220000 ?)

**If you have some idea or question please create an issue here on this github page.**

## TODO
* Processing the resource table to be able to load the MCU code to 0xFFF88000 too (the last SRAM segment). This would allow to use 0xFFF81000 - 0xFFF87FFF SRAM addresses for data exchange.

# Introduction

The Rockchip rk3506 is a very promising Single-Linux SoC and there are multiple low-cost boards available with this chip.
It is advertised with the Cortex-M0 Core running at 200 MHz (?) along with the three ARM-A7 Linux cores. 
Theoretically you can even debug the Cortex-M0 code over SWD, which is amazing for the development.

I want to drive an external SPI ADC with 64 kHz sampling rate. This is not possible with the A7 Cores from Linux userspace, 
but would be very easy using the separated Cortex-M0 Core. 

I've already implemented this external ADC sampling on a Milk-V Duo. For the Milk-V Duo, the manufacturer(s) provide a 
remote processor driver for their linux. With this remote processor driver, the "MCU" bare-metal code can be loaded, started and stopped
using the kernel file interface at `/sys/class/remoteproc/remoteproc0`. This is the recommended way controlling an extra processor, implemented also by big names like ST, NXP or Ti.

# Remote Processor Driver for the rk3506

Unfortunately neither the chip manufacturer (Rockchip) nor the board manufacturer provide remote processor driver for the rk3506 (or any Rockchip SoCs).
I found an AMP example from Luckfox that I successfully ran and analyzed. In this example, the MCU code is loaded and started by U-Boot.
This approach is not very developer-friendly. Examining the u-boot code, I've seen that some Rockchip provided closed-source ARM Trusted code calls might be necessary to control special part of the rk3506,
especially the SRAM mapping (the integrated SRAM can be used as TCM Memory for the MCU Core). So **it seems that the remote processor kernel driver is the only way
to load/start/stop MCU code on a running Linux**.

I've seen, that these remote processor drivers are pretty simple and short so I decided to create one for the rk3506. This is the main purpose of this github project.
I added clock and reset signal handling using device-tree support. The source code and some info is in the [rk3506_rproc subdirectory](rk3506_rproc).

# Rockchip AMP Example

In the Luckfox SDK there is a build confi "rk3506g_buildroot_spinand_amp_defconfig". This generates a NAND Flash image where the u-boot loads an example code into the Cortex-M0. By default this does nothing else than writing the string "[HAL INFO] Hello RK3506 mcu" once to the UART4_TX at 1.C2.

I've modified this example to continuously send something to the UART:
```C
    uint32_t hbcnt = 0;
    while (1)
    {
      HAL_DBG("MCU hbcnt = %d\r\n", hbcnt);
      HAL_DelayUs(500000);
      ++hbcnt;
    }
```
This version served as my primary test source for a while.

However, when I attempted to load this code using my remoteproc driver, it failed to start — costing me considerable time to diagnose the root cause.

The Rockchip MCU HAL example does not include clock gating configuration. Instead, it assumes that peripheral clocks (such as those for UART4 and TIMER5) are already enabled externally. While these clocks were active during U-Boot, the Linux kernel later disabled unused clocks—including UART4 and TIMER5 — causing the MCU code to halt unexpectedly.

Rockchip provides a workaround via the rockchip-amp kernel module. This module requires a device tree entry that explicitly lists and re-enables the peripherals used by the MCU firmware — specifically TIMER5 and UART4 in this case.

This is why it's generally better to start the MCU firmware _after_ the kernel has fully initialized: it ensures that the necessary clocks are properly managed and available.

# VIHAL Drivers

Once my remote processor driver is functioning properly I started developing [VIHAL](https://github.com/nvitya/vihal) drivers for the RK3506 for embedded development, mainly focusing on Cortex-M0 tasks.

I already integrated the RK3506 into the "blinky" and "uart" tests in the [vihaltests](https://github.com/nvitya/vihaltests) repository.

# Testing

I bought a Luckfox Lyra Plus for the testing.
I included the MCU Test firmware in the subdirectory [mcu_firmware](mcu_firmware).
I've included setup instructions for configuring the rk3506 Linux system in the [test-configs](test-configs) subirectory.

