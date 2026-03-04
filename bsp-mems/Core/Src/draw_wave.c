/*
 * display_wave.c
 *
 *  Created on: 5 Feb 2026
 *      Author: irine
 */

#include "main.h"
#include "draw_wave.h"

void init_DrawWavePDM(draw_wave_t *draw_wave_h, int16_t *buffer,
		uint32_t x,
		uint32_t y,
		uint32_t width,
		uint32_t height)
{
	draw_wave_t *h = draw_wave_h;
	h->p_buffBase= buffer;
	h->main_win.x = x;
	h->main_win.y = y;
	h->main_win.width = width;
	h->main_win.height = height;

	for (uint8_t i = 0; i < QUEUE_SIZE; i++)
	{
		h->draw_q.valid[i] = 0;
	}
	h->draw_q.write = 0;
	h->draw_q.read = 0;
	h->draw_q.count = 0;
    h->current_x  = h->main_win.x;
    h->block_width = h->main_win.width / QUEUE_SIZE;
}

void init_DrawWaveADC(draw_wave_t *draw_wave_h, int16_t *buffer, int16_t *snapBuff, int16_t *drawBuff,
		uint32_t x,
		uint32_t y,
		uint32_t width,
		uint32_t height)
{
	draw_wave_t *h = draw_wave_h;
	h->p_buffBase= buffer;
	h->buffBaseSize = DMA_N_SAMPLES;
	h->p_snapBuff = snapBuff;
	h->snapBuffSize = 1024;
	h->p_drawBuff = drawBuff;
	h->drawBuffSize = 256;

	h->main_win.x = x;
	h->main_win.y = y;
	h->main_win.width = width;
	h->main_win.height = height;

}

void draw_q_push(draw_wave_t *draw_wave_h, audio_chunk_t chunk)
{
	audio_queue_t *q = &draw_wave_h->draw_q;

	uint8_t i = q->write;
	q->valid[i] = 1;
	q->q[i] = chunk;
	q->write = (q->write + 1) % N_BUFFERS;
}

audio_chunk_t draw_q_pop(draw_wave_t *draw_wave_h)
{
	audio_queue_t *q = &draw_wave_h->draw_q;

	uint8_t i = q->read;
	if (!q->valid[i])
	{
		return (audio_chunk_t){ .data = NULL, .n_samples = 0 };
	}
	audio_chunk_t out = q->q[i];
	q->valid[i] = 0;
	q->read = (q->read + 1) % N_BUFFERS;
	return out;
}


void DrawChunk(audio_chunk_t *p_chunk, draw_window_t half_win)
{

  // clear window area
  UTIL_LCD_FillRect(half_win.x, half_win.y, half_win.width, half_win.height, UTIL_LCD_COLOR_WHITE);

  uint32_t prev_y = (int32_t)(half_win.y + half_win.height/2);

  uint32_t px = 0; //iterating over pixel on the horizontal (x) axis
  for (;px < half_win.width; px++)
  {
    uint32_t i = (px* p_chunk->n_samples) / half_win.width;
    int32_t sample = (int16_t)p_chunk->data[i]; // -32768..32767
    int32_t y = (int32_t)(half_win.y + half_win.height/2) - (sample * (int32_t)(half_win.height/2) / PCM16_FULL_SCALE);

    // clip amplitude to display area
    if (y < (int32_t)half_win.y)
      {y = (int32_t)half_win.y;}
    if (y > (int32_t)(half_win.y + half_win.height - 1))
      {y = (int32_t)(half_win.y + half_win.height - 1);}
    if (px > 0)
    {
      UTIL_LCD_DrawLine(half_win.x + px-1, prev_y, half_win.x + px, y, UTIL_LCD_COLOR_BLUE);
    }
    prev_y = y;
  }
}

