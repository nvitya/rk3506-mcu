// SPDX-License-Identifier: GPL-2.0
// Driver for RK3506 Cortex-M0 remoteproc
//
// Copyright (c) Viktor Nagy <nvitya@gmail.com>
// first version published at https://github.com/nvitya/rk3506-mcu

#define FW_FORMAT_BIN 0

/*

Device-tree block:

	mcu_rproc: mcu@fff84000 {
		compatible = "rockchip,rk3506-mcu";
		reg = <0xfff84000 0x8000>;
		firmware-name = "rk3506-m0.elf";

    // for now include the clocks also that required by the MCU FW
		clocks = <&cru HCLK_M0>, <&cru STCLK_M0>,
		         <&cru PCLK_TIMER>, <&cru CLK_TIMER0_CH5>,
						 <&cru SCLK_UART4>, <&cru PCLK_UART4>;

		resets = <&cru SRST_H_M0>, <&cru SRST_M0_JTAG>, <&cru SRST_HRESETN_M0_AC>;
		reset-names = "h_m0", "m0_jtag", "hresetn_m0_ac";
	};

*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/remoteproc.h>
#include <linux/of_device.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/elf.h>

int rproc_elf_sanity_check(struct rproc *rproc, const struct firmware *fw);
u64 rproc_elf_get_boot_addr(struct rproc * rproc, const struct firmware * fw);
int rproc_elf_load_segments(struct rproc * rproc, const struct firmware * fw);
int rproc_elf_load_rsc_table(struct rproc * rproc, const struct firmware * fw);
struct resource_table *rproc_elf_find_loaded_rsc_table(struct rproc * rproc, const struct firmware * fw);

//#include "remoteproc_internal.h"

/* Rockchip platform SiP call ID */
#define SIP_ATF_VERSION			0x82000001
#define SIP_ACCESS_REG			0x82000002
#define SIP_SUSPEND_MODE		0x82000003
#define SIP_PENDING_CPUS		0x82000004
#define SIP_UARTDBG_CFG			0x82000005
#define SIP_UARTDBG_CFG64		0xc2000005
#define SIP_MCU_EL3FIQ_CFG		0x82000006
#define SIP_ACCESS_CHIP_STATE64		0xc2000006
#define SIP_SECURE_MEM_CONFIG		0x82000007
#define SIP_ACCESS_CHIP_EXTRA_STATE64	0xc2000007
#define SIP_DRAM_CONFIG			0x82000008
#define SIP_SHARE_MEM			0x82000009
#define SIP_SIP_VERSION			0x8200000a
#define SIP_REMOTECTL_CFG		0x8200000b
#define SIP_VPU_RESET			0x8200000c
#define SIP_SOC_BUS_DIV			0x8200000d
#define SIP_LAST_LOG			0x8200000e
#define SIP_ACCESS_MEM_OS_REG		0x8200000f
#define SIP_AMP_CFG			0x82000022
#define SIP_HDCP_CONFIG			0x82000025
#define SIP_MCU_CFG			    0x82000028

/* RK_SIP_MCU_CFG child configs, MCU ID */
#define ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID		0x00
#define ROCKCHIP_SIP_CONFIG_BUSMCU_1_ID		0x01
#define ROCKCHIP_SIP_CONFIG_PMUMCU_0_ID		0x10
#define ROCKCHIP_SIP_CONFIG_DDRMCU_0_ID		0x20
#define ROCKCHIP_SIP_CONFIG_NPUMCU_0_ID		0x30

/* RK_SIP_MCU_CFG child configs */
#define ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR		0x01
#define ROCKCHIP_SIP_CONFIG_MCU_EXPERI_START_ADDR	0x02
#define ROCKCHIP_SIP_CONFIG_MCU_SRAM_START_ADDR		0x03
#define ROCKCHIP_SIP_CONFIG_MCU_EXSRAM_START_ADDR	0x04

#define RK3506_MCU_TCM_ADDR                 0xFFF84000
#define RK3506_MCU_TCM_SIZE                     0x8000

#define RK3506_MCU_SHMEM_ADDR               0x03C00000
#define RK3506_MCU_SHMEM_SIZE                 0x100000

// the register definitions of the beginning of the PMU
//
typedef struct
{
  volatile uint32_t VERSION;            // Offset: 0x0000
  volatile uint32_t PWR_CON;            // Offset: 0x0004
  volatile uint32_t GLB_POWER_STS;      // Offset: 0x0008
  volatile uint32_t INT_MASK_CON;       // Offset: 0x000C  <- MCU reset + MCU int mask
  volatile uint32_t WAKEUP_INT_CON;     // Offset: 0x0010
  volatile uint32_t WAKEUP_INT_ST;      // Offset: 0x0014
           uint32_t _reserved_018[2];   // Offset: 0x0018
//
} rk3506_pmu_regs_t;

