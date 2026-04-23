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
#define PLOT_BUFF_SIZE 50
#endif

int16_t plot_buff[PLOT_BUFF_SIZE];


// todo: define default setup
void init_plotter(audio_plotter_handle_t *h)
{
	if (!h) {
		error_handler(ERR_NULL_PTR);
		return;
	}
	if (h->decimate.audio_source == AUDIO_SOURCE_ADC)
	{
		h->decimate.dma_wr = 0;
		h->plot.buffer = &plot_buff;
		h->plot.buffer_size = PLOT_BUFF_SIZE;
	}
	else if (h->decimate.audio_source == AUDIO_SOURCE_MEMS)
	{
		h->decimate.dma_wr = 0;
		h->plot.buffer = &plot_buff;
		h->plot.buffer_size = PLOT_BUFF_SIZE;
		h->decimate.queue.size = AUDIO_QUEUE_SIZE;
		h->decimate.queue.r = 0;
		h->decimate.queue.wr = 0;

	}
	else
	{
		error_handler(ERR_UNSUPPORTED);
		return;
	}
}

//todo: implement n_buffers, and n_polls

//todo: what am i plotting? n_buffers, half-half buffer, or am i polling n samples
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







/*=========== error handler for fatal errors in plotter code ============ */
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




