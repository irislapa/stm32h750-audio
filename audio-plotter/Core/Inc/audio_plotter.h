/*
 * signal_plotter.h
 *
 *  Created on: 19 Feb 2026
 *      Author: irine
 */

#ifndef INC_SIGNAL_PLOTTER_H_
#define INC_SIGNAL_PLOTTER_H_

#include <stdint.h>
#include <limits.h>
#include "main.h"

#define RANGE_16_BIT 32768
#define DMA_TRANSFER_HALF 0
#define DMA_TRANSFER_COMPLETE 1

#define AUDIO_SOURCE_ADC 0
#define AUDIO_SOURCE_MEMS 1

#define SAMPLE_DECIMATION 0
#define PEAK_DECIMATION 1
#define AVG_DECIMATION 2

#define QUEUE_SIZE 4

typedef struct {
  int16_t *p_samples;
  uint32_t n_samples;
} audio_segment_t;

typedef struct {
  uint16_t size;
  audio_segment_t q[QUEUE_SIZE];
  uint16_t wr; //where to push
  uint16_t r;  //where to pop
  uint16_t count;
} audio_queue_t;

typedef struct {
	uint8_t audio_source;
	uint16_t *buffer;
	uint16_t buffer_size;

	audio_queue_t queue;
	uint8_t plot_flag;
	uint32_t dma_wr;
	uint8_t dma_transfer;
} decimate_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} draw_window_t;

typedef struct {
  int16_t *buffer;
  uint16_t buffer_size;
  uint16_t n_buffs;
  draw_window_t window;
} plot_t;

typedef struct {
  decimate_t decimate;
  plot_t plot;
  draw_window_t main_window;
} audio_plotter_handle_t;


void sample_decimate(const uint16_t *p_buff_in, uint32_t in_size, uint32_t dma_idx,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size);
void peak_decimate(const uint16_t *p_buff_in, uint32_t in_size, uint32_t dma_idx,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size);
void avg_decimate(const uint16_t *p_buff_in, uint32_t in_size, uint32_t dma_idx,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size);
void decimate(audio_plotter_handle_t *h, uint32_t decimation_method);

void plot_segment_wave(const int16_t *seg, uint32_t seg_size, draw_window_t w);
void plot_segment_minmax(const int16_t *seg, uint32_t seg_size, draw_window_t w);
void plot_buffer(audio_plotter_handle_t *h,int16_t *buff, uint32_t buffSize);

void init_plotter_adc(audio_plotter_handle_t *h, uint8_t audio_source);

//void get_snapshot(int16_t *srcBuffBase, uint32_t srcBuffSize, uint32_t *srcIdx, int16_t *snapshotBuff, uint32_t snapshotBuffSize);

#endif /* INC_SIGNAL_PLOTTER_H_ */
