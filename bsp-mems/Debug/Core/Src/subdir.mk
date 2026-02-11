################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/app_timers.c \
../Core/Src/audio_play.c \
../Core/Src/audio_record.c \
../Core/Src/draw_wave.c \
../Core/Src/main.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32h7xx.c 

OBJS += \
./Core/Src/app_timers.o \
./Core/Src/audio_play.o \
./Core/Src/audio_record.o \
./Core/Src/draw_wave.o \
./Core/Src/main.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32h7xx.o 

C_DEPS += \
./Core/Src/app_timers.d \
./Core/Src/audio_play.d \
./Core/Src/audio_record.d \
./Core/Src/draw_wave.d \
./Core/Src/main.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32h7xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../PDM2PCM/App -I../Middlewares/ST/STM32_Audio/Addons/PDM/Inc -I../Drivers/CMSIS_DSP/Include -I../Drivers/CMSIS_DSP/Include/dsp -I../Drivers/CMSIS_DSP/PrivateInclude -I../Drivers/BSP/STM32H750B-DK -I/bsp-mems/Drivers/BSP -I../Utilities/lcd -I../Utilities/CPU -I../Drivers/BSP/Components/Common -I../Utilities/Fonts -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/app_timers.cyclo ./Core/Src/app_timers.d ./Core/Src/app_timers.o ./Core/Src/app_timers.su ./Core/Src/audio_play.cyclo ./Core/Src/audio_play.d ./Core/Src/audio_play.o ./Core/Src/audio_play.su ./Core/Src/audio_record.cyclo ./Core/Src/audio_record.d ./Core/Src/audio_record.o ./Core/Src/audio_record.su ./Core/Src/draw_wave.cyclo ./Core/Src/draw_wave.d ./Core/Src/draw_wave.o ./Core/Src/draw_wave.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32h7xx_it.cyclo ./Core/Src/stm32h7xx_it.d ./Core/Src/stm32h7xx_it.o ./Core/Src/stm32h7xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32h7xx.cyclo ./Core/Src/system_stm32h7xx.d ./Core/Src/system_stm32h7xx.o ./Core/Src/system_stm32h7xx.su

.PHONY: clean-Core-2f-Src

