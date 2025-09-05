/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 */

#include "hal_bsp.h"
#include "hal_base.h"
#include <stdlib.h>

/********************* Private MACRO Definition ******************************/
//#define IRQ_LATENCY_TEST
//#define PERF_TEST
//#define SOFTIRQ_TEST
//#define TIMER_TEST
//#define SYSTICK_TEST

//#define HAS_CONSOLE
#define RSVD0_MCU_IRQn 28

#ifdef HAS_CONSOLE
#define CONSOLE_BUF_MAX  100
#define CONSOLE_LINE_MAX 10

static void command_testall_process(uint8_t *in, int len);
struct console_command command_testall = {
    .name = "testall",
    .help = "Run all auto test demos",
    .process = command_testall_process,
};
#endif

/********************* Private Structure Definition **************************/

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/

/************************************************/
/*                                              */
/*                 HW Borad config              */
/*                                              */
/************************************************/

/************************************************/
/*                                              */
/*             IRQ_LATENCY_TEST                 */
/*                                              */
/************************************************/
#ifdef IRQ_LATENCY_TEST
#define IRQ_LATENCY_TEST_NUM   10
#define IRQ_LATENCY_TEST_LOOP  10000
#define IRQ_LATENCY_TEST_DELAY 500

static uint64_t time_start, time_end;
static double time_one, time_sum, time_max, time_min;

static void irq_rsvd_isr(void)
{
    time_end = HAL_GetSysTimerCount();
    time_one = ((time_end - time_start) * 1000000.0) / PLL_INPUT_OSC_RATE;
    time_sum += time_one;
    if (time_one > time_max) {
        time_max = time_one;
    }
    if (time_one < time_min) {
        time_min = time_one;
    }
//    HAL_DBG("irq_rsvd: latency = %.2f\n", time_one);
}

static void irq_latency_test(void)
{
    int i, j;

    HAL_DBG("irq_latency_test start:\n");

    HAL_DBG("irq_rsvd:\n");
    HAL_NVIC_SetIRQHandler(RSVD0_MCU_IRQn, irq_rsvd_isr);
    HAL_NVIC_EnableIRQ(RSVD0_MCU_IRQn);
    HAL_DelayMs(2000);

    for (i = 0; i < IRQ_LATENCY_TEST_NUM; i++) {
        time_sum = 0;
        time_max = 0;
        time_min = 1000;
        for (j = 0; j < IRQ_LATENCY_TEST_LOOP; j++) {
            time_start = HAL_GetSysTimerCount();
            HAL_NVIC_SetPendingIRQ(RSVD0_MCU_IRQn);
            HAL_DelayUs(IRQ_LATENCY_TEST_DELAY);
        }
        HAL_DBG("irq_rsvd latency: avg = %.2f, max = %.2f, min = %.2f\n",
                time_sum / IRQ_LATENCY_TEST_LOOP, time_max, time_min);
    }

    HAL_DBG("irq_latency_test end.\n");
}
#endif

/************************************************/
/*                                              */
/*                  PERF_TEST                   */
/*                                              */
/************************************************/
#ifdef PERF_TEST
#include "benchmark.h"

uint32_t g_sum = 0;

static void perf_test(void)
{
    uint32_t loop = 10000, size = 10 * 1024;
    uint32_t *ptr;
    uint32_t time_start, time_end, time_ms;

    HAL_DBG("Perftest Start:\n");
    benchmark_main();

    HAL_DBG("memset test start:\n");
    ptr = (uint32_t *)malloc(size);
    if (ptr) {
        time_start = HAL_GetTick();
        for (int i = 0; i < loop; i++) {
            memset(ptr, i, size);
        }
        time_end = HAL_GetTick();
        time_ms = time_end - time_start;
        HAL_DBG("memset bw=%" PRId32 "KB/s, time_ms=%d\n",
                1000 * (size * loop / 1024) / time_ms, time_ms);

        /* prevent optimization */
        for (int i = 0; i < size / sizeof(uint32_t); i++) {
            g_sum += ptr[i];
        }
        HAL_DBG("sum=%d\n", g_sum);
        free(ptr);
    }
    HAL_DBG("memset test end\n");

    HAL_DBG("Perftest End:\n");
}
#endif

