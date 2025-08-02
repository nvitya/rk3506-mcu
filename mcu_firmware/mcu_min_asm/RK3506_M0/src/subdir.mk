################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../src/cortexm0_minimal.S 

OBJS += \
./src/cortexm0_minimal.o 

S_UPPER_DEPS += \
./src/cortexm0_minimal.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.S src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU Arm Cross Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -Os -fmessage-length=0 -ffunction-sections -fdata-sections -ffreestanding -fno-unwind-tables -g3 -x assembler-with-cpp -fno-unwind-tables -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


