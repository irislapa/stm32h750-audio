/*
 * signal_plotter.c
 *
 *  Created on: 19 Feb 2026
 *      Author: irine
 */
#include "audio_plotter.h"
#include "measureCycles.h"


#ifndef PLOT_BUFF_SIZE
#define PLOT_BUFF_SIZE 128  
#endif

#define DMA_TRANSFER_HALF 0
#define DMA_TRANSFER_FULL 1

static int16_t plot_buff[PLOT_BUFF_SIZE];
static uint16_t n_buff_history = 4;



static plotter_config_t default_config;

// gropu size = 2
// out_size = 128
// dma_len = 4096
// dma_idx = 2048 or 4096
// 2048/128 = 16 groups of 2

void sample_decimation(const uint16_t *dma_buf,
                             uint32_t dma_len,
                             uint32_t dma_idx,
                             uint32_t group_size,
                             int16_t *out,
                             uint32_t out_size)
{
	if (group_size == 0)
	{
		group_size = uint16_t (dma_len / out_size);
	}

  //should not exceed more than half of the dma buffer size
  int32_t start_idx = dma_idx-(group_size * out_size); 
  if (start_idx < 0)
  {
    //error: groups too large
    // error output size too large
    return;
  }

  for(uint32_t i = 0; i < out_size; i++)
  {
      uint32_t idx = (start_idx + i * group_size);
      if (idx >= dma_len)
      {
        //error: index out of bounds
        return;
      }
      uint16_t raw = dma_buf[idx];
      out[i] = (int16_t)((int32_t)raw - RANGE_16_BIT);
  }
}



void peak_decimation(int16_t *inputSamplesBuff, uint32_t n_samples, int16_t *outputDecimatedBuff, uint32_t n_groups)
{
  if (inputSamplesBuff == NULL|| outputDecimatedBuff == NULL || n_samples == 0 || n_groups == 0)
    return;

  // Ensure the division yields an exact integer step
  if (n_samples % n_groups != 0) {
      // Handle error (e.g., return early, adjust n_groups, or pad input)
      return;
  }

  uint32_t step = n_samples / n_groups;

  for (uint32_t i = 0; i < n_groups; i++)
  {
  	if (i * step >= n_samples) {
  		return; // Prevent out-of-bounds access
  	}
      uint16_t max_val = 0;
      for (uint32_t j = 0; j < step; j++)
      {
          uint16_t sample = inputSamplesBuff[i * step + j];
          if (sample > max_val)
          {
              max_val = sample;
          }
      }
      outputDecimatedBuff[i] = max_val;
  }
}

void high_res_decimation(int16_t *inputSamplesBuff, uint32_t n_samples, int16_t *outputDecimatedBuff, uint32_t n_groups)
{
  if (inputSamplesBuff == NULL|| outputDecimatedBuff == NULL || n_samples == 0 || n_groups == 0)
    return;

  // Ensure the division yields an exact integer step
  if (n_samples % n_groups != 0) {
      // Handle error (e.g., return early, adjust n_groups, or pad input)
      return;
  }

  uint32_t step = n_samples / n_groups;

  for (uint32_t i = 0; i < n_groups; i++)
  {
  	if (i * step >= n_samples) {
  		return; // Prevent out-of-bounds access
  	}
      uint32_t sum = 0;
      for (uint32_t j = 0; j < step; j++)
      {
          sum += inputSamplesBuff[i * step + j];
      }
      outputDecimatedBuff[i] = sum / step;
  }
}
 
#define SAMPLE_DECIMATION 0
#define PEAK_DECIMATION 1
#define AVG_DECIMATION 2
void decimate(audio_plotter_handle_t *h, uint32_t decimation_method)
{
  if(decimation_method == SAMPLE_DECIMATION)
  {
    sample_decimation(h->input_buffer, h->input_buffer_size, *(h->p_dma_write_idx), 0, h->cfg.plot_buffer, h->cfg.plot_buffer_size);
  }
  else if(decimation_method == PEAK_DECIMATION)
  {
    peak_decimation(h->cfg.plot_buffer, h->cfg.plot_buffer_size, h->cfg.plot_buffer, h->cfg.plot_buffer_size);
  }
  else if(decimation_method == AVG_DECIMATION)
  {
    high_res_decimation(h->cfg.plot_buffer, h->cfg.plot_buffer_size, h->cfg.plot_buffer, h->cfg.plot_buffer_size);
  }
}