#define RK3506_PMU_BASE   0xFF900000
#define RK3506_CRU_BASE   0xFF9A0000
#define RK3506_GRF_BASE   0xFF288000

typedef struct
{
  struct rproc *            rproc;

	struct clk_bulk_data *    clks;
	int                       num_clks;

  struct reset_control *    rst_h_m0;
  struct reset_control *    rst_m0_jtag;
  struct reset_control *    rst_hresetn_m0_ac;

  uint8_t *                 tcm_virt;
  phys_addr_t               tcm_phys;

  // iomapped peripherals, using uint8_t for natural offsets
  uint8_t *                 regs_PMU;  // FF900
  uint8_t *                 regs_CRU;  // FF9A0
  uint8_t *                 regs_GRF;  // FF288

  uint8_t *                 shmem_virt;  // 0x03C00000

  struct platform_device *  pdev;
//
} rk3506_mcu_t;


/* Original, working MCU starter from the Rockchip U-Boot source code:

int fit_standalone_release(char *id, uintptr_t entry_point)
{
	// address map: map 0 to sram, enable TCM mode for sram
	// 0xfff84000 for sram
	// 0x03e00000 for ddr

	sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
		ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
		entry_point);


	// bus m0 configuration:
	//   open m0 swclktck & hclk
	writel(0x0c000000, CRU_BASE + CRU_GATE_CON5);

	// set m0 system time calibration GRF->GRF_SOC_CON36
	writel(0xbcd3d80, 0xff288090);

	// enable m0 interrupt: PMU->PMU_INT_MASK_CON mcu_rst_dis_cfg=1,glb_int_mask_mcu=0
	writel(0x00060004, 0xff90000c);

	// select jtag m1 GPIO0C6 GPIO0C7
	//writel(0x00220000, 0xff960000);
	//writel(0x00300020, 0xff288000);
	//writel(0x00ff0022, 0xff4d8064);
	//writel(0xff002200, 0xff950014);
	return 0;
}

*/

static void rk3506_rproc_mcu_run(rk3506_mcu_t * mcu, bool arun)
{
  if (arun)
  {
    // Release the M0 reset + enable the M0 interrupts (mcu_rst_dis_cfg=1,glb_int_mask_mcu=0)
    writel(0x00060004, mcu->regs_PMU + 0x00C);  // offset 0x00C = PMU_INT_MASK_CON
  }
  else
  {
    // Assert the M0 reset + disable the M0 interrupts (mcu_rst_dis_cfg=0, glb_int_mask_mcu=1)
    writel(0x00060002, mcu->regs_PMU + 0x00C);  // offset 0x00C = PMU_INT_MASK_CON
  }
}

static int rk3506_rproc_start(struct rproc * rproc)
{
  rk3506_mcu_t *        mcu = rproc->priv;
  struct arm_smccc_res  res;
  uint32_t              mcu_entry = 0xFFF84000;  // the code here should start with the ARM Cortex-M vector table

  dev_info(&rproc->dev, "Starting M0 MCU at 0x%08X...", mcu_entry);

  // WARNING: this call makes the SRAM at 0xFFF84000 inaccessible !

  /* address map: map 0 to sram, enable TCM mode for sram
   * 0xfff84000 for sram
   * 0x03e00000 for ddr */
  arm_smccc_smc(SIP_MCU_CFG, ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
                ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
                mcu_entry,
                0, 0, 0, 0, &res);
  if (res.a0)
  {
    dev_err(&rproc->dev, "SMCCC CODE START call error: %i", (int)res.a0);
    return -1;
  }

#if 0
  reset_control_deassert(mcu->rst_m0_jtag);
  reset_control_deassert(mcu->rst_h_m0);
  reset_control_deassert(mcu->rst_hresetn_m0_ac);

  // enable hclk_m0 + swclktck_m0
  writel(0x0c000000, mcu->regs_CRU + 0x814); // offset 0x815 = CRU_GATE_CON5

	// set m0 system time calibration GRF->GRF_SOC_CON36
	writel(0xbcd3d80, mcu->regs_GRF + 0x090);  // value taken from Rockchip U-Boot source code
#endif

  rk3506_rproc_mcu_run(mcu, true);
  return 0;
}

static int rk3506_rproc_stop(struct rproc * rproc)
{
  rk3506_mcu_t *        mcu = rproc->priv;
  //struct arm_smccc_res  res;

  dev_info(&rproc->dev, "Stopping M0 MCU");

  rk3506_rproc_mcu_run(mcu, false);

#if 0
  // Disable M0 TCM to be accessible from here
  arm_smccc_smc(SIP_MCU_CFG, ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
                ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
                0xFFF8C000,
                0, 0, 0, 0, &res);
  if (res.a0)
  {
    dev_err(&rproc->dev, "SMCCC MCU TCM Unmap error: %i", (int)res.a0);
  }
#endif

  return 0;
}

