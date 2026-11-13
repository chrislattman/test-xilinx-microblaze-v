#include "xgpiops.h"

XGpioPs_Config XGpioPs_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,zynq-gpio-1.0", /* compatible */
		0xe000a000, /* reg */
		0xffff, /* interrupts */
		0xffff /* interrupt-parent */
	},
	 {
		 NULL
	}
};