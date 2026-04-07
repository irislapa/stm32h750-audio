/*
 * audio_plotter_decimate.c
 *
 *  Created on: 19 Mar 2026
 *      Author: irinej
 */

#include "audio_plotter.h"


void plotter_queue_push(audio_plotter_handle_t *h, audio_segment_t seg)
{
	audio_queue_t *q = &h->decimate.queue;

	if (q->count >= q->size)
	{
		//todo: handle overflow
		return;
	}

	uint16_t i = q->wr;
	q->q[i] = seg;
	q->wr = (q->wr + 1) % q->size;

	__disable_irq();
	q->count++;
	__enable_irq();
}

audio_segment_t plotter_queue_pop(audio_plotter_handle_t *h)
{
	audio_queue_t *q = &h->decimate.queue;

	if (q->count == 0)
	{
		return (audio_segment_t){ .p_samples= NULL, .n_samples = 0 };
	}

	uint16_t i = q->r;
	audio_segment_t seg = q->q[i];
	q->r = (q->r + 1) % q->size;

	__disable_irq();
	q->count--;
	__enable_irq();

	return seg;
}

void sample_decimate(const uint16_t *p_buff_in, uint32_t in_size, uint32_t dma_idx,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size)
{
  uint16_t group_size = (uint16_t) (in_size / out_size);

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
      if (idx < in_size)
      {
          uint16_t raw = p_buff_in[idx];
          p_buff_out[i] = (int16_t)((int32_t)raw - dc_offset);
      }


  }
}

void peak_decimate(const uint16_t *p_buff_in, uint32_t in_size, uint32_t dma_idx,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size)
{
    if (!p_buff_in || !p_buff_out || in_size == 0 || out_size < 2u) return;
    if (out_size & 1u) return; // must be even


    uint32_t n_groups = out_size / 2u;
    uint32_t end = dma_idx % in_size;
    uint32_t step = in_size / n_groups;
    if (step == 0u) step = 1u;
    uint32_t window = step * n_groups;
    uint32_t start0 = (end + in_size - (window % in_size)) % in_size;


    for (uint32_t g = 0; g < n_groups; g++)
    {
        int16_t mn = INT16_MAX;
        int16_t mx = INT16_MIN;

        uint32_t base = start0 + g * step;

        for (uint32_t j = 0; j < step; j++)
        {
            uint32_t k = (base + j) % in_size;
            int32_t s = (int32_t)p_buff_in[k] - dc_offset;

            if (s < mn) mn = (int16_t)s;
            if (s > mx) mx = (int16_t)s;
        }
        p_buff_out[2u*g]     = mn;
        p_buff_out[2u*g + 1] = mx;
    }
}

void avg_decimate(const uint16_t *p_buff_in, uint32_t in_size, uint32_t dma_idx,
		uint32_t dc_offset, int16_t *p_buff_out, uint32_t out_size)
{
    if (!p_buff_in || !p_buff_out) return;
    if (in_size == 0 || out_size == 0) return;

    // Determine group size (samples per output)
    uint32_t step = (uint16_t) (in_size / out_size);

    if (step == 0u) {
        step = in_size / out_size;
        if (step == 0u) step = 1u;
    }

    uint32_t group = step * out_size;

    // Start index of the window ending at dma_idx
    int32_t start_idx = (int32_t)dma_idx - (int32_t)group;
    if (start_idx < 0) {
        // todo: window would underflow
        return;
    }
    if ((uint32_t)dma_idx > in_size) {
        // todo: end index out of bounds
        return;
    }
    if ((uint32_t)start_idx + group > in_size) {
        // todo: group would exceed buffer end
        return;
    }

    for (uint32_t i = 0; i < out_size; i++)
    {
        uint32_t base = (uint32_t)start_idx + i * step;

        // todo:
        if (base + step > in_size) return;

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
	const uint16_t *p_buff_in = h->decimate.buffer;
    uint32_t in_size = h->decimate.buffer_size;
    uint32_t dma_idx = h->decimate.dma_wr;

    int16_t *p_buff_out = h->plot.buffer;
    uint32_t out_size = h->plot.buffer_size;

    // AUDIO_SOURCE_MEMS is typically signed PCM (offset 0)
    // AUDIO_SOURCE_ADC is typically unsigned (offset 32768)
    int32_t dc_offset = (h->decimate.audio_source == AUDIO_SOURCE_MEMS) ? 0 : RANGE_16_BIT;

    switch (decimation_method)
    {
        case SAMPLE_DECIMATION:
            sample_decimate(p_buff_in, in_size, dma_idx, p_buff_out, out_size, dc_offset);
            break;

        case PEAK_DECIMATION: // Combined into one logic using dc_offset
            peak_decimate(p_buff_in, in_size, dma_idx, p_buff_out, out_size, dc_offset);
            break;

        case AVG_DECIMATION:
            avg_decimate(p_buff_in, in_size, dma_idx, p_buff_out, out_size, dc_offset);
            break;

        default:
        	// todo:
            // error_trap(ERR_INVALID_METHOD, __FILE__, __LINE__);
            break;
    }
}