void plot_segment(int16_t *seg, uint32_t segSize, draw_window_t w)
{
  UTIL_LCD_FillRect(w.x, w.y, w.width, w.height, UTIL_LCD_COLOR_WHITE);

  const int32_t mid = (int32_t)w.y + (int32_t)w.height / 2;
  uint32_t yp = mid;
  uint32_t xp = w.x;

  for (uint32_t i = 0; i < segSize; i++)
  {
    int16_t sample = (int16_t)seg[i];
    int32_t y = mid - (sample * (int32_t)(w.height / 2) / RANGE_16_BIT);
    // clip amplitude to display area
    if (y < (int32_t)w.y)
      {y = (int32_t)w.y;}
    if (y > (int32_t)(w.y + w.height - 1))
      {y = (int32_t)(w.y + w.height - 1);}
  
    uint32_t x = (uint32_t)(w.x + i);
    UTIL_LCD_DrawLine(xp, yp, x, y, UTIL_LCD_COLOR_BLUE);
    xp = x;
    yp = y;
  }
}

/**
 * @brief should use plot_segment to plot first half and second half of the buffer interchangibly 
 * 
 * @param sampleBuffer
 * @param window
 */
void plot_buffer(audio_plotter_handle_t *h,int16_t *buff, uint32_t buffSize)
{
  // clear window area
  // which half of buffer is being plotted
  h->cfg.signal_window.y = h->window.y;
  h->cfg.signal_window.width = h->window.width/2;
  h->cfg.signal_window.height = h->window.height;
  

  decimate(h, SAMPLE_DECIMATION);

  if (h->transfer_dma == DMA_TRANSFER_HALF)
  {
    h->cfg.signal_window.x = h->window.x;
    plot_segment(buff, buffSize / 2, h->cfg.signal_window);
  }
  else
  {
    h->cfg.signal_window.x = h->window.width/2 + h->window.x;
    plot_segment(&buff[buffSize / 2], buffSize / 2, h->cfg.signal_window);
  }
}

/**
 * todo:
 * returns the last N samples from the dma circular buffer
 */
void get_snapshot(const uint16_t *srcBuffBase, uint32_t srcBuffSize, uint32_t srcIdx, uint16_t *snapshotBuff, uint32_t snapshotBuffSize)
{
	// Can't get more samples than the buffer holds
	if (snapshotBuffSize > srcBuffSize) {
		return;
	}

	SCB_InvalidateDCache_by_Addr((uint32_t*)srcBuffBase,
	                             srcBuffSize * sizeof(uint16_t));
	uint32_t start = (srcIdx - snapshotBuffSize + srcBuffSize) % srcBuffSize;

	uint32_t firstChunk = srcBuffSize - start;
	if (firstChunk >= snapshotBuffSize) {
		// If the start index is smaller srcBuffSize - snapshotBufferSize samples are copied in 1 chunk
		memcpy(snapshotBuff, &srcBuffBase[start], snapshotBuffSize * sizeof(uint16_t));
	} else {
		// If overflow occurse eg. snapshot size is larger than half of record buffer size, samples are copied in 2 chunks
		uint32_t secondChunk = snapshotBuffSize - firstChunk;
		memcpy(snapshotBuff, &srcBuffBase[start], firstChunk * sizeof(uint16_t));
		memcpy(&snapshotBuff[firstChunk], srcBuffBase, secondChunk * sizeof(uint16_t));
	}
}

void init_plotter(signal_plotter_handle_t *handle, plotter_config_t *config)
{
  if (config == NULL)
  {
    default_config.plot_buffer = &plot_buff;
    default_config.plot_buffer_size = PLOT_BUFF_SIZE;
    default_config.n_buff_history = n_buff_history;
    default_config.signal_window.x = handle->window.x;
    default_config.signal_window.y = handle->window.y;
    default_config.signal_window.width = handle->window.width/n_buff_history;
    default_config.signal_window.height = handle->window.height;
    handle->cfg = default_config;
  }
  else
  {
	  handle->cfg = *config;
  }
}


/**
 * todo: calls plot_buffer in defferent ways
 * @brief 
 * 
 * @param 
 */
uint32_t core_clock;
void plot_audio(signal_plotter_handle_t *handle)
{
	DWT_Init();
	core_clock = get_system_clock();
	uint32_t t0 = DWT_GetCycles();

  SCB_InvalidateDCache_by_Addr((uint32_t *)handle->input_buffer, handle->input_buffer_size * sizeof(uint16_t));
  sample_decimation(handle->input_buffer, 
                    handle->input_buffer_size, 
                    (uint32_t) *(handle->p_dma_write_idx), 
                    2,
                    handle->cfg.plot_buffer, 
                    handle->cfg.plot_buffer_size);

  plot_segment(handle->cfg.plot_buffer, handle->cfg.plot_buffer_size, handle->cfg.signal_window); 
  handle->cfg.signal_window.x += handle->cfg.signal_window.width;
  if (handle->cfg.signal_window.x >= (handle->window.x + handle->window.width))
  {
    handle->cfg.signal_window.x = handle->window.x;
  }

  uint32_t t1 = DWT_GetCycles();
  n_cycles = t1 - t0;
}





