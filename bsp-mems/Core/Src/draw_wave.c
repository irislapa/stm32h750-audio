/*
 * display_wave.c
 *
 *  Created on: 5 Feb 2026
 *      Author: irine
 */

#include "main.h"
#include "draw_wave.h"

void init_DrawWave(draw_wave_t *draw_wave_h, int16_t *buffer,
		uint32_t x,
		uint32_t y,
		uint32_t width,
		uint32_t height)
{
	draw_wave_t *h = draw_wave_h;
	h->p_buffer = buffer;
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


void DrawHalfCB(audio_chunk_t *p_chunk, draw_window_t half_win)
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



void DrawWave_process(draw_wave_t *draw_wave_h)
{
	draw_wave_t *h = draw_wave_h;

	audio_chunk_t chunk;

	__disable_irq();
	chunk = draw_q_pop(draw_wave_h);
	__enable_irq();

	if (chunk.data == NULL)
		return;
	draw_window_t halfCBWin = { .x = h->current_x, .y = h->main_win.y, .width = h->block_width, .height = h->main_win.height };
	DrawHalfCB(&chunk, halfCBWin);

    h->current_x += h->block_width;
    // wrap around (circular)
    if (h->current_x >= (h->main_win.x + h->main_win.width))
    {
        h->current_x = h->main_win.x;
    }


}

