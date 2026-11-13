#include "xusbps.h"

XUsbPs_Config XUsbPs_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,zynq-usb-2.20a", /* compatible */
		0xe0002000, /* reg */
		0xffff, /* interrupts */
		0xffff /* interrupt-parent */
	},
	 {
		 NULL
	}
};