#if FW_FORMAT_BIN

static int rk3506_rproc_load(struct rproc * rproc, const struct firmware * fw)
{
  rk3506_mcu_t *  mcu = rproc->priv;

  if (fw->size > RK3506_MCU_TCM_SIZE)
  {
    dev_err(&rproc->dev, "M0 MCU FW is too big: size=%u", (uint32_t)fw->size);
    return -EINVAL;
  }
  dev_info(&rproc->dev, "Loading FW: virt_addr=0x%08X, size=%u", (uint32_t)mcu->tcm_virt, (uint32_t)fw->size);

  memcpy_toio(mcu->tcm_virt, fw->data, fw->size);
  return 0;
}

#else

static void * my_da_to_va(struct rproc * rproc, u64 da, size_t len, bool * is_iomem)
{
  rk3506_mcu_t *  mcu = rproc->priv;
  void __iomem * va;

  if (da + len <= 0x8000)  // change TCM local addresses
  {
    va = mcu->tcm_virt + da;  // mapped base + offset
  }
  else if ((da <= RK3506_MCU_SHMEM_ADDR) && (len <= RK3506_MCU_SHMEM_SIZE))
  {
    va = mcu->shmem_virt + (da - RK3506_MCU_SHMEM_ADDR);  // mapped base + offset
  }
  else
  {
    dev_err(&rproc->dev, "Invalid rproc address: 0x%08X, len=%d", (uint32_t)da, len);
    va = NULL;
  }

  return va;
}

#endif

static const struct rproc_ops rk3506_rproc_ops =
{
  .start = rk3506_rproc_start,
  .stop = rk3506_rproc_stop,

#if FW_FORMAT_BIN
  .load = rk3506_rproc_load,
#else
  .da_to_va = my_da_to_va,
	.load = rproc_elf_load_segments,
	// .parse_fw = rk3506_rproc_parse_fw,
	.find_loaded_rsc_table = rproc_elf_find_loaded_rsc_table,
	.sanity_check = rproc_elf_sanity_check,
	.get_boot_addr = rproc_elf_get_boot_addr,
#endif

};

static const char * rk3506_rproc_get_firmware(struct platform_device * pdev)
{
	const char * fw_name;
	int ret;

	ret = of_property_read_string(pdev->dev.of_node, "firmware-name",
				      &fw_name);
	if (ret)
		return ERR_PTR(ret);

	return fw_name;
}

