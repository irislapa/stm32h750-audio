/*
 * display_wave.c
 *
 *  Created on: 5 Feb 2026
 *      Author: irine
 */

#include "main.h"
#include "draw_wave.h"

static volatile disp_block_t g_draw_q[2]; // queue for displaying 2 of the callback blocks (half and full)
static volatile uint8_t g_slot_valid[2] = {0,0};
static volatile uint8_t g_write_idx = 0, g_read_idx= 0;

static uint32_t g_width;
static uint32_t g_height;
static uint32_t g_x;
static uint32_t g_y;

uint32_t g_start_x = 0;
uint32_t g_nbuffs = 2;

void draw_q_push(uint32_t start, uint32_t len, cb_trans_t trans)
{
	uint8_t i = g_write_idx;
	g_slot_valid[i] = 1;
	g_draw_q[i].start = start;
	g_draw_q[i].len   = len;
	g_draw_q[i].trans = trans;
	g_write_idx ^= 1;
}

uint8_t draw_q_pop(disp_block_t *out)
{
	uint8_t i = g_read_idx;
	if (!g_slot_valid[i])
	{
		return 0;
	}
	*out = g_draw_q[i];
	g_slot_valid[i] = 0;
	g_read_idx ^= 1;
	return 1;
}

void displayDimensions(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	g_x = x;
	g_y = y;
	g_width = width;
	g_height = height;
	g_start_x = x;
}


void DrawWaveformBlock(const int16_t *s, uint32_t n, cb_trans_t trans, uint32_t start_x, uint32_t buff_width)
{
	uint32_t x0;
	const uint32_t y0 = g_y;
	const uint32_t width = buff_width/2; // pixels
	const uint32_t height = g_height;   // pixels
	const uint32_t mid = y0 + height/2;

	if (trans == TRANSFER_HALF)
	{
		x0 = start_x;
	}
	else
	{
		x0 = start_x + width;
	}

	// clear waveform area
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_FillRect(x0, y0, width, height, UTIL_LCD_COLOR_WHITE);

	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);

	// decimate samples to width
	uint32_t step = (n / width);
	if (step < 1)
	{
		step = 1;
	}

	int32_t prev_y = (int32_t)mid;
	uint32_t x = 0;

	for (uint32_t x = 0; x < width; x++)
	{
		// s is int16 PCM. Map to y in [y0, y0+h)
		uint32_t i = (x*n)/ width;
		int32_t v = s[i]; // -32768..32767
		int32_t y = (int32_t)mid - (v * (int32_t)(height/2) / 32768);

		// clip to display area
		if (y < (int32_t)y0)
		{
			y = (int32_t)y0;
		}
		if (y > (int32_t)(y0 + height - 1))
		{
			y = (int32_t)(y0 + height - 1);
		}
		if (x > 0)
		{
		  UTIL_LCD_DrawLine(x0 + x - 1, (uint32_t)prev_y, x0 + x, (uint32_t)y, UTIL_LCD_COLOR_BLUE);
		}
		prev_y = y;
	}
}


void DrawWaveformBlockN(const int16_t *s, uint32_t n, cb_trans_t trans)
{
  // pair = 0 draws into left half (quarters 0+1)
  // pair = 1 draws into right half (quarters 2+3)
	uint32_t width = g_width/g_nbuffs;
	DrawWaveformBlock(s, n, trans, g_start_x, width);

	if (trans == TRANSFER_COMPLETE)
	{
		// after full block, move start x for next block
		// (half block doesn't move start x, so next block will draw on other half)
		g_start_x += width;

		// wrap around (circular)
		if (g_start_x >= g_x + g_width)
		{
			g_start_x = g_x;
		}
	}
}

