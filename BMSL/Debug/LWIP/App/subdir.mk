################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../LWIP/App/lwip.c 

C_DEPS += \
./LWIP/App/lwip.d 

OBJS += \
./LWIP/App/lwip.o 


# Each subdirectory must supply rules for building sources it contributes
LWIP/App/%.o LWIP/App/%.su LWIP/App/%.cyclo: ../LWIP/App/%.c LWIP/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DNucleo=8000000UL -DBoard=25000000UL -DHSE_VALUE=Nucleo -DUSE_HAL_DRIVER -DSTM32H723xx -DDATA_IN_D2_SRAM -c -I"/home/alejandro/projects/BMSL/ST-LIB" -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"/home/alejandro/projects/BMSL/ST-LIB/Inc" -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/HALAL/Models" -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/HALAL/Services" -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/ST-LIB_LOW" -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/ST-LIB_HIGH" -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/HALAL/Services/Communication" -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/HALAL/Services/Communication/Ethernet" -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/HALAL/Services/Communication/Ethernet/UDP" -I"/home/alejandro/projects/BMSL/ST-LIB/Inc/HALAL/Services/Communication/Ethernet/TCP" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-LWIP-2f-App

clean-LWIP-2f-App:
	-$(RM) ./LWIP/App/lwip.cyclo ./LWIP/App/lwip.d ./LWIP/App/lwip.o ./LWIP/App/lwip.su

.PHONY: clean-LWIP-2f-App

