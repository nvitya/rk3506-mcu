# RK3506 Cortex-M0 MCU Remote Processor Driver

## Own Remote Processor Driver

I've seen, that the remote processor drivers are actually pretty simple and short so I decided to create one for the rk3506. The ChatGPT could even generate me the base skeleton, and observing the Cvitek driver (Milk-V Duo) I made some adjustments, like ELF loading. I added clock and reset signal handling supported by the device-tree.

## Accessing Information about the rk3506

Unfortunately at the beginning it was hard to get any information about the RK3506 because the Reference Manual
was not available, but around 18th July "DeciHD" uploaded an [rk3506 TRM](https://github.com/DeciHD/rockchip_docs/tree/main/rk3506)

Even having the TRM some parts are missing, like the SRAM mapping control or having clear instructions how to start or stop the MCU.

So the main information sources about this issue are still kernel and u-boot sources.
In the rockchip u-boot code I found a very relevant part in the file [arch/arm/mach-rockchip/rk3506/rk3506.c](https://github.com/rockchip-linux/u-boot/blob/next-dev/arch/arm/mach-rockchip/rk3506/rk3506.c):

```C
int fit_standalone_release(char *id, uintptr_t entry_point)
{
	/* address map: map 0 to sram, enable TCM mode for sram
	 * 0xfff84000 for sram
	 * 0x03e00000 for ddr */
	sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
		ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
		entry_point);

	/*
	* bus m0 configuration:
	* open m0 swclktck & hclk
	*/
	writel(0x0c000000, CRU_BASE + CRU_GATE_CON5);

	/* set m0 system time calibration GRF->GRF_SOC_CON36 */
	writel(0xbcd3d80, 0xff288090);

	/* enable m0 interrupt: PMU->PMU_INT_MASK_CON mcu_rst_dis_cfg=1,glb_int_mask_mcu=0 */
	writel(0x00060004, 0xff90000c);

	/* select jtag m1 GPIO0C6 GPIO0C7 */
	//writel(0x00220000, 0xff960000);
	//writel(0x00300020, 0xff288000);
	//writel(0x00ff0022, 0xff4d8064);
	//writel(0xff002200, 0xff950014);
	return 0;
}
```


