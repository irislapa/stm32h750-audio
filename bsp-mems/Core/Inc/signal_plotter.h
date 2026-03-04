/*
 * signal_plotter.h
 *
 *  Created on: 19 Feb 2026
 *      Author: irine
 */

#ifndef INC_SIGNAL_PLOTTER_H_
#define INC_SIGNAL_PLOTTER_H_

#include <stdint.h>
#include "main.h"

#define RANGE_16_BIT 32768



typedef enum {
    WAVE_DECIMATE_SAMPLE = 0,
    WAVE_DECIMATE_HIGH_RES,
    WAVE_DECIMATE_PEAK
} decimation_mode_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} draw_window_t;

typedef struct {
  int16_t *plot_buffer;
  uint16_t plot_buffer_size;
  uint16_t n_buff_history;
  draw_window_t signal_window;
} plotter_config_t;

typedef struct {
  draw_window_t window;
  uint16_t *input_buffer;
  int16_t input_buffer_size;

  uint32_t *p_dma_write_idx;
  plotter_config_t cfg;

} signal_plotter_handle_t;


//void signal_plotter_init(signal_plotter_handle_t *handle);
void plot_buffer(int16_t *sampleBuffer, uint32_t sampleBufferSize, draw_window_t window);
void signal_plotter_plot(signal_plotter_handle_t *handle);
void signal_plotter_draw_waveform(signal_plotter_handle_t *handle);

void sample_decimation(const uint16_t *dma_buf, uint32_t dma_len, uint32_t dma_idx, uint32_t group_size, int16_t *out, uint32_t out_size);
void peak_decimation(int16_t *input,  uint32_t input_size, int16_t *output, uint32_t output_size);
void high_res_decimation(int16_t *input,  uint32_t input_size, int16_t *output, uint32_t output_size);

//void get_snapshot(int16_t *srcBuffBase, uint32_t srcBuffSize, uint32_t *srcIdx, int16_t *snapshotBuff, uint32_t snapshotBuffSize);

#endif /* INC_SIGNAL_PLOTTER_H_ */
