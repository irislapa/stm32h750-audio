/*
 * signal_plotter.c
 *
 *  Created on: 19 Feb 2026
 *      Author: irine
 */
#include "signal_plotter.h"
#include "measureCycles.h"


#ifndef PLOT_BUFF_SIZE
#define PLOT_BUFF_SIZE 128  
#endif

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

/**
 * @brief
 *
 * @param sampleBuffer
 * @param window
 */
void plot_buffer(int16_t *sampleBuffer, uint32_t sampleBufferSize, draw_window_t window)
{
  // clear window area
  UTIL_LCD_FillRect(window.x, window.y, window.width, window.height, UTIL_LCD_COLOR_WHITE);

  uint32_t prev_y = (int32_t) (window.y + window.height/2);

  uint32_t i = (sampleBufferSize > window.width) ? (sampleBufferSize - window.width) : 0;
  uint16_t step = (window.width > sampleBufferSize) ? (uint16_t)(window.width / sampleBufferSize) : 1;
  uint16_t px = window.x;
  for (; i < sampleBufferSize; i++)
  {
    int16_t sample = (int16_t)sampleBuffer[i];
    int32_t y = (int32_t)(window.y + window.height/2) - (sample * (int32_t)(window.height/2) / RANGE_16_BIT);
    // clip amplitude to display area
    if (y < (int32_t)window.y)
      {y = (int32_t)window.y;}
    if (y > (int32_t)(window.y + window.height - 1))
      {y = (int32_t)(window.y + window.height - 1);}

    if (px > window.x + window.width)
    {
      break; // stop if we exceed the window width
    }
    UTIL_LCD_DrawLine(px, prev_y, px + step, y, UTIL_LCD_COLOR_BLUE);
    px += step; // adjust px to account for the line drawn
    prev_y = y;
  }
}


void plot_segment(int16_t *in, uint32_t in_size, draw_window_t win)
{
  UTIL_LCD_FillRect(win.x, win.y, win.width, win.height, UTIL_LCD_COLOR_WHITE);

  const int32_t mid = (int32_t)win.y + (int32_t)win.height / 2;
  int32_t prev_y = mid;

  for (uint32_t i = 0; i < in_size; i++)
  {
    int16_t sample = (int16_t)in[i];
    int32_t y = mid - (sample * (int32_t)(win.height / 2) / RANGE_16_BIT);
    // clip amplitude to display area
    if (y < (int32_t)win.y)
      {y = (int32_t)win.y;}
    if (y > (int32_t)(win.y + win.height - 1))
      {y = (int32_t)(win.y + win.height - 1);}
  
    uint32_t x = (uint32_t)(win.x + i);
    UTIL_LCD_DrawLine(x, prev_y, x+1, y, UTIL_LCD_COLOR_BLUE);
    prev_y = y;
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
void signal_plotter_plot(signal_plotter_handle_t *handle)
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