static int rk3506_rproc_probe(struct platform_device * pdev)
{
  struct rproc *      rproc;
  rk3506_mcu_t *      mcu;
  //struct resource *   res;
  int                 ret;
	const char *        firmware;

	firmware = rk3506_rproc_get_firmware(pdev);
	if (IS_ERR(firmware))
  {
    dev_err(&pdev->dev, "error getting firmware-name from the device-tree");
		return PTR_ERR(firmware);
  }

  rproc = rproc_alloc(&pdev->dev, dev_name(&pdev->dev),
                      &rk3506_rproc_ops, firmware, sizeof(rk3506_mcu_t));
  if (!rproc)
  {
    return -ENOMEM;
  }
  mcu = rproc->priv;
  mcu->rproc = rproc;

	mcu->num_clks = devm_clk_bulk_get_all(&pdev->dev, &mcu->clks);
	if (mcu->num_clks < 0)
  {
    dev_err(&pdev->dev, "error getting all clocks from the device-tree");
    rproc_free(rproc);
		return -ENODEV;
  }

  //res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  //mcu->tcm_virt = devm_ioremap_resource(&pdev->dev, res);

  mcu->tcm_phys = RK3506_MCU_TCM_ADDR;
  mcu->tcm_virt = ioremap(RK3506_MCU_TCM_ADDR, RK3506_MCU_TCM_SIZE);
  if (IS_ERR(mcu->tcm_virt))
  {
    rproc_free(rproc);
    return PTR_ERR(mcu->tcm_virt);
  }
  mcu->shmem_virt = ioremap(RK3506_MCU_SHMEM_ADDR, RK3506_MCU_SHMEM_SIZE);

  // make the PMU accessible for this module
  mcu->regs_PMU = ioremap(RK3506_PMU_BASE, 4096);
  mcu->regs_CRU = ioremap(RK3506_CRU_BASE, 4096);
  mcu->regs_GRF = ioremap(RK3506_GRF_BASE, 4096);

  mcu->pdev = pdev;

  // ensure that the MCU is in reset !
  rk3506_rproc_mcu_run(mcu, false);

  // clocks

	ret = clk_bulk_prepare_enable(mcu->num_clks, mcu->clks);
	if (ret)
  {
    dev_err(&pdev->dev, "Error enabling all the specified clocks: %d", ret);
  }

  // enable hclk_m0 + swclktck_m0
  writel(0x0c000000, mcu->regs_CRU + 0x814); // offset 0x815 = CRU_GATE_CON5

	// set m0 system time calibration GRF->GRF_SOC_CON36
	writel(0xbcd3d80, mcu->regs_GRF + 0x090);  // value taken from Rockchip U-Boot source code

/*
  mcu->hclk_m0 = devm_clk_get(&pdev->dev, "hclk_m0");
  if (IS_ERR(mcu->hclk_m0))
  {
    dev_err(&pdev->dev, "error getting clock: hclk_m0");
    return PTR_ERR(mcu->hclk_m0);
  }

  mcu->stclk_m0 = devm_clk_get(&pdev->dev, "stclk_m0");
  if (IS_ERR(mcu->stclk_m0))
  {
    dev_err(&pdev->dev, "error getting clock: stclk_m0");
    return PTR_ERR(mcu->stclk_m0);
  }
*/

  // resets

  mcu->rst_h_m0 = devm_reset_control_get(&pdev->dev, "h_m0");
  if (IS_ERR(mcu->rst_h_m0))
  {
    dev_err(&pdev->dev, "error getting reset: h_m0");
    return PTR_ERR(mcu->rst_h_m0);
  }

  mcu->rst_m0_jtag = devm_reset_control_get(&pdev->dev, "m0_jtag");
  if (IS_ERR(mcu->rst_m0_jtag))
  {
    dev_err(&pdev->dev, "error getting reset: m0_jtag");
    return PTR_ERR(mcu->rst_m0_jtag);
  }

  mcu->rst_hresetn_m0_ac = devm_reset_control_get(&pdev->dev, "hresetn_m0_ac");
  if (IS_ERR(mcu->rst_hresetn_m0_ac))
  {
    dev_err(&pdev->dev, "error getting reset: hresetn_m0_ac");
    return PTR_ERR(mcu->rst_hresetn_m0_ac);
  }

  reset_control_deassert(mcu->rst_m0_jtag);
  reset_control_deassert(mcu->rst_h_m0);
  reset_control_deassert(mcu->rst_hresetn_m0_ac);

  platform_set_drvdata(pdev, rproc);

  ret = rproc_add(rproc);
  if (ret)
    return ret;

  return ret;
}

static void rk3506_rproc_shutdown(struct platform_device * pdev)
{
  struct rproc *   rproc = platform_get_drvdata(pdev);
  //rk3506_mcu_t *   mcu = rproc->priv;

  rk3506_rproc_stop(rproc);

  // disable hclk_m0 + swclktck_m0
  //writel(0x0c0000c0, mcu->regs_CRU + 0x814); // offset 0x815 = CRU_GATE_CON5

  //reset_control_assert(mcu->rst_m0_jtag);
  //reset_control_assert(mcu->rst_h_m0);
  //reset_control_assert(mcu->rst_hresetn_m0_ac);

	//clk_bulk_disable_unprepare(mcu->num_clks, mcu->clks);
}

static int rk3506_rproc_remove(struct platform_device * pdev)
{
  struct rproc *   rproc = platform_get_drvdata(pdev);
  rk3506_mcu_t *   mcu = rproc->priv;

  rk3506_rproc_shutdown(pdev);

  if (mcu->tcm_virt)    iounmap(mcu->tcm_virt);
  if (mcu->shmem_virt)  iounmap(mcu->shmem_virt);

  if (mcu->regs_PMU)    iounmap(mcu->regs_PMU);
  if (mcu->regs_CRU)    iounmap(mcu->regs_CRU);
  if (mcu->regs_GRF)    iounmap(mcu->regs_GRF);

  rproc_del(rproc);
  rproc_free(rproc);
  return 0;
}

static const struct of_device_id rk3506_rproc_match[] =
{
  { .compatible = "rockchip,rk3506-mcu" },
  {}
};
MODULE_DEVICE_TABLE(of, rk3506_rproc_match);

static struct platform_driver rk3506_rproc_driver =
{
  .probe = rk3506_rproc_probe,
  .remove = rk3506_rproc_remove,  // when the module unloaded
  .shutdown = rk3506_rproc_shutdown,  // when the system shut down or restarted
  .driver = {
    .name = "rk3506_mcu_rproc",
    .of_match_table = rk3506_rproc_match,
  },
};

module_platform_driver(rk3506_rproc_driver);

MODULE_AUTHOR("nvitya");
MODULE_DESCRIPTION("RK3506 Cortex-M0 Remote Processor Driver");
MODULE_LICENSE("GPL v2");

