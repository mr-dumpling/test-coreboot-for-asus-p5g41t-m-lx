/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Damien Zammit <damien@zamaudio.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <stdlib.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <console/console.h>
#include <southbridge/intel/i82801gx/i82801gx.h>
#include <northbridge/intel/x4x/x4x.h>
#include <superio/winbond/common/winbond.h>
#include <superio/winbond/w83627dhg/w83627dhg.h>
#include <cpu/x86/bist.h>
#include <lib.h>
#include <cpu/intel/romstage.h>
#include <arch/stages.h>
#include <cbmem.h>

#define SERIAL_DEV PNP_DEV(0x2e, W83627DHG_SP1)
#define GPIO_DEV PNP_DEV(0x2e, W83627DHG_GPIO)
#define EC_DEV PNP_DEV(0x2e, W83627DHG_EC)
#define SUPERIO_DEV PNP_DEV(0x2e, 0)

/* Early mainboard specific GPIO setup.
 * We should use standard gpio.h eventually
 */

/*static void pnp_enter_ext_func_mode(device_t dev)
{
	u16 port = dev >> 8;
	outb(0x87, port);
	outb(0x87, port);
}

static void pnp_exit_ext_func_mode(device_t dev)
{
	u16 port = dev >> 8;
	outb(0xaa, port);
}*/

//static void superio_gpio_config(void)
//{
//	device_t dev = PNP_DEV(0x2e, 0x9);
//	pnp_enter_ext_func_mode(dev);
//	pnp_write_config(dev, 0x29, 0x02); /* Pins 119, 120 are GPIO21, 20 */
//	pnp_write_config(dev, 0x30, 0x03); /* Enable GPIO2+3 */
//	pnp_write_config(dev, 0x2a, 0x01); /* Pins 62, 63, 65, 66 are
//					      GPIO27, 26, 25, 24 */
//	pnp_write_config(dev, 0x2c, 0xc3); /* Pin 90 is GPIO32,
//					      Pins 78~85 are UART B */
//	pnp_write_config(dev, 0x2d, 0x00); /* Pins 67, 68, 70~73, 75, 77 are
//					      GPIO57~50 */
//	pnp_set_logical_device(dev);
//	/* Values can only be changed, when devices are enabled. */
//	//pnp_write_config(dev, 0xe3, 0xdd); /* GPIO2 bits 1, 5 are output */
//	//pnp_write_config(dev, 0xe4, (dis_bl_inv << 5) | (lvds_3v << 1)); /* GPIO2 bits 1, 5 */
//	pnp_exit_ext_func_mode(dev);
//}

static void mb_gpio_init(void)
{
	device_t dev;

	/* Southbridge GPIOs. */
	dev = PCI_DEV(0x0, 0x1f, 0x0);

	/* Set the value for GPIO base address register and enable GPIO. */
	pci_write_config32(dev, GPIO_BASE, (DEFAULT_GPIOBASE | 1));
	pci_write_config8(dev, GPIO_CNTL, 0x10);

	outl(0x1f3df7c1, DEFAULT_GPIOBASE + 0x00); /* GPIO_USE_SEL */
	outl(0xe0c86ec3, DEFAULT_GPIOBASE + 0x04); /* GP_IO_SEL */
	outl(0xe2eebe7f, DEFAULT_GPIOBASE + 0x0c); /* GP_LVL */
	outl(0x00002400, DEFAULT_GPIOBASE + 0x2c); /* GPI_INV */
	outl(0x000000ff, DEFAULT_GPIOBASE + 0x30); //... GPIO_USE_SEL2
	outl(0x000000f0, DEFAULT_GPIOBASE + 0x34); //... GP_IO_SEL2
	outl(0x000300f3, DEFAULT_GPIOBASE + 0x38); //... GP_LVL2

	/* Set default GPIOs on superio */
        //superio_gpio_config();

	/* IRQ routing */
        RCBA32(0x3100) = 0x00042210;
        RCBA32(0x3104) = 0x00002100;
        RCBA32(0x3108) = 0x10004321;
        RCBA32(0x310c) = 0x00214321;
        RCBA32(0x3110) = 0x00000001;
        RCBA32(0x3140) = 0x32410162;
        RCBA32(0x3144) = 0x32100237;
        //RCBA32(0x3148) = 0x00003215;  //Should I set these as well?
        //RCBA32(0x31fc) = 0x03000000;


	/* Enable IOAPIC */
	/*RCBA8(0x31ff) = 0x03;
	RCBA8(0x31ff);*/
}

//static void ich7_enable_lpc(void)
//{
//	/* Disable Serial IRQ */
//	pci_write_config8(PCI_DEV(0, 0x1f, 0), 0x64, 0x00);
//	/* Decode range */
//	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x80, 0x0010);
//	pci_write_config16(PCI_DEV(0, 0x1f, 0), LPC_EN,
//		CNF1_LPC_EN | CNF2_LPC_EN | KBC_LPC_EN | COMA_LPC_EN | COMB_LPC_EN);
//
//	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x88, 0x0291);
//	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x8a, 0x007c);
//}

void main(unsigned long bist)
{
	//                          ch0      ch1
	const u8 spd_addrmap[4] = { 0x50, 0, 0x52, 0 };

	/* Disable watchdog timer */
	RCBA32(0x3410) = RCBA32(0x3410) | 0x20;

        post_code(0xA1);

	/* Set southbridge and Super I/O GPIOs. */
	mb_gpio_init();

        post_code(0xA2);
       
        w83627dhg_set_clksel_48(SUPERIO_DEV);
	winbond_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);

        //printk(BIOS_DEBUG, "Hello world 1!!!!\n");

        //post_code(0xA4);

	console_init();

        printk(BIOS_DEBUG, "Hello world 2!!!!\n");

        post_code(0xA3);

	report_bist_failure(bist);

        post_code(0xA4);

	enable_smbus();

        post_code(0xA5);

	x4x_early_init();

        post_code(0x42);

	printk(BIOS_DEBUG, "Initializing memory\n");
	sdram_initialize(0, spd_addrmap);
	quick_ram_check();
	cbmem_initialize_empty();
	printk(BIOS_DEBUG, "Memory initialized\n");
}
