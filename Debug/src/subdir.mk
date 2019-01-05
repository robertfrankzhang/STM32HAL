################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/pwm.c \
../src/rtc.c \
../src/stm32f1xx_it.c \
../src/syscalls.c \
../src/system_stm32f1xx.c 

OBJS += \
./src/main.o \
./src/pwm.o \
./src/rtc.o \
./src/stm32f1xx_it.o \
./src/syscalls.o \
./src/system_stm32f1xx.o 

C_DEPS += \
./src/main.d \
./src/pwm.d \
./src/rtc.d \
./src/stm32f1xx_it.d \
./src/syscalls.d \
./src/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -DDEBUG -DSTM32F103xB -DUSE_HAL_DRIVER -I"/Users/robert/Google_Drive/2 Projects/PillBot/STM/PillBotDevice/HAL_Driver/Inc/Legacy" -I"/Users/robert/Google_Drive/2 Projects/PillBot/STM/PillBotDevice/inc" -I"/Users/robert/Google_Drive/2 Projects/PillBot/STM/PillBotDevice/CMSIS/device" -I"/Users/robert/Google_Drive/2 Projects/PillBot/STM/PillBotDevice/CMSIS/core" -I"/Users/robert/Google_Drive/2 Projects/PillBot/STM/PillBotDevice/HAL_Driver/Inc" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


