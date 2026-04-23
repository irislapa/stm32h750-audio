/*
 * audio_plotter_decimate.c
 *
 *  Created on: 19 Mar 2026
 *      Author: irinej
 */

#include "audio_plotter.h"


void plotter_queue_push(audio_plotter_handle_t *h, audio_segment_t seg)
{
	if (!h) return;
    audio_queue_t *q = &h->decimate.queue;

    __disable_irq();
	if (q->count < q->size)
	{
		q->buffer[q->wr] = seg;
	    q->wr = (q->wr + 1) % q->size;
	    q->count++;
	}
	__enable_irq();
}

audio_segment_t plotter_queue_pop(audio_plotter_handle_t *h)
{
	if (!h) return (audio_segment_t){NULL, 0};
	audio_queue_t *q = &h->decimate.queue;
	audio_segment_t seg = {NULL, 0};

	__disable_irq();
	if (q->count > 0)
	{
		uint16_t i = q->r;
		audio_segment_t seg = q->buffer[i];
		q->r = (q->r + 1) % q->size;
		q->count--;

	}
	__enable_irq();

	return seg;
}

void sample_decimate(const uint16_t *p_buff_in, uint32_t in_size, 
  uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size)
{
  if (out_size == 0 || p_buff_in == NULL || p_buff_out == NULL)
  {
    error_handler(ERR_INVALID_PARAM);
    return;
  }

  uint16_t group_size = (uint16_t) (in_size / out_size);
  if (group_size == 0)
  {
    error_handler(ERR_OUT_OF_BOUNDS);
    return;
  }

  for(uint32_t i = 0; i < out_size; i++)
  {
      uint32_t idx = (i * group_size);
      if (idx < in_size)
      {
          uint16_t raw = p_buff_in[idx];
          p_buff_out[i] = (int16_t)((int32_t)raw - dc_offset);
      }
  }
}

void peak_decimate(const uint16_t *p_buff_in, uint32_t in_size,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size)
{
  if (!p_buff_in || !p_buff_out || in_size == 0 || out_size < 2u) return;
  if (out_size & 1u) return; // must be even


  uint32_t n_groups = out_size / 2u;
  uint32_t step = in_size / n_groups;
  if (step == 0u) step = 1u;

  for (uint32_t g = 0; g < n_groups; g++)
  {
    int16_t mn = INT16_MAX;
    int16_t mx = INT16_MIN;

    uint32_t base = g * step;

    for (uint32_t j = 0; j < step; j++)
    {
      int32_t s = (int32_t)p_buff_in[base + j] - dc_offset;

      // cap to range
      if (s < mn) mn = (int16_t)s;
      if (s > mx) mx = (int16_t)s;
    }
    p_buff_out[2u*g]     = mn;
    p_buff_out[2u*g + 1] = mx;
  }
}

void avg_decimate(const uint16_t *p_buff_in, uint32_t in_size,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size)
{
  if (!p_buff_in || !p_buff_out || in_size == 0 || out_size == 0) return;

  uint32_t step = (uint16_t) (in_size / out_size);
  if (step == 0u) step = 1u;

  for (uint32_t i = 0; i < out_size; i++)
  {
    uint32_t base = i * step;

    if (base + step > in_size) break;

    int32_t sum = 0;
    for (uint32_t j = 0; j < step; j++)
    {
      sum += p_buff_in[base + j];
    }

    p_buff_out[i] = (int16_t)(sum / (int32_t)step) - dc_offset;
  }
}

void decimate(audio_plotter_handle_t *h, uint32_t decimation_method)
{
	uint16_t *p_buff_in = h->decimate.buffer;
    uint32_t in_size = h->decimate.buffer_size;
    int32_t dc_offset = 0; //mems default
    int16_t *p_buff_out = h->plot.buffer;
    uint32_t out_size = h->plot.buffer_size;

    // AUDIO_SOURCE_MEMS is typically signed PCM (offset 0)
    // AUDIO_SOURCE_ADC is typically unsigned (offset 32768)
    if (h->decimate.audio_source == AUDIO_SOURCE_MEMS)
    {
      audio_segment_t seg = plotter_queue_pop(&h); // pop the segment we just plotted
      p_buff_in = (uint16_t *)seg.p_samples;
      in_size = seg.n_samples;
    }
    else if (h->decimate.audio_source == AUDIO_SOURCE_ADC)
    {
        dc_offset = INT16_MAX;
    }
    else
    {
        // error: unsupported audio source
        error_handler(ERR_UNSUPPORTED);
        return;
    }
    SCB_InvalidateDCache_by_Addr((uint16_t *)p_buff_in, in_size* sizeof(uint16_t));
    if ((uint16_t *)p_buff_in == NULL || in_size == 0) return;
    switch (decimation_method)
    {
        case SAMPLE_DECIMATION:
            sample_decimate(p_buff_in, in_size, dc_offset, p_buff_out, out_size);
            break;

        case PEAK_DECIMATION: // Combined into one logic using dc_offset
            peak_decimate(p_buff_in, in_size, dc_offset, p_buff_out, out_size);            break;

        case AVG_DECIMATION:
            avg_decimate(p_buff_in, in_size, dc_offset, p_buff_out, out_size);
            break;

        default:
            error_handler(ERR_UNSUPPORTED);
          break;
    }
}
