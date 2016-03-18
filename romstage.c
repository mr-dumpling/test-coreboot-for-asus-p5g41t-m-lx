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

static void mb_gpio_init(void)
{
	device_t dev;

	/* Southbridge GPIOs. */
	dev = PCI_DEV(0x0, 0x1f, 0x0);

	/* Set the value for GPIO base address register and enable GPIO. */
	//pci_write_config32(dev, GPIO_BASE, (DEFAULT_GPIOBASE | 1));
	//pci_write_config8(dev, GPIO_CNTL, 0x10);

	outl(0x1f3df7c1, DEFAULT_GPIOBASE + 0x00); /* GPIO_USE_SEL */
	outl(0xe0c86ec3, DEFAULT_GPIOBASE + 0x04); /* GP_IO_SEL */
	outl(0xe2eebe7f, DEFAULT_GPIOBASE + 0x0c); /* GP_LVL */
	outl(0x00002400, DEFAULT_GPIOBASE + 0x2c); /* GPI_INV */
	outl(0x000000ff, DEFAULT_GPIOBASE + 0x30); //... GPIO_USE_SEL2
	outl(0x000000f0, DEFAULT_GPIOBASE + 0x34); //... GP_IO_SEL2
	outl(0x000300f3, DEFAULT_GPIOBASE + 0x38); //... GP_LVL2

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
	RCBA8(0x31ff) = 0x03; //Where can I find these?
	RCBA8(0x31ff);
}

void main(unsigned long bist)
{

	/* Disable watchdog timer */
	//RCBA32(0x3410) = RCBA32(0x3410) | 0x20;

        post_code(0xA1);

	/* Set southbridge and Super I/O GPIOs. */
	mb_gpio_init();

        post_code(0xA2);
       
        //w83627dhg_set_clksel_48(SUPERIO_DEV);
	winbond_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	console_init();

        printk(BIOS_DEBUG, "Hello world 2!!!!\n");

        post_code(0xA3);

	report_bist_failure(bist);

        post_code(0xA4);

	enable_smbus();

        post_code(0xA5);

	x4x_early_init();

        post_code(0xA6);

	printk(BIOS_DEBUG, "Initializing memory\n");

	//                          ch0      ch1
	const u8 spd_addrmap[4] = { 0x50, 0, 0x52, 0 };

	sdram_initialize(0, spd_addrmap);
	quick_ram_check();
	cbmem_initialize_empty();
	printk(BIOS_DEBUG, "Memory initialized\n");
        post_code(0x99);
}
