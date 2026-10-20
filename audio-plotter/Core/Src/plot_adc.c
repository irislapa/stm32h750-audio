/**
  ******************************************************************************
  * @file    audio-plotter/Core/Src/plot_adc.c
  * @brief   Audio record demo using ADC + DMA.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include "string.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "audio_plotter.h"

/* Parameters ----------------------------------------------------------------*/
#define ADC_DMA_BUFFER_SIZE FFT_SAMPLE_SIZE
//#define ADC_DMA_BUFFER_SIZE MAX_ADC_DMA_BUFF_SIZE
uint32_t current_adcDmaBuffer_size = ADC_DMA_BUFFER_SIZE;

/* DMA buffers ---------------------------------------------------------------*/
ALIGN_32BYTES (uint16_t adcDmaBuffer[MAX_ADC_DMA_BUFF_SIZE]);

__attribute__((section(".RAM_D2"))) audio_plotter_handle_t hplot_adc;

extern ADC_HandleTypeDef hadc3;
extern DMA_HandleTypeDef hdma_adc3;

extern uint32_t channel_nbr;
extern __IO uint32_t ButtonState;
extern uint8_t CheckForUserInput(void);

extern float32_t fft_in[];

void set_sample_size(audio_plotter_handle_t* h, uint16_t sample_size);
void set_segments(audio_plotter_handle_t* h, uint16_t n_segments);


/**
  * @brief  ADC record demo with waveform plotter.
  * @retval None
  */
void PlotAdcDemo(void)
{
  uint32_t x_size, y_size;

  BSP_LCD_GetXSize(0, &x_size);
  BSP_LCD_GetYSize(0, &y_size);

  /* Clear the LCD */
  UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);
  /* Header */
  UTIL_LCD_FillRect(0, 0, x_size, 90, UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
  UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_SetFont(&Font24);
  UTIL_LCD_DisplayStringAt(0, 0, (uint8_t *)"EXTERNAL ANALOG MICROPHONE", CENTER_MODE);
  UTIL_LCD_SetFont(&Font16);
  //UTIL_LCD_DisplayStringAt(0, 40, (uint8_t *)"Press User button to toggle audio input", CENTER_MODE);  // not enough flash for both demos


  MX_DMA_Init();
  MX_TIM6_Init();
  MX_ADC3_Init();
  HAL_TIM_Base_Start(&htim6);

  hplot_adc.sampling.audio_source    = AUDIO_SOURCE_ADC;
  hplot_adc.sampling.sampling_method = PEAK_DECIMATION;
  hplot_adc.sampling.data			 = (uint16_t*)adcDmaBuffer;
  hplot_adc.sampling.data_size       = ADC_DMA_BUFFER_SIZE;
  hplot_adc.main_window.x            = 12;
  hplot_adc.main_window.y            = 102;
  hplot_adc.main_window.width        = x_size - 24;
  hplot_adc.main_window.height       = y_size - 114;


  draw_window_t spectogram_window;
  spectogram_window.x = 12;
  spectogram_window.y = 101;
  spectogram_window.width = 220;
  spectogram_window.height = 150;
  UTIL_LCD_DrawRect(10, 100, 224, 152, UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_DisplayStringAt(12, 70, (uint8_t *)"Frequency histogram:", LEFT_MODE);

  draw_window_t waveform_window;
  waveform_window.x = 224 + 10 + 12;
  waveform_window.y = 101;
  waveform_window.width = 220;
  waveform_window.height = 150;
  UTIL_LCD_DrawRect(10 + 224 + 10, 100, 224, 152, UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_DisplayStringAt(10 + 224 + 30, 70, (uint8_t *)"   Waveform:", LEFT_MODE);

  UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
  UTIL_LCD_DisplayStringAt(8, 258, (uint8_t *)"0Hz              8kHz", LEFT_MODE);

  //set_segments(&hplot_adc, 2);
  //set_sample_size(&hplot_adc, FFT_SAMPLE_SIZE);
  HAL_ADC_Start_DMA(&hadc3, (uint32_t*)adcDmaBuffer, FFT_SAMPLE_SIZE);
  init_plotter(&hplot_adc);

  while (1)
  {
		if (!queue_pop(&hplot_adc)) continue;
		draw_waveform(&hplot_adc, waveform_window);
		draw_freq_spectogram(&hplot_adc, spectogram_window);

    if (CheckForUserInput() > 0)
    {
      ButtonState = 0;
      HAL_ADC_Stop_DMA(&hadc3);
      HAL_TIM_Base_Stop(&htim6);
      return;
    }
  }
}





HAL_StatusTypeDef set_DMA_buff_size(uint32_t buff_size)
{
	if (buff_size == 0 || buff_size > MAX_ADC_DMA_BUFF_SIZE)
	{
		return HAL_ERROR;
	}
	if (HAL_ADC_Stop_DMA(&hadc3) != HAL_OK)
	{
		return HAL_ERROR;
	}
	current_adcDmaBuffer_size = buff_size;
    return HAL_ADC_Start_DMA(&hadc3, (uint32_t *)adcDmaBuffer, buff_size);
}



void set_sample_size(audio_plotter_handle_t* h, uint16_t sample_size)
{
	set_DMA_buff_size(sample_size);
}

void set_segments(audio_plotter_handle_t* h, uint16_t n_segments)
{
	segment_config(h, n_segments);
	set_sample_size(&h, MAX_ADC_DMA_BUFF_SIZE/n_segments);
}


/**
  * @brief  Conversion complete callback in non-blocking mode.
  * @param  hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
   if (hadc->Instance != ADC3) return;

   SCB_InvalidateDCache_by_Addr((uint32_t*)&adcDmaBuffer[current_adcDmaBuffer_size/2],
		   current_adcDmaBuffer_size/2 * sizeof(uint16_t));

   audio_segment_t seg = {
	 .p_samples = (int16_t*)&adcDmaBuffer[current_adcDmaBuffer_size/2],
	 .n_samples = current_adcDmaBuffer_size/2
   };

   __disable_irq();
   queue_push(&hplot_adc, seg);
   copy_fft(adcDmaBuffer, 512, current_adcDmaBuffer_size/2);
   __enable_irq();
}


/**
  * @brief  Conversion DMA half-transfer callback in non-blocking mode.
  * @param  hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
   if (hadc->Instance != ADC3) return;

   SCB_InvalidateDCache_by_Addr((uint32_t*)&adcDmaBuffer[0],
		   current_adcDmaBuffer_size/2 * sizeof(uint16_t));

   audio_segment_t seg = {
	 .p_samples = (int16_t*)&adcDmaBuffer[0],
	 .n_samples = current_adcDmaBuffer_size/2
   };

   __disable_irq();
   queue_push(&hplot_adc, seg);
   copy_fft(adcDmaBuffer, 512, 0);
   __enable_irq();
}
