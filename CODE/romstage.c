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

#define SERIAL1_DEV PNP_DEV(0x2e, W83627DHG_SP1) //Serial port 1
#define SERIAL2_DEV PNP_DEV(0x2e, W83627DHG_SP2) //Serial port 2
#define GPIO6_DEV PNP_DEV(0x2e, W83627DHG_GPIO6_V) //GPIO6
#define GPIO2345_DEV PNP_DEV(0x2e, W83627DHG_GPIO2345_V) //GPIO2,3,4 and 5
#define GPIO_DEV PNP_DEV(0x2e, W83627DHG_GPIO)
#define EC_DEV PNP_DEV(0x2e, W83627DHG_EC)
#define SUPERIO_DEV PNP_DEV(0x2e, 0)

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
	outl(0x000000ff, DEFAULT_GPIOBASE + 0x30); /* GPIO_USE_SEL2 */
	outl(0x000000f0, DEFAULT_GPIOBASE + 0x34); /* GP_IO_SEL2 */
	outl(0x000300f3, DEFAULT_GPIOBASE + 0x38); /* GP_LVL2 */

	/* Device 1f interrupt pin register */
        RCBA32(0x3100) = 0x00042210;
        RCBA32(0x3108) = 0x10004321;

        RCBA32(0x3104) = 0x00002100;

	/* PCIe Interrupts */
        RCBA32(0x310c) = 0x00214321;
	/* HD Audio Interrupt */
        RCBA32(0x3110) = 0x00000001;

        RCBA32(0x3140) = 0x32410162;
        RCBA32(0x3144) = 0x32100237;
        //RCBA32(0x3148) = 0x00003215;  //Should I set these as well?
        //RCBA32(0x31fc) = 0x03000000;

	/* dev irq route register */
	RCBA16(0x3140) = 0x0162;
	RCBA16(0x3144) = 0x0237;
	RCBA16(0x3148) = 0x3215;

	/* Enable IOAPIC */
	RCBA8(0x31ff) = 0x03; //Where can I find these?
	//RCBA8(0x31ff);
}

static void sio_init( void )
{
	pnp_enter_ext_func_mode(GPIO2345_DEV);
	pnp_set_logical_device(GPIO2345_DEV);

        
        pnp_write_config(GPIO2345_DEV, 0x30, 0x05);
        pnp_write_config(GPIO2345_DEV, 0xe0, 0xff); //default
        pnp_write_config(GPIO2345_DEV, 0xe1, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xe2, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xe3, 0xff); //default
        pnp_write_config(GPIO2345_DEV, 0xe4, 0x04);
        pnp_write_config(GPIO2345_DEV, 0xe5, 0x00); //default
        pnp_write_config(GPIO2345_DEV, 0xe6, 0x00); //default
        pnp_write_config(GPIO2345_DEV, 0xe7, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xe8, 0xdf);
        pnp_write_config(GPIO2345_DEV, 0xe9, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xf0, 0xff); //default
        pnp_write_config(GPIO2345_DEV, 0xf1, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xf2, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xf3, 0x09);
        pnp_write_config(GPIO2345_DEV, 0xf4, 0xa4);
        pnp_write_config(GPIO2345_DEV, 0xf5, 0xce);
        pnp_write_config(GPIO2345_DEV, 0xf6, 0x00); //default
        pnp_write_config(GPIO2345_DEV, 0xf7, 0x00); //default
        pnp_write_config(GPIO2345_DEV, 0xf8, 0x00); //default
        pnp_write_config(GPIO2345_DEV, 0xf9, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xfa, 0xff);
        pnp_write_config(GPIO2345_DEV, 0xfe, 0xff);

        pnp_write_config(GPIO6_DEV, 0x30, 0x06);
        pnp_write_config(GPIO6_DEV, 0xf4, 0xff); //default
        pnp_write_config(GPIO6_DEV, 0xf5, 0xff);
        pnp_write_config(GPIO6_DEV, 0xf6, 0xff);
        pnp_write_config(GPIO6_DEV, 0xf7, 0xff);
        pnp_write_config(GPIO6_DEV, 0xf8, 0xff);
        pnp_exit_ext_func_mode(GPIO2345_DEV);
}

static void ich7_enable_lpc(void)
{

	// Enable Serial IRQ
	//pci_write_config8(PCI_DEV(0, 0x1f, 0), 0x64, 0xd0);

	/* Disable Serial IRQ */
	pci_write_config8(PCI_DEV(0, 0x1f, 0), 0x64, 0x00);
	/* Decode range */
	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x80,
		pci_read_config16(PCI_DEV(0, 0x1f, 0), 0x80) | 0x0010);
	pci_write_config16(PCI_DEV(0, 0x1f, 0), LPC_EN,
		CNF1_LPC_EN | CNF2_LPC_EN | KBC_LPC_EN | COMA_LPC_EN);


	// Enable COM1
	//pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x82, 0x140d); //needed?

	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x88, 0x0291); //??
	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x8a, 0x007c); //??

}

void main(unsigned long bist)
{
	//                          ch0      ch1
	const u8 spd_addrmap[4] = { 0x50, 0, 0x52, 0 };
        //const u8 spd_addrmap[4] = { 0x50, 0x51, 0, 0 };

	/* Disable watchdog timer */
	RCBA32(0x3410) = RCBA32(0x3410) | 0x20;

        post_code(0xA1);
	mb_gpio_init();

        post_code(0xA2);
        ich7_enable_lpc();  

        post_code(0xB1);
        sio_init();

        post_code(0xB2);
        //w83627dhg_set_clksel_48(SUPERIO_DEV);
	winbond_enable_serial(SERIAL1_DEV, CONFIG_TTYS0_BASE);

        post_code(0xB3);
	console_init();

        post_code(0xA3);
	report_bist_failure(bist);

        post_code(0xA4);
	enable_smbus();

        post_code(0xA5);
	x4x_early_init();

	printk(BIOS_DEBUG, "Initializing memory\n");

        post_code(0xE0);
	sdram_initialize(0, spd_addrmap);
        post_code(0xE1);
	quick_ram_check();
        post_code(0xE2);
	cbmem_initialize_empty();

	printk(BIOS_DEBUG, "Memory initialized\n");
        post_code(0x99);
}
