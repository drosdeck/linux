// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel LPSS PCI support.
 *
 * Copyright (C) 2015, Intel Corporation
 *
 * Authors: Andy Shevchenko <andriy.shevchenko@linux.intel.com>
 *          Mika Westerberg <mika.westerberg@linux.intel.com>
 */

#include <linux/device.h>
#include <linux/gfp_types.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>

#include <linux/pxa2xx_ssp.h>

#include <asm/errno.h>

#include "intel-lpss.h"

static const struct pci_device_id quirk_ids[] = {
	{
		/* Microsoft Surface Go (version 1) I2C4 */
		PCI_DEVICE_SUB(PCI_VENDOR_ID_INTEL, 0x9d64, 0x152d, 0x1182),
		.driver_data = QUIRK_IGNORE_RESOURCE_CONFLICTS,
	},
	{
		/* Microsoft Surface Go 2 I2C4 */
		PCI_DEVICE_SUB(PCI_VENDOR_ID_INTEL, 0x9d64, 0x152d, 0x1237),
		.driver_data = QUIRK_IGNORE_RESOURCE_CONFLICTS,
	},
	{
		/* Dell XPS 9530 (2023) */
		PCI_DEVICE_SUB(PCI_VENDOR_ID_INTEL, 0x51fb, 0x1028, 0x0beb),
		.driver_data = QUIRK_CLOCK_DIVIDER_UNITY,
	},
	{ }
};