/************************************************/
/*                                              */
/*                SOFTIRQ_TEST                  */
/*                                              */
/************************************************/
#ifdef SOFTIRQ_TEST
static void soft_isr(void)
{
    HAL_DBG("softirq_test: enter isr\n");
}

static void softirq_test(void)
{
    HAL_DBG("softirq_test start\n");
    HAL_NVIC_SetIRQHandler(RSVD0_MCU_IRQn, soft_isr);
    HAL_NVIC_EnableIRQ(RSVD0_MCU_IRQn);

    HAL_NVIC_SetPendingIRQ(RSVD0_MCU_IRQn);
}
#endif

/************************************************/
/*                                              */
/*                  TIMER_TEST                  */
/*                                              */
/************************************************/
#ifdef TIMER_TEST
static int timer_int_count = 0;
static uint32_t latency_sum = 0;
static uint32_t latency_max = 0;
static uint32_t overhead = 0;
struct TIMER_REG *test_timer = TIMER4; // TIMER0 direct to NVIC, TIMER4 to INTMUX
static uint32_t timer_irq = TIMER4_IRQn;
static bool desc_timer = true;

#if 0
static void timer_isr(uint32_t irq, void *args)
{
    uint32_t count;
    uint32_t latency;

    count = (uint32_t)HAL_TIMER_GetCount(test_timer);
    if (desc_timer) {
        count = 24000000 - count;
    }
    if (count > overhead) {
        count -= overhead;
    }
    /* 24M timer: 41.67ns per count */
    latency = count * 41;
    HAL_DBG("timer_test: latency=%dns(count=%d)\n", latency, count);
    timer_int_count++;
    latency_sum += latency;
    latency_max = latency_max > latency ? latency_max : latency;
    if (timer_int_count == 100) {
        HAL_DBG("timer_test: latency avg=%dns,max=%dns\n", latency_sum / timer_int_count, latency_max);
        timer_int_count = 0;
        latency_sum = 0;
        latency_max = 0;
        HAL_TIMER_ClrInt(test_timer);
        HAL_TIMER_Stop_IT(test_timer);
    }

    HAL_TIMER_ClrInt(test_timer);
}
#else
static uint64_t timer_start = 0;
static uint64_t timer_end = 0;

static void timer_isr(uint32_t irq, void *args)
{
    uint32_t count;
    uint32_t latency;

    if (++timer_int_count == 100) {
        timer_end = HAL_GetSysTimerCount();
        count = timer_end - timer_start - 2400000000;
        /* 24M timer: 41.67ns per count */
        latency = (count / 100) * 41;
        HAL_DBG("timer_test: latency=%dns(count=%d, start=%lld, end=%lld)\n",
                latency, count / 100, timer_start, timer_end);
        timer_int_count = 0;
        HAL_TIMER_ClrInt(test_timer);
        HAL_TIMER_Stop_IT(test_timer);

        return;
    }

    HAL_TIMER_ClrInt(test_timer);
}
#endif