//used when audio device pushes data via callback
void DrawWave_processCB(draw_wave_t *draw_wave_h)
{
	draw_wave_t *h = draw_wave_h;

	audio_chunk_t chunk;

	__disable_irq();
	chunk = draw_q_pop(draw_wave_h);
	__enable_irq();

	if (chunk.data == NULL)
		return;
	draw_window_t halfCBWin = { .x = h->current_x, .y = h->main_win.y, .width = h->block_width, .height = h->main_win.height };
	DrawChunk(&chunk, halfCBWin);

    h->current_x += h->block_width;
    // wrap around (circular)
    if (h->current_x >= (h->main_win.x + h->main_win.width))
    {
        h->current_x = h->main_win.x;
    }
}

void sampleDecimateADC(uint16_t *inputSamplesBuff, uint32_t n_samples, uint16_t *outputDecimatedBuff, uint32_t n_groups);
void hrDecimateADC(uint32_t *inputSamplesBuff , uint32_t *outputDecimatedBuff);
void peakDecimateADC(uint32_t *inputSamplesBuff , uint32_t *outputDecimatedBuff);
void getSnapshot(uint16_t *srcBuffBase, uint32_t srcBuffSize, uint16_t *snapshotBuff, uint32_t snapshotBuffSize);
// used when audio device continuously refills a circular buffer
void DrawWave_processContinuous(draw_wave_t *draw_wave_h)
{
	draw_wave_t *h = draw_wave_h;

	draw_window_t win = { .x = h->current_x, .y = h->main_win.y, .width = h->drawBuffSize, .height = h->main_win.height };
	getSnapshot(h->p_buffBase, h->buffBaseSize, h->p_snapBuff, h->snapBuffSize);
	sampleDecimateADC(h->p_snapBuff, h->snapBuffSize, h->p_drawBuff, h->drawBuffSize);
	audio_chunk_t chunk;
	chunk.data = (int16_t*)h->p_drawBuff;
	chunk.n_samples = h->drawBuffSize;
	DrawChunk(&chunk, win);
}


uint32_t *dmaWriteIdx;


/**
 * todo:
 * seperates the input samples buffer into groups, and returns only the last samples of the groups
 *
 *
 */
void sampleDecimateADC(uint16_t *inputSamplesBuff, uint32_t n_samples, uint16_t *outputDecimatedBuff, uint32_t n_groups)
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
        outputDecimatedBuff[i] = inputSamplesBuff[i * step];
    }
}
/**
 * todo:
 * seperates the input samples buffer into groups, and returns average sample values of the groups
 */
void hrDecimateADC(uint32_t *inputSamplesBuff , uint32_t *outputDecimatedBuff)
{
	return;

}

/**
 * todo:
 * seperates the input samples buffer into groups, finds the largest and lowest sample value of each group, and returns them
 */
void peakDecimateADC(uint32_t *inputSamplesBuff , uint32_t *outputDecimatedBuff)
{
	return;
}


/**
 * todo:
 * returns the last N samples from the dma circular buffer
 */
extern DMA_HandleTypeDef hdma_adc3;
void getSnapshot(uint16_t *srcBuffBase, uint32_t srcBuffSize, uint16_t *snapshotBuff, uint32_t snapshotBuffSize)
{
	// Can't get more samples than the buffer holds
	if (snapshotBuffSize > srcBuffSize) {
		return;
	}

	uint32_t srcIdx = srcBuffSize - __HAL_DMA_GET_COUNTER(&hdma_adc3);

	uint32_t start = (srcIdx - snapshotBuffSize + srcBuffSize) % srcBuffSize;

	uint32_t firstChunk = srcBuffSize - start;
	if (firstChunk >= snapshotBuffSize) {
		// If the start index is smaller srcBuffSize - snapshotBufferSize samples are copied in 1 chunk
		memcpy(snapshotBuff, &srcBuffBase[start], snapshotBuffSize * sizeof(uint16_t));
	} else {
		// If it goes over the edge, samples are copied in 2 chunks
		uint32_t secondChunk = snapshotBuffSize - firstChunk;
		memcpy(snapshotBuff, &srcBuffBase[start], firstChunk * sizeof(uint16_t));
		memcpy(&snapshotBuff[firstChunk], srcBuffBase, secondChunk * sizeof(uint16_t));
	}


}
