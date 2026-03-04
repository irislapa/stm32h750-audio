################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Utilities/lcd/subdir.mk
-include PDM2PCM/App/subdir.mk
-include Drivers/STM32H7xx_HAL_Driver/Src/subdir.mk
-include Drivers/BSP/STM32H750B-DK/subdir.mk
-include Drivers/BSP/Components/wm8994/subdir.mk
-include Drivers/BSP/Components/mt48lc4m32b2/subdir.mk
-include Drivers/BSP/Components/mt25tl01g/subdir.mk
-include Drivers/BSP/Components/ft5336/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := bsp-mems
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
bsp-mems.elf \

MAP_FILES += \
bsp-mems.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
bsp-mems.list \


# All Target
all: main-build

# Main-build Target
main-build: bsp-mems.elf secondary-outputs

# Tool invocations
bsp-mems.elf bsp-mems.map: $(OBJS) $(USER_OBJS) D:\faks\stm32h750-audio\bsp-mems\STM32H750XBHX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "bsp-mems.elf" @"objects.list" $(USER_OBJS) -Wl,--start-group $(LIBS) -Wl,--end-group -mcpu=cortex-m7 -T"D:\faks\stm32h750-audio\bsp-mems\STM32H750XBHX_FLASH.ld" --specs=nosys.specs -Wl,-Map="bsp-mems.map" -Wl,--gc-sections -static -L../Middlewares/ST/STM32_Audio/Addons/PDM/Lib --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

bsp-mems.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "bsp-mems.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) bsp-mems.elf bsp-mems.list bsp-mems.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
