#include "xuartps.h"

XUartPs_Config XUartPs_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,xuartps", /* compatible */
		0xe0000000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		0x0, /* xlnx,has-modem */
		0xffff, /* clocks */
		0xffff, /* interrupts */
		0xffff /* interrupt-parent */
	},
	 {
		 NULL
	}
};