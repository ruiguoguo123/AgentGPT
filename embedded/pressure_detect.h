#ifndef PRESSURE_DETECT_H
#define PRESSURE_DETECT_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Hardware pin / peripheral configuration
 * ----------------------------------------------------------------------- */
#define PRESSURE_DETECT_CLK_PORT        0
#define PRESSURE_DETECT_PIN_IOCON       15
#define PRESSURE_DETECT_CS_PORT         0
#define PRESSURE_DETECT_CS_PIN_IOCON    16
#define PRESSURE_DETECT_MISO_PORT       0
#define PRESSURE_DETECT_MISO_PIN_IOCON  17

#define UART0_TX_PORT                   0
#define UART0_TX_PIN_IOCON              2
#define UART0_RX_PORT                   0
#define UART0_RX_PIN_IOCON              3

#define PRESSURE_DETECT_SPI_BAUD_RATE   1000000U
#define PRESSURE_DETECT_UART_BAUD_RATE  115200U

/* Sliding-window size used by the moving-average filter */
#define AF_WINDOW_SIZE                  32U

/* Number of pre-fill iterations after the initial seed */
#define PRESSURE_DETECT_PREFILL_COUNT   100U

/* -----------------------------------------------------------------------
 * Filter types
 * ----------------------------------------------------------------------- */

/** Kalman filter state */
typedef struct {
    float q; /**< Process noise covariance */
    float r; /**< Measurement noise covariance */
    float p; /**< Estimation error covariance */
    float x; /**< Current estimate */
    float k; /**< Kalman gain */
} kalman_filter_t;

/** Moving-average filter state */
typedef struct {
    float  *buffer;  /**< External sample buffer */
    uint8_t size;    /**< Window size */
    uint8_t index;   /**< Write index */
    float   sum;     /**< Running sum */
} average_moving_filter_t;

/** First-order low-pass filter state */
typedef struct {
    float alpha; /**< Smoothing coefficient */
    float prev;  /**< Previous output value */
} low_pass_filter_t;

/* -----------------------------------------------------------------------
 * Pressure-detection control structure
 * ----------------------------------------------------------------------- */
typedef struct {
    kalman_filter_t        kf;           /**< Kalman filter */
    average_moving_filter_t af;          /**< Moving-average filter */
    float                  af_buffer[AF_WINDOW_SIZE]; /**< Buffer for af */
    low_pass_filter_t      lpf;          /**< Low-pass filter */
    float                  origin_value; /**< Raw ADC value from last trigger */
} pressure_detect_t;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialise the SPI peripheral used by the pressure sensor.
 */
void pressure_detect_spi_init(void);

/**
 * @brief  Initialise the UART peripheral used for pressure data output.
 */
void pressure_detect_uart_init(void);

/**
 * @brief  Initialise all signal filters inside @p pressure.
 * @param  pressure  Pointer to the pressure-detection control structure.
 */
void pressure_detect_filter_init(pressure_detect_t *pressure);

/**
 * @brief  Pre-fill filters so they converge before normal operation begins.
 * @param  pressure  Pointer to the pressure-detection control structure.
 */
void pressure_detect_prefill(pressure_detect_t *pressure);

/**
 * @brief  Trigger one pressure acquisition cycle and store the raw value.
 * @param  pressure  Pointer to the pressure-detection control structure.
 */
void pressure_detect_trigger(pressure_detect_t *pressure);

/**
 * @brief  Initialise all pressure-detection base components.
 *
 * Performs SPI init, UART init, filter init, and filter pre-fill so that
 * the subsystem is ready for normal operation on return.
 *
 * @param  pressure  Pointer to the pressure-detection control structure.
 * @return 0 on success.
 */
int32_t pressure_detect_base_init(pressure_detect_t *pressure);

/* -----------------------------------------------------------------------
 * Filter helper API (implemented elsewhere / provided by BSP)
 * ----------------------------------------------------------------------- */
void kalman_filter_init(kalman_filter_t *kf, float q, float r);
void kalman_filter_reset(kalman_filter_t *kf, float value);
void average_moving_filter_init(average_moving_filter_t *af, float *buffer, uint8_t size);
void average_moving_filter_reset(average_moving_filter_t *af, float value);
void low_pass_filter_init(low_pass_filter_t *lpf, uint32_t fs, uint32_t fc);

#endif /* PRESSURE_DETECT_H */