static void timer_test(void)
{
    uint64_t start, end;
    uint32_t count;

    HAL_DBG("timer_test start\n");
    start = HAL_GetSysTimerCount();
    HAL_DelayUs(1000000);
    end = HAL_GetSysTimerCount();
    count = end > start ? (uint32_t)(end - start) : (uint32_t)(start - end);
    HAL_DBG("sys_timer 1s count: %d(%lld, %lld)\n", count, start, end);

    HAL_TIMER_Init(test_timer, TIMER_FREE_RUNNING);
    HAL_TIMER_SetCount(test_timer, -1);
    HAL_TIMER_Start(test_timer);
    start = HAL_TIMER_GetCount(test_timer);
    HAL_DelayUs(1000000);
    end = HAL_TIMER_GetCount(test_timer);
    desc_timer = start > end;
    count = desc_timer ? (uint32_t)(start - end) : (uint32_t)(end - start);
    HAL_DBG("test_timer 1s count: %d(%lld, %lld)\n", count, start, end);
    start = HAL_TIMER_GetCount(test_timer);
    end = HAL_TIMER_GetCount(test_timer);
    overhead = desc_timer ? (uint32_t)(start - end) : (uint32_t)(end - start);
    HAL_DBG("test_timer overhead=%d\n", overhead);
    HAL_TIMER_Stop(test_timer);

    if (test_timer == TIMER0) {
        HAL_INTMUX_SetIRQHandler(timer_irq, timer_isr, NULL);
        HAL_INTMUX_EnableIRQ(timer_irq);
    } else if (test_timer == TIMER4) {
        HAL_NVIC_SetIRQHandler(timer_irq, timer_isr);
        HAL_NVIC_EnableIRQ(timer_irq);
    } else {
        HAL_DBG("timer_test: unsupport timer\n");

        return;
    }
    HAL_DBG("timer_test: 1s interrupt start: %d\n", timer_irq);
    HAL_TIMER_Init(test_timer, TIMER_FREE_RUNNING);
    HAL_TIMER_SetCount(test_timer, 24000000);
    HAL_TIMER_Start_IT(test_timer);
    timer_start = HAL_GetSysTimerCount();
}
#endif

#ifdef SYSTICK_TEST
static uint64_t last_tick_time = 0;
static void systick_isr(uint32_t irq, void *args)
{
    uint64_t curr;

    curr = HAL_GetSysTimerCount();
    if (last_tick_time != 0) {
        uint32_t interval = (uint32_t)((curr - last_tick_time) * 41.67);
        HAL_DBG("systick_isr: interval=%dns\n", interval);
    }
    HAL_SYSTICK_IRQHandler();
    last_tick_time = curr;
}

static void systick_test(void)
{
    HAL_NVIC_SetIRQHandler(SysTick_IRQn, systick_isr);
    HAL_NVIC_EnableIRQ(SysTick_IRQn);
    HAL_SetTickFreq(1);
    HAL_SYSTICK_Init();
}
#endif

#ifndef HAS_CONSOLE
void test_demo(void)
#else
static void command_testall_process(uint8_t *in, int len)
#endif
{
#ifdef IRQ_LATENCY_TEST
    irq_latency_test();
#endif

#ifdef PERF_TEST
    perf_test();
#endif

#ifdef SOFTIRQ_TEST
    softirq_test();
#endif

#ifdef TIMER_TEST
    timer_test();
#endif

#ifdef SYSTICK_TEST
    systick_test();
#endif
}

#ifdef HAS_CONSOLE
void test_demo(void)
{
    int i;
    uint8_t *lines[CONSOLE_LINE_MAX];

    for (i = 0; i < CONSOLE_LINE_MAX; i++) {
        lines[i] = malloc(CONSOLE_BUF_MAX);
        memset(lines[i], 0, CONSOLE_BUF_MAX);
    }

    console_init(&g_uart7Dev, lines, CONSOLE_BUF_MAX, CONSOLE_LINE_MAX);
    console_add_command(&command_testall);
    console_add_command(&command_gpio);
    console_add_command(&command_wdt);
    console_add_command(&command_clk);

    HAL_INTMUX_SetIRQHandler(g_uart4Dev.irqNum, console_uart_isr, NULL);
    HAL_INTMUX_EnableIRQ(g_uart4Dev.irqNum);
    HAL_UART_EnableIrq(g_uart4Dev.pReg, 1);
    console_run(true);

    command_testall_process(NULL, 0);
}
#endif
