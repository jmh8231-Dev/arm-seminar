################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
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


BUILD_ARTIFACT_NAME := Chap3 CLCD Test
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Chap3\ CLCD\ Test.elf \

MAP_FILES += \
Chap3\ CLCD\ Test.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Chap3\ CLCD\ Test.list \

OBJCOPY_HEX += \
Chap3\ CLCD\ Test.hex \

OBJCOPY_BIN += \
Chap3\ CLCD\ Test.bin \


# All Target
all: main-build

# Main-build Target
main-build: Chap3\ CLCD\ Test.elf secondary-outputs

# Tool invocations
Chap3\ CLCD\ Test.elf Chap3\ CLCD\ Test.map: $(OBJS) $(USER_OBJS) /Users/jmh8231/Desktop/대진대학교\ 과제/로보틱스/세미나/2025\ ARM\ 세미나/STM32CubeIDE/Chap3\ CLCD\ Test/STM32F405VGTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Chap3 CLCD Test.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/Users/jmh8231/Desktop/대진대학교 과제/로보틱스/세미나/2025 ARM 세미나/STM32CubeIDE/Chap3 CLCD Test/STM32F405VGTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Chap3 CLCD Test.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Chap3\ CLCD\ Test.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Chap3 CLCD Test.list"
	@echo 'Finished building: $@'
	@echo ' '

Chap3\ CLCD\ Test.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "Chap3 CLCD Test.hex"
	@echo 'Finished building: $@'
	@echo ' '

Chap3\ CLCD\ Test.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "Chap3 CLCD Test.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Chap3\ CLCD\ Test.bin Chap3\ CLCD\ Test.elf Chap3\ CLCD\ Test.hex Chap3\ CLCD\ Test.list Chap3\ CLCD\ Test.map Chap3\\ CLCD\\ Test.elf default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
