/**
  ******************************************************************************
  * @file    BSP/Src/record_adc.c
  * @brief   Audio record demo using ADC + DMA.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "audio_plotter.h"

/* Parameters ----------------------------------------------------------------*/
#define ADC_DMA_BUFFER_SIZE 1024U

/* DMA buffers ---------------------------------------------------------------*/
ALIGN_32BYTES (uint16_t adcDmaBuffer[ADC_DMA_BUFFER_SIZE]);

static audio_plotter_handle_t hplotter_adc;
static volatile uint32_t adc_dma_write_idx = 0;

/**
  * @brief  ADC record demo with waveform plotter.
  * @retval None
  */
void AudioRecordADC_demo(void)
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
  UTIL_LCD_DisplayStringAt(0, 0, (uint8_t *)"ADC RECORD PLOTTER", CENTER_MODE);
  UTIL_LCD_SetFont(&Font16);
  UTIL_LCD_DisplayStringAt(0, 24, (uint8_t *)"Press User button to stop", CENTER_MODE);

  /* Plot frame */
  UTIL_LCD_DrawRect(10, 100, x_size - 20, y_size - 110, UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_DrawRect(11, 101, x_size - 22, y_size - 112, UTIL_LCD_COLOR_BLUE);

  MX_DMA_Init();
  MX_TIM6_Init();
  MX_ADC3_Init();
  HAL_TIM_Base_Start(&htim6);
  HAL_ADC_Start_DMA(&hadc3, (uint32_t*)adcDmaBuffer, ADC_DMA_BUFFER_SIZE);

  hplotter_adc.input_buffer = (int16_t*)adcDmaBuffer;
  hplotter_adc.input_buffer_size = ADC_DMA_BUFFER_SIZE;
  hplotter_adc.window.x = 12;
  hplotter_adc.window.y = 102;
  hplotter_adc.window.width = x_size - 24;
  hplotter_adc.window.height = y_size - 114;
  hplotter_adc.p_dma_write_idx = (uint32_t *)&adc_dma_write_idx;

  init_plotter(&plotter_adc, (void *)NULL);

  while (1)
  {
    signal_plotter_plot(&plotter_adc);

    if (CheckForUserInput() > 0)
    {
      ButtonState = 0;
      HAL_ADC_Stop_DMA(&hadc3);
      HAL_TIM_Base_Stop(&htim6);
      return;
    }
  }
}

/**
  * @brief  Conversion complete callback in non-blocking mode.
  * @param  hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC3)
  {
    hplotter_adc.transfer_dma = DMA_TRANSFER_COMPLETE;
    hplotter_adc.dma_write_idx = ADC_DMA_BUFFER_SIZE;
  }
}

/**
  * @brief  Conversion DMA half-transfer callback in non-blocking mode.
  * @param  hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC3)
  {
    hplotter_adc.transfer_dma = DMA_TRANSFER_HALF;
    hplotter_adc.dma_write_idx = ADC_DMA_BUFFER_SIZE / 2U;
  }
}
