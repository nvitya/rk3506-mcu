/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 */

#include "hal_bsp.h"
#include "hal_base.h"

/********************* Private MACRO Definition ******************************/
//#define TEST_DEMO

/********************* Private Structure Definition **************************/

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/

/********************* Public Function Definition ****************************/
static struct UART_REG *pUart = UART4;

#ifdef __GNUC__
__USED int _write(int fd, char *ptr, int len)
{
    int i = 0;

    /*
     * write "len" of char from "ptr" to file id "fd"
     * Return number of char written.
     *
    * Only work for STDOUT, STDIN, and STDERR
     */
    if (fd > 2) {
        return -1;
    }

    while (*ptr && (i < len)) {
        if (*ptr == '\n') {
            HAL_UART_SerialOutChar(pUart, '\r');
        }
        HAL_UART_SerialOutChar(pUart, *ptr);

        i++;
        ptr++;
    }

    return i;
}
#else
int fputc(int ch, FILE *f)
{
    if (ch == '\n') {
        HAL_UART_SerialOutChar(pUart, '\r');
    }

    HAL_UART_SerialOutChar(pUart, (char)ch);

    return 0;
}
#endif

int main(void)
{
    struct HAL_UART_CONFIG hal_uart_config = {
        .baudRate = UART_BR_1500000,
        .dataBit = UART_DATA_8B,
        .stopBit = UART_ONE_STOPBIT,
        .parity = UART_PARITY_DISABLE,
    };

    /* HAL BASE Init */
    HAL_Init();

    /* BSP Init */
    BSP_Init();

    /* INTMUX Init */
    HAL_INTMUX_Init();

    /* UART Init */
    HAL_PINCTRL_SetRMIO(GPIO_BANK1,
                        GPIO_PIN_C2,
                        RMIO_UART4_TX);
    HAL_PINCTRL_SetRMIO(GPIO_BANK1,
                        GPIO_PIN_C3,
                        RMIO_UART4_RX);
    HAL_UART_Init(&g_uart4Dev, &hal_uart_config);

    HAL_DBG("Hello RK3506 mcu\r\n");

    uint32_t hbcnt = 0;
    while (1)
    {
      HAL_DBG("MCU hbcnt = %d\r\n", hbcnt);
      HAL_DelayUs(500000);
      ++hbcnt;
    }


#ifdef TEST_DEMO
    //test_demo();
#endif

    while (1) {
        __asm volatile ("wfi");
    }
}

int entry(void)
{
    return main();
}
