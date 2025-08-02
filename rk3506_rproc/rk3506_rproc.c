// SPDX-License-Identifier: GPL-2.0
// Driver for RK3506 Cortex-M0 remoteproc
//
// Copyright (c) Viktor Nagy <nvitya@gmail.com>
// first version published at https://github.com/nvitya/rk3506-mcu


/*

Device-tree block:

	mcu_rproc: mcu@fff84000 {
		compatible = "rockchip,rk3506-mcu";
		reg = <0xfff84000 0x8000>;
		firmware-name = "rk3506-m0.elf";
		clocks = <&cru HCLK_M0>, <&cru STCLK_M0>;
		clock-names = "hclk_m0", "stclk_m0";
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

#define RK3506_MCU_TCM_ADDR                       0xfff84000
#define RK3506_MCU_TCM_SIZE                       0x8000

typedef struct
{
  struct rproc *            rproc;

  struct clk *              hclk_m0;
  struct clk *              stclk_m0;
  struct reset_control *    rst_h_m0;
  struct reset_control *    rst_m0_jtag;
  struct reset_control *    rst_hresetn_m0_ac;

  void __iomem *            tcm_virt;
  phys_addr_t               tcm_phys;

  struct platform_device *  pdev;
//
} rk3506_mcu_t;

static int rk3506_rproc_start(struct rproc * rproc)
{
  rk3506_mcu_t *        mcu = rproc->priv;
  struct arm_smccc_res  res;

  dev_info(&rproc->dev, "Starting FW at 0x%08X...", (uint32_t)rproc->bootaddr);

#if 0
  arm_smccc_smc(SIP_MCU_CFG, ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
                ROCKCHIP_SIP_CONFIG_MCU_SRAM_START_ADDR,
                0xfff84000,
                0, 0, 0, 0, &res);
  if (res.a0)
  {
    dev_err(&rproc->dev, "SMCCC SRAM start call error: %i", (int)res.a0);
  }
#endif

  /* address map: map 0 to sram, enable TCM mode for sram
   * 0xfff84000 for sram
   * 0x03e00000 for ddr */
  arm_smccc_smc(SIP_MCU_CFG, ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
                ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
                rproc->bootaddr,
                0, 0, 0, 0, &res);
  if (res.a0)
  {
    dev_err(&rproc->dev, "SMCCC CODE START call error: %i", (int)res.a0);
  }

  reset_control_deassert(mcu->rst_m0_jtag);
  reset_control_deassert(mcu->rst_h_m0);
  reset_control_deassert(mcu->rst_hresetn_m0_ac);

  return 0;
}

static int rk3506_rproc_stop(struct rproc * rproc)
{
  rk3506_mcu_t *        mcu = rproc->priv;

  reset_control_assert(mcu->rst_m0_jtag);
  reset_control_assert(mcu->rst_h_m0);
  reset_control_assert(mcu->rst_hresetn_m0_ac);

  return 0;
}

#if 0
static int rk3506_rproc_load(struct rproc * rproc, const struct firmware * fw)
{
  rk3506_mcu_t *  mcu = rproc->priv;

  if (fw->size > RK3506_MCU_TCM_SIZE)
      return -EINVAL;

  dev_info(&rproc->dev, "Loading FW: virt_addr=0x%08X, size=%u", (uint32_t)mcu->tcm_virt, (uint32_t)fw->size);
  dev_info(&rproc->dev, "FW Entry: 0x%08X", (uint32_t)rproc->bootaddr);

  memcpy_toio(mcu->tcm_virt, fw->data, fw->size);
  return 0;
}
#endif

static void * my_da_to_va(struct rproc * rproc, u64 da, size_t len, bool * is_iomem)
{
  rk3506_mcu_t *  mcu = rproc->priv;
  void __iomem * va;

  // Only allow mapping of addresses starting from 0
  if (da + len >= 0x8000)
      return NULL;

  va = mcu->tcm_virt + da;  // mapped base + offset
  return va;
}

static const struct rproc_ops rk3506_rproc_ops =
{
  .start = rk3506_rproc_start,
  .stop = rk3506_rproc_stop,
  .da_to_va = my_da_to_va,
  //.load = rk3506_rproc_load,
	.load = rproc_elf_load_segments,
	// .parse_fw = rk3506_rproc_parse_fw,

	.find_loaded_rsc_table = rproc_elf_find_loaded_rsc_table,
	.sanity_check = rproc_elf_sanity_check,
	.get_boot_addr = rproc_elf_get_boot_addr,
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
  struct resource *   res;
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

  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  mcu->tcm_virt = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(mcu->tcm_virt))
  {
    rproc_free(rproc);
    return PTR_ERR(mcu->tcm_virt);
  }

  mcu->tcm_phys = res->start;
  mcu->pdev = pdev;

  // clocks

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

  platform_set_drvdata(pdev, rproc);

  ret = rproc_add(rproc);
  if (ret)
    return ret;

  clk_prepare_enable(mcu->hclk_m0);
  clk_prepare_enable(mcu->stclk_m0);

  reset_control_deassert(mcu->rst_m0_jtag);

  return ret;
}

static int rk3506_rproc_remove(struct platform_device * pdev)
{
  struct rproc *   rproc = platform_get_drvdata(pdev);
  rk3506_mcu_t *   mcu = rproc->priv;

  rproc_del(rproc);
  clk_disable_unprepare(mcu->stclk_m0);
	clk_disable_unprepare(mcu->hclk_m0);
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
  .remove = rk3506_rproc_remove,
  .driver = {
    .name = "rk3506_mcu_rproc",
    .of_match_table = rk3506_rproc_match,
  },
};

module_platform_driver(rk3506_rproc_driver);

MODULE_AUTHOR("nvitya");
MODULE_DESCRIPTION("RK3506 Cortex-M0 Remote Processor Driver");
MODULE_LICENSE("GPL v2");

