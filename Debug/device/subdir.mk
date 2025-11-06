################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../device/system_MK22F51212.c 

C_DEPS += \
./device/system_MK22F51212.d 

OBJS += \
./device/system_MK22F51212.o 


# Each subdirectory must supply rules for building sources it contributes
device/%.o: ../device/%.c device/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK22FN512VLL12 -DCPU_MK22FN512VLL12_cm4 -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\source\controls" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\source\io" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\source\com" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\source\utils" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\startup" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\source" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\drivers" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\device" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\CMSIS" -I"C:\Users\felix\Documents\MCUXpressoIDE_11.10.0_3148\workspace_PREN\PREN_Puzzleroboter\board" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-device

clean-device:
	-$(RM) ./device/system_MK22F51212.d ./device/system_MK22F51212.o

.PHONY: clean-device

