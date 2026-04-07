/*
 * signal_plotter.c
 *
 *  Created on: 19 Feb 2026
 *      Author: irine
 */
#include "audio_plotter.h"
#include "measureCycles.h"
#include "plot_err.h"


#ifndef PLOT_BUFF_SIZE
#define PLOT_BUFF_SIZE 100
#endif

// todo: define the init funciton
void init_plotter(audio_plotter_handle_t *h)
{
	if (!h) {
		error_handler(ERR_NULL_PTR);
		return;
	}
	if (h->decimate.audio_source == AUDIO_SOURCE_ADC)
	{
		h->decimate.dma_wr = 0;
	}
	else if (h->decimate.audio_source == AUDIO_SOURCE_MEMS)
	{
		h->decimate.queue.size = QUEUE_SIZE;

	}
	else
	{
		error_handler(ERR_UNSUPPORTED);
		return;
	}
}


//todo: am i plotting n_buffers, half-half buffer, or am i polling n samples
void plot_adc(audio_plotter_handle_t *h)
{
  plot_buffer(h, h->plot.buffer, h->plot.buffer_size);
}


void plot_mems(audio_plotter_handle_t *h)
{
  plot_buffer(h, h->plot.buffer, h->plot.buffer_size);
}


// bottom of the callstack
uint32_t core_clock;
void plot_audio(audio_plotter_handle_t *h)
{
	DWT_Init();
	core_clock = get_system_clock();
	uint32_t t0 = DWT_GetCycles();

  SCB_InvalidateDCache_by_Addr((uint32_t *)h->decimate.buffer, h->decimate.buffer_size * sizeof(uint16_t));
  decimate(h, SAMPLE_DECIMATION);
  plot_buffer(h, h->plot.buffer, h->plot.buffer_size);
  uint32_t t1 = DWT_GetCycles();
  n_cycles = t1 - t0;
}

void error_handler(uint32_t error_code)
{
    __disable_irq();
    __BKPT(0);   // debugger stops here if attached
    uint32_t code = error_code;
    while (1)
    {
        // stay here for debugger inspection
    }
}




