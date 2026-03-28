/**
 * @file   pressure_detect.c
 * @brief  Pressure-detection base component initialisation.
 *
 * The original monolithic pressure_detect_base_init() has been refactored
 * into four focused helpers:
 *
 *   pressure_detect_spi_init()    – SPI peripheral setup
 *   pressure_detect_uart_init()   – UART peripheral setup
 *   pressure_detect_filter_init() – Kalman / moving-average / low-pass init
 *   pressure_detect_prefill()     – Filter warm-up / convergence loop
 *
 * pressure_detect_base_init() is now a thin orchestrator that calls the
 * helpers in the correct order.
 */

#include "pressure_detect.h"

/* BSP / chip-support headers (provided by the target SDK) */
#include "chip.h"
#include "iocon_17xx_40xx.h"
#include "ssp_17xx_40xx.h"
#include "uart_17xx_40xx.h"

/* RTOS delay */
#include "tx_api.h"

/* -----------------------------------------------------------------------
 * Private helpers
 * ----------------------------------------------------------------------- */

/**
 * @brief  Configure and enable the SPI (SSP0) peripheral.
 *
 * Pin-mux is set for CLK, CS, and MISO lines.  The peripheral is left
 * disabled here; it must be enabled immediately before a transfer begins.
 */
void pressure_detect_spi_init(void)
{
    /* Pin-mux: CLK, CS, MISO → SSP0 alternate function */
    Chip_IOCON_PinMuxSet(LPC_IOCON,
                         PRESSURE_DETECT_CLK_PORT,
                         PRESSURE_DETECT_PIN_IOCON,
                         IOCON_FUNC2 | IOCON_MODE_PULLUP);

    Chip_IOCON_PinMuxSet(LPC_IOCON,
                         PRESSURE_DETECT_CS_PORT,
                         PRESSURE_DETECT_CS_PIN_IOCON,
                         IOCON_FUNC2 | IOCON_MODE_PULLUP);

    Chip_IOCON_PinMuxSet(LPC_IOCON,
                         PRESSURE_DETECT_MISO_PORT,
                         PRESSURE_DETECT_MISO_PIN_IOCON,
                         IOCON_FUNC2 | IOCON_MODE_PULLUP);

    /* Enable peripheral clock */
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SSP0);

    /* Master mode */
    Chip_SSP_Set_Mode(LPC_SSP0, SSP_MODE_MASTER);

    /* SPI frame: 8-bit, CPHA=1 CPOL=1 */
    Chip_SSP_SetFormat(LPC_SSP0,
                       SSP_BITS_8,
                       SSP_FRAMEFORMAT_SPI,
                       SSP_CLOCK_CPHA1_CPOL1);

    /* Baud rate */
    Chip_SSP_SetBitRate(LPC_SSP0, PRESSURE_DETECT_SPI_BAUD_RATE);

    /* Enable the interrupt (priority already set in user_hw_sysinit) */
    NVIC_EnableIRQ(SSP0_IRQn);

    /* NOTE: The peripheral itself is intentionally NOT enabled here.
     *       Enable it immediately before each communication session. */
}

/**
 * @brief  Configure and enable the UART0 peripheral.
 *
 * Sets baud rate, 8-N-1 framing, FIFO, and enables the transmitter.
 */
void pressure_detect_uart_init(void)
{
    /* Pin-mux: TX and RX → UART0 alternate function */
    Chip_IOCON_PinMuxSet(LPC_IOCON,
                         UART0_TX_PORT,
                         UART0_TX_PIN_IOCON,
                         IOCON_FUNC1 | IOCON_MODE_INACT);

    Chip_IOCON_PinMuxSet(LPC_IOCON,
                         UART0_RX_PORT,
                         UART0_RX_PIN_IOCON,
                         IOCON_FUNC1 | IOCON_MODE_INACT);

    Chip_UART_Init(LPC_UART0);

    /* Baud rate */
    Chip_UART_SetBaud(LPC_UART0, PRESSURE_DETECT_UART_BAUD_RATE);

    /* 8 data bits, 1 stop bit */
    Chip_UART_ConfigData(LPC_UART0, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT);

    /* Enable FIFO, trigger level 2 */
    Chip_UART_SetupFIFOS(LPC_UART0,
                         UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2);

    /* Enable transmitter */
    Chip_UART_TXEnable(LPC_UART0);
}

/**
 * @brief  Initialise all signal filters in @p pressure.
 * @param  pressure  Pointer to the pressure-detection control structure.
 */
void pressure_detect_filter_init(pressure_detect_t *pressure)
{
    /* Kalman filter: process noise Q=0.1, measurement noise R=1e-5 */
    kalman_filter_init(&pressure->kf, 0.1f, 1e-5f);

    /* Moving-average filter with the embedded buffer */
    average_moving_filter_init(&pressure->af,
                               pressure->af_buffer,
                               AF_WINDOW_SIZE);

    /* First-order low-pass filter: fs=1000 Hz, fc=100 Hz */
    low_pass_filter_init(&pressure->lpf, 1000U, 100U);
}

/**
 * @brief  Warm up / pre-fill the filters so they converge before use.
 *
 * A single seed acquisition is performed first, then both the Kalman and
 * moving-average filters are reset to the measured value.  The loop that
 * follows repeats this PRESSURE_DETECT_PREFILL_COUNT times so that the
 * filter state fully reflects the resting pressure.
 *
 * @param  pressure  Pointer to the pressure-detection control structure.
 */
void pressure_detect_prefill(pressure_detect_t *pressure)
{
    /* Seed: obtain the very first reading */
    pressure_detect_trigger(pressure);
    tx_thread_sleep(2);
    kalman_filter_reset(&pressure->kf, pressure->origin_value);
    average_moving_filter_reset(&pressure->af, pressure->origin_value);

    /* Convergence loop */
    for (uint8_t i = 0; i < PRESSURE_DETECT_PREFILL_COUNT; i++)
    {
        pressure_detect_trigger(pressure);
        tx_thread_sleep(2);
        kalman_filter_reset(&pressure->kf, pressure->origin_value);
        average_moving_filter_reset(&pressure->af, pressure->origin_value);
    }
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialise all pressure-detection base components.
 *
 * Orchestrates SPI init, UART init, filter init, and filter pre-fill so
 * that the subsystem is ready for normal operation on return.
 *
 * @param  pressure  Pointer to the pressure-detection control structure.
 * @return 0 on success.
 */
int32_t pressure_detect_base_init(pressure_detect_t *pressure)
{
    pressure_detect_spi_init();
    pressure_detect_uart_init();
    pressure_detect_filter_init(pressure);
    pressure_detect_prefill(pressure);

    return 0;
}
