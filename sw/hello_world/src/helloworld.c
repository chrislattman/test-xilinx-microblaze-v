/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <sleep.h>
#include <stdio.h>
#include <xgpio.h>
#include <xparameters.h>
#include <xstatus.h>
#include <xwdttb.h>

#include "platform.h"
#include "xil_printf.h"

int main()
{
    XGpio leds;
    XWdtTb_Config *cfg_ptr;
    XWdtTb wdt;
    int status;
    
    init_platform();
    XGpio_Initialize(&leds, XPAR_XGPIO_0_BASEADDR);
    XGpio_SetDataDirection(&leds, 1, 0);
    cfg_ptr = XWdtTb_LookupConfig(XPAR_XWDTTB_0_BASEADDR);
    if (!cfg_ptr) {
        return XST_FAILURE;
    }
    status = XWdtTb_CfgInitialize(&wdt, cfg_ptr, cfg_ptr->BaseAddr);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }
    XWdtTb_Start(&wdt);

    while (1) {
        XGpio_DiscreteWrite(&leds, 1, 1);
        sleep(1);
        print("Hello\r\n");
        XGpio_DiscreteWrite(&leds, 1, 0);
        sleep(1);
        print("World!\r\n");
        XWdtTb_RestartWdt(&wdt);
    }

    cleanup_platform();
    return 0;
}
