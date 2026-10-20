
# Audio Plotter

A real-time audio waveform plotter for the **STM32H750B-DK** discovery board. It captures audio from either the on-board **MEMS PDM microphones** or an **analog input through the ADC**, and draws the waveform live on the 480×272 LCD in an oscilloscope-style sweep.
## Demo:

|              MEMS microphone               |                ADC input                 | Recorded sine wave frequency |
| :----------------------------------------: | :--------------------------------------: | ---------------------------- |
| ![MEMS demo\|126](attachments/mems440.jpg) | ![ADC demo\|121](attachments/adc440.jpg) | 440Hz                        |
|  ![ADC demo\|121](attachments/mems1k.jpg)  | ![ADC demo\|121](attachments/adc1k.jpg)  | 1kHz                         |
| ![ADC demo\|121](attachments/mems4k_.jpg)  | ![ADC demo\|121](attachments/adc4k.jpg)  | 4Khz                         |
|  ![ADC demo\|121](attachments/mems8k.jpg)  | ![ADC demo\|121](attachments/adc8k.jpg)  | 8kHz                         |

## Hardware:

- STM32H750B-DK (Cortex-M7, 480×272 RGB LCD, external SDRAM, on-board PDM MEMS mics)
- For the ADC demo: an analog signal on **PF8** (ADC3 channel 7)
- Toolchain: STM32CubeIDE, STM32 HAL + board BSP

## Pipeline
![[attachments/diagram.png]]
```
 capture (DMA)  ──►  segment queue  ──►  decimate  ──►  draw  ──►  display
```

1. **Capture.** DMA fills a circular buffer and fires half-transfer / transfer-complete callbacks, so one half is processed while the other is being filled.
2. **Segment queue.** Each callback hands the main loop a chunk of samples as an `audio_segment_t` (pointer + length) via a small ring-buffer queue (`audio_queue.c`). This decouples the interrupt context from the drawing code.
3. **Decimation** (`audio_plotter_sample.c`). A chunk of samples is reduced to one value per pixel column of the plot segment. Three methods are available:
   - `SAMPLE_DECIMATION`: pick every Nth sample (fastest)
   - `PEAK_DECIMATION`: min/max per group (preserves transients)
   - `AVG_DECIMATION`: average per group (smoothest)

   The DC offset is subtracted here so the waveform is centred on zero.
4. **Draw** (`audio_plotter_draw.c`). The plot window is split into `n_segments` slots. Each new chunk is drawn into the next slot, then the slot index wraps, giving a sweeping display. For each update:
   - the previous frame is copied front → back buffer with **DMA2D** (memory-to-memory),
   - the segment area is cleared with **DMA2D** (register-to-memory),
   - the waveform is drawn with **Bresenham lines written directly into the SDRAM back buffer**,
   - the D-cache is cleaned so the LTDC sees the pixels,
   - the buffers are swapped by reloading the LTDC layer address **during vertical blanking** to avoid tearing.
5. **Double buffering.** Two framebuffers (`BUFFER_PING`, `BUFFER_PONG`) live in SDRAM. The SDRAM region is configured as non-cacheable via the MPU.

## MEMS and ADC configuration:

|                     | **MEMS demo** (`plot_mems.c`)                                                  | **ADC demo** (`plot_adc.c`)                                      |
| ------------------- | ------------------------------------------------------------------------------ | ---------------------------------------------------------------- |
| Source              | On-board digital PDM microphone                                                | Analog signal on PF8 (ADC3, ch. 7)                               |
| Sample format       | 1-bit PDM stream, converted to 16-bit PCM with BSP                             | 16-bit unsigned raw ADC samples                                  |
| Sample timing       | Set by the audio peripheral at 16 kHz                                          | Hardware-triggered by **TIM6 TRGO**                              |
| DMA                 | SAI/BDMA into a D3-RAM buffer (`.RAM_D3`)                                      | DMA1 Stream1, circular, into `adcDmaBuffer`                      |
| Extra processing    | `BSP_AUDIO_IN_PDMToPCM()` in the DMA callbacks, writing into a PCM ring buffer | None, raw samples are plotted directly                           |
| Hand-off to plotter | Callback pushes a 1024-sample segment into the queue                           | Callback sets `plot_flag` / `dma_wr` on the DMA half/full events |
| DC offset           | Near zero (signed PCM)                                                         | Non-zero (unsigned ADC), subtracted in decimation /sampling step |
| Cache handling      | D-cache invalidate on PDM input, clean on PCM output                           | D-cache invalidate before decimation                             |




In short: the MEMS path needs an extra **PDM → PCM** stage and uses the queue to pass finished PCM segments to the plotter, while the ADC path gets samples that are already usable and is paced by a hardware timer.

## Configuration

Defaults live in `audio_plotter.h` and `audio_plotter_config.h`:

| Define | Meaning |
|---|---|
| `DEFAULT_MAIN_WINDOW_*` | Position and size of the plot area |
| `DEFAULT_SAMPLE_SIZE` | Samples per chunk handed to the plotter |
| `DEFAULT_MAX_SEGMENTS` | Upper bound for plot segments / queue size |
| `sampling_method` | `SAMPLE_`, `PEAK_` or `AVG_DECIMATION` |

## Building & running

1. Open the project in STM32CubeIDE.
2. In `main.c`, select the demo by uncommenting: `PlotMemsDemo()` or `PlotADC_demo()`.
3. Build and flash to the STM32H750B-DK.

## License

_TBD_. Parts of the code are based on STMicroelectronics BSP examples (BSD-3-Clause).