static int intel_lpss_pci_probe(struct pci_dev *pdev,
				const struct pci_device_id *id)
{
	const struct intel_lpss_platform_info *data = (void *)id->driver_data;
	const struct pci_device_id *quirk_pci_info;
	struct intel_lpss_platform_info *info;
	int ret;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (ret < 0)
		return ret;

	info = devm_kmemdup(&pdev->dev, data, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	/* No need to check mem and irq here as intel_lpss_probe() does it for us */
	info->mem = pci_resource_n(pdev, 0);
	info->irq = pci_irq_vector(pdev, 0);

	quirk_pci_info = pci_match_id(quirk_ids, pdev);
	if (quirk_pci_info)
		info->quirks = quirk_pci_info->driver_data;

	pdev->d3cold_delay = 0;

	/* Probably it is enough to set this for iDMA capable devices only */
	pci_set_master(pdev);
	pci_try_set_mwi(pdev);

	ret = intel_lpss_probe(&pdev->dev, info);
	if (ret)
		return ret;

	pm_runtime_put(&pdev->dev);
	pm_runtime_allow(&pdev->dev);

	return 0;
}

static void intel_lpss_pci_remove(struct pci_dev *pdev)
{
	pm_runtime_forbid(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	intel_lpss_remove(&pdev->dev);
}

static const struct property_entry spt_spi_properties[] = {
	PROPERTY_ENTRY_U32("intel,spi-pxa2xx-type", LPSS_SPT_SSP),
	{ }
};

static const struct software_node spt_spi_node = {
	.properties = spt_spi_properties,
};

static const struct intel_lpss_platform_info spt_spi_info = {
	.clk_rate = 120000000,
	.swnode = &spt_spi_node,
};

static const struct property_entry spt_i2c_properties[] = {
	PROPERTY_ENTRY_U32("i2c-sda-hold-time-ns", 230),
	{ },
};

static const struct software_node spt_i2c_node = {
	.properties = spt_i2c_properties,
};

static const struct intel_lpss_platform_info spt_i2c_info = {
	.clk_rate = 120000000,
	.swnode = &spt_i2c_node,
};

static const struct property_entry uart_properties[] = {
	PROPERTY_ENTRY_U32("reg-io-width", 4),
	PROPERTY_ENTRY_U32("reg-shift", 2),
	PROPERTY_ENTRY_BOOL("snps,uart-16550-compatible"),
	{ },
};

static const struct software_node uart_node = {
	.properties = uart_properties,
};

static const struct intel_lpss_platform_info spt_uart_info = {
	.clk_rate = 120000000,
	.clk_con_id = "baudclk",
	.swnode = &uart_node,
};

static const struct property_entry bxt_spi_properties[] = {
	PROPERTY_ENTRY_U32("intel,spi-pxa2xx-type", LPSS_BXT_SSP),
	{ }
};

static const struct software_node bxt_spi_node = {
	.properties = bxt_spi_properties,
};

static const struct intel_lpss_platform_info bxt_spi_info = {
	.clk_rate = 100000000,
	.swnode = &bxt_spi_node,
};

static const struct intel_lpss_platform_info bxt_uart_info = {
	.clk_rate = 100000000,
	.clk_con_id = "baudclk",
	.swnode = &uart_node,
};

static const struct property_entry bxt_i2c_properties[] = {
	PROPERTY_ENTRY_U32("i2c-sda-hold-time-ns", 42),
	PROPERTY_ENTRY_U32("i2c-sda-falling-time-ns", 171),
	PROPERTY_ENTRY_U32("i2c-scl-falling-time-ns", 208),
	{ },
};

static const struct software_node bxt_i2c_node = {
	.properties = bxt_i2c_properties,
};

static const struct intel_lpss_platform_info bxt_i2c_info = {
	.clk_rate = 133000000,
	.swnode = &bxt_i2c_node,
};

static const struct property_entry apl_i2c_properties[] = {
	PROPERTY_ENTRY_U32("i2c-sda-hold-time-ns", 207),
	PROPERTY_ENTRY_U32("i2c-sda-falling-time-ns", 171),
	PROPERTY_ENTRY_U32("i2c-scl-falling-time-ns", 208),
	{ },
};

static const struct software_node apl_i2c_node = {
	.properties = apl_i2c_properties,
};

static const struct intel_lpss_platform_info apl_i2c_info = {
	.clk_rate = 133000000,
	.swnode = &apl_i2c_node,
};

static const struct property_entry glk_i2c_properties[] = {
	PROPERTY_ENTRY_U32("i2c-sda-hold-time-ns", 313),
	PROPERTY_ENTRY_U32("i2c-sda-falling-time-ns", 171),
	PROPERTY_ENTRY_U32("i2c-scl-falling-time-ns", 290),
	{ },
};

static const struct software_node glk_i2c_node = {
	.properties = glk_i2c_properties,
};

static const struct intel_lpss_platform_info glk_i2c_info = {
	.clk_rate = 133000000,
	.swnode = &glk_i2c_node,
};

static const struct property_entry cnl_spi_properties[] = {
	PROPERTY_ENTRY_U32("intel,spi-pxa2xx-type", LPSS_CNL_SSP),
	{ }
};

static const struct software_node cnl_spi_node = {
	.properties = cnl_spi_properties,
};

static const struct intel_lpss_platform_info cnl_spi_info = {
	.clk_rate = 120000000,
	.swnode = &cnl_spi_node,
};

static const struct intel_lpss_platform_info cnl_i2c_info = {
	.clk_rate = 216000000,
	.swnode = &spt_i2c_node,
};

static const struct intel_lpss_platform_info ehl_i2c_info = {
	.clk_rate = 100000000,
	.swnode = &bxt_i2c_node,
};

static const struct property_entry tgl_spi_properties[] = {
	PROPERTY_ENTRY_U32("intel,spi-pxa2xx-type", LPSS_CNL_SSP),
	{ }
};

static const struct software_node tgl_spi_node = {
	.properties = tgl_spi_properties,
};

static const struct intel_lpss_platform_info tgl_spi_info = {
	.clk_rate = 100000000,
	.swnode = &tgl_spi_node,
};

static const struct pci_device_id intel_lpss_pci_ids[] = {
	/* CML-LP */
	{ PCI_VDEVICE(INTEL, 0x02a8), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x02a9), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x02aa), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x02ab), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x02c5), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x02c6), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x02c7), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x02e8), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x02e9), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x02ea), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x02eb), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x02fb), (kernel_ulong_t)&cnl_spi_info },
	/* CML-H */
	{ PCI_VDEVICE(INTEL, 0x06a8), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x06a9), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x06aa), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x06ab), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x06c7), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x06e8), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x06e9), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x06ea), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x06eb), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x06fb), (kernel_ulong_t)&cnl_spi_info },
	/* BXT A-Step */
	{ PCI_VDEVICE(INTEL, 0x0aac), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0aae), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0ab0), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0ab2), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0ab4), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0ab6), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0ab8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0aba), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x0abc), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x0abe), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x0ac0), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x0ac2), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x0ac4), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x0ac6), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x0aee), (kernel_ulong_t)&bxt_uart_info },
	/* BXT B-Step */
	{ PCI_VDEVICE(INTEL, 0x1aac), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1aae), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1ab0), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1ab2), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1ab4), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1ab6), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1ab8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1aba), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x1abc), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x1abe), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x1ac0), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x1ac2), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x1ac4), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x1ac6), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x1aee), (kernel_ulong_t)&bxt_uart_info },
	/* EBG */
	{ PCI_VDEVICE(INTEL, 0x1bad), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x1bae), (kernel_ulong_t)&bxt_uart_info },
	/* GLK */
	{ PCI_VDEVICE(INTEL, 0x31ac), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31ae), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31b0), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31b2), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31b4), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31b6), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31b8), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31ba), (kernel_ulong_t)&glk_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x31bc), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x31be), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x31c0), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x31c2), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x31c4), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x31c6), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x31ee), (kernel_ulong_t)&bxt_uart_info },
	/* ICL-LP */
	{ PCI_VDEVICE(INTEL, 0x34a8), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x34a9), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x34aa), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x34ab), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x34c5), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x34c6), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x34c7), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x34e8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x34e9), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x34ea), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x34eb), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x34fb), (kernel_ulong_t)&cnl_spi_info },
	/* ICL-N */
	{ PCI_VDEVICE(INTEL, 0x38a8), (kernel_ulong_t)&spt_uart_info },
	/* TGL-H */
	{ PCI_VDEVICE(INTEL, 0x43a7), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x43a8), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x43a9), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x43aa), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x43ab), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x43ad), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x43ae), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x43d8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x43da), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x43e8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x43e9), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x43ea), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x43eb), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x43fb), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x43fd), (kernel_ulong_t)&tgl_spi_info },
	/* EHL */
	{ PCI_VDEVICE(INTEL, 0x4b28), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x4b29), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x4b2a), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x4b2b), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x4b37), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x4b44), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4b45), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4b4b), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4b4c), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4b4d), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x4b78), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4b79), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4b7a), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4b7b), (kernel_ulong_t)&ehl_i2c_info },
	/* JSL */
	{ PCI_VDEVICE(INTEL, 0x4da8), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x4da9), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x4daa), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x4dab), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x4dc5), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4dc6), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4dc7), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x4de8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4de9), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4dea), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4deb), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x4dfb), (kernel_ulong_t)&cnl_spi_info },
	/* ADL-P */
	{ PCI_VDEVICE(INTEL, 0x51a8), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x51a9), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x51aa), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x51ab), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x51c5), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51c6), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51c7), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x51d8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51d9), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51e8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51e9), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51ea), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51eb), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x51fb), (kernel_ulong_t)&tgl_spi_info },
	/* ADL-M */
	{ PCI_VDEVICE(INTEL, 0x54a8), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x54a9), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x54aa), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x54ab), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x54c5), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x54c6), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x54c7), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x54e8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x54e9), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x54ea), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x54eb), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x54fb), (kernel_ulong_t)&tgl_spi_info },
	/* APL */
	{ PCI_VDEVICE(INTEL, 0x5aac), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5aae), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5ab0), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5ab2), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5ab4), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5ab6), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5ab8), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5aba), (kernel_ulong_t)&apl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x5abc), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x5abe), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x5ac0), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x5ac2), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x5ac4), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x5ac6), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x5aee), (kernel_ulong_t)&bxt_uart_info },
	/* ARL-H */
	{ PCI_VDEVICE(INTEL, 0x7725), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7726), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7727), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7730), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7746), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7750), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7751), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7752), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7778), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7779), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x777a), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x777b), (kernel_ulong_t)&bxt_i2c_info },
	/* RPL-S */
	{ PCI_VDEVICE(INTEL, 0x7a28), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7a29), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7a2a), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7a2b), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7a4c), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7a4d), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7a4e), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7a4f), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7a5c), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7a79), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7a7b), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7a7c), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7a7d), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7a7e), (kernel_ulong_t)&bxt_uart_info },
	/* ADL-S */
	{ PCI_VDEVICE(INTEL, 0x7aa8), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7aa9), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7aaa), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7aab), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7acc), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7acd), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7ace), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7acf), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7adc), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7af9), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7afb), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7afc), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7afd), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7afe), (kernel_ulong_t)&bxt_uart_info },
	/* MTL-P */
	{ PCI_VDEVICE(INTEL, 0x7e25), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7e26), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7e27), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7e30), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7e46), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7e50), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7e51), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7e52), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7e78), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7e79), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7e7a), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7e7b), (kernel_ulong_t)&bxt_i2c_info },
	/* MTP-S */
	{ PCI_VDEVICE(INTEL, 0x7f28), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7f29), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7f2a), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7f2b), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7f4c), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7f4d), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7f4e), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7f4f), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7f5c), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7f5d), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x7f5e), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7f5f), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x7f7a), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x7f7b), (kernel_ulong_t)&bxt_i2c_info },
	/* LKF */
	{ PCI_VDEVICE(INTEL, 0x98a8), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x98a9), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x98aa), (kernel_ulong_t)&bxt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x98c5), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x98c6), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x98c7), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x98e8), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x98e9), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x98ea), (kernel_ulong_t)&bxt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x98eb), (kernel_ulong_t)&bxt_i2c_info },
	/* SPT-LP */
	{ PCI_VDEVICE(INTEL, 0x9d27), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x9d28), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x9d29), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x9d2a), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0x9d60), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9d61), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9d62), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9d63), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9d64), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9d65), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9d66), (kernel_ulong_t)&spt_uart_info },
	/* CNL-LP */
	{ PCI_VDEVICE(INTEL, 0x9da8), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x9da9), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x9daa), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x9dab), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0x9dc5), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9dc6), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9dc7), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0x9de8), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9de9), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9dea), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9deb), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0x9dfb), (kernel_ulong_t)&cnl_spi_info },
	/* TGL-LP */
	{ PCI_VDEVICE(INTEL, 0xa0a8), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa0a9), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa0aa), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa0ab), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa0c5), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0c6), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0c7), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa0d8), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0d9), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0da), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa0db), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa0dc), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa0dd), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa0de), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa0df), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa0e8), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0e9), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0ea), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0eb), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa0fb), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa0fd), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa0fe), (kernel_ulong_t)&cnl_spi_info },
	/* SPT-H */
	{ PCI_VDEVICE(INTEL, 0xa127), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa128), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa129), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa12a), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa160), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa161), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa162), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa166), (kernel_ulong_t)&spt_uart_info },
	/* KBL-H */
	{ PCI_VDEVICE(INTEL, 0xa2a7), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa2a8), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa2a9), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa2aa), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa2e0), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa2e1), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa2e2), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa2e3), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa2e6), (kernel_ulong_t)&spt_uart_info },
	/* CNL-H */
	{ PCI_VDEVICE(INTEL, 0xa328), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa329), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa32a), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa32b), (kernel_ulong_t)&cnl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa347), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa368), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa369), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa36a), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa36b), (kernel_ulong_t)&cnl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa37b), (kernel_ulong_t)&cnl_spi_info },
	/* CML-V */
	{ PCI_VDEVICE(INTEL, 0xa3a7), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa3a8), (kernel_ulong_t)&spt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa3a9), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa3aa), (kernel_ulong_t)&spt_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa3e0), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa3e1), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa3e2), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa3e3), (kernel_ulong_t)&spt_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa3e6), (kernel_ulong_t)&spt_uart_info },
	/* LNL-M */
	{ PCI_VDEVICE(INTEL, 0xa825), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa826), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa827), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa830), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa846), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xa850), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa851), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa852), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xa878), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa879), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa87a), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xa87b), (kernel_ulong_t)&ehl_i2c_info },
	/* PTL-H */
	{ PCI_VDEVICE(INTEL, 0xe325), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xe326), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xe327), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xe330), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xe346), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xe350), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe351), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe352), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xe378), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe379), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe37a), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe37b), (kernel_ulong_t)&ehl_i2c_info },
	/* PTL-P */
	{ PCI_VDEVICE(INTEL, 0xe425), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xe426), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xe427), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xe430), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xe446), (kernel_ulong_t)&tgl_spi_info },
	{ PCI_VDEVICE(INTEL, 0xe450), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe451), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe452), (kernel_ulong_t)&bxt_uart_info },
	{ PCI_VDEVICE(INTEL, 0xe478), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe479), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe47a), (kernel_ulong_t)&ehl_i2c_info },
	{ PCI_VDEVICE(INTEL, 0xe47b), (kernel_ulong_t)&ehl_i2c_info },
	{ }
};
MODULE_DEVICE_TABLE(pci, intel_lpss_pci_ids);

static struct pci_driver intel_lpss_pci_driver = {
	.name = "intel-lpss",
	.id_table = intel_lpss_pci_ids,
	.probe = intel_lpss_pci_probe,
	.remove = intel_lpss_pci_remove,
	.driver = {
		.pm = pm_ptr(&intel_lpss_pm_ops),
	},
};

module_pci_driver(intel_lpss_pci_driver);

MODULE_AUTHOR("Andy Shevchenko <andriy.shevchenko@linux.intel.com>");
MODULE_AUTHOR("Mika Westerberg <mika.westerberg@linux.intel.com>");
MODULE_DESCRIPTION("Intel LPSS PCI driver");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS("INTEL_LPSS");
