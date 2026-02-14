/*
 * display_wave.h
 *
 *  Created on: 5 Feb 2026
 *      Author: irine
 */

#ifndef INC_DRAW_WAVE_H_
#define INC_DRAW_WAVE_H_
#include <stdint.h>


#define N_BUFFERS 16
#define QUEUE_SIZE (N_BUFFERS*2)

#define PCM16_FULL_SCALE 32768

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
} draw_window_t;

typedef struct {
  int16_t *data;
  uint32_t n_samples;
} audio_chunk_t;

typedef struct {
  audio_chunk_t q[QUEUE_SIZE];
  uint16_t valid[QUEUE_SIZE];
  uint16_t write; //where to push
  uint16_t read;  //where to pop
  uint16_t count;
} audio_queue_t;

typedef struct{
	audio_queue_t draw_q;
	draw_window_t main_win;
	const int16_t *p_buffer;
	int16_t current_x;
	uint16_t block_width;
}draw_wave_t;

void displayDimensions(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

void init_DrawWave(draw_wave_t *draw_wave_h, int16_t *buffer,
		uint32_t x,
		uint32_t y,
		uint32_t width,
		uint32_t height);

void draw_q_push(draw_wave_t *draw_wave_h, audio_chunk_t chunk);
audio_chunk_t draw_q_pop(draw_wave_t *draw_wave_h);
void DrawHalfCB(audio_chunk_t *p_chunk, draw_window_t half_win);
void DrawWave_process(draw_wave_t *draw_wave_h);

#endif /* INC_DRAW_WAVE_H_ */
