/*
 * display_wave.h
 *
 *  Created on: 5 Feb 2026
 *      Author: irine
 */

#ifndef INC_DRAW_WAVE_H_
#define INC_DRAW_WAVE_H_
#include <stdint.h>

typedef enum {
  TRANSFER_HALF = 0,
  TRANSFER_COMPLETE = 1,
} cb_trans_t;

typedef struct {
  uint32_t start;
  uint32_t len;
  cb_trans_t trans;
} disp_block_t;

void displayDimensions(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

void draw_q_push(uint32_t start, uint32_t len, cb_trans_t trans);
uint8_t draw_q_pop(disp_block_t *out);
void DrawWaveformBlock(const int16_t *s, uint32_t n, cb_trans_t trans, uint32_t start_x, uint32_t buff_width);
void DrawWaveformBlockN(const int16_t *s, uint32_t n, cb_trans_t trans);

#endif /* INC_DRAW_WAVE_H_ */
