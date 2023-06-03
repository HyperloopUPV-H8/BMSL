#include "main.h"
#include "lwip.h"

#include "BMS-LIB.hpp"
#include "Runes/Runes.hpp"

int main(void)
{
	BMSA bmsa(SPI::spi3);
	uint8_t current_sensor = ADC::inscribe(PA0).value();
	uint8_t led = DigitalOutputService::inscribe(PG6);
	STLIB::start();

	array<uint8_t, 8> rx_data = { 0 };

	ADC::turn_on(current_sensor);


	while(1) {
		bmsa.wake_up();
		bmsa.start_adc_conversion_gpio();
		HAL_Delay(50);
		bmsa.wake_up();
		bmsa.send_receive_command(BMS::COMMAND::READ_AUXILIARY_REGISTER_GROUP_A, rx_data);
		DigitalOutputService::toggle(led);
		HAL_Delay(50);
		float current = ADC::get_value(current_sensor)*2;
		uint16_t temp  = rx_data[2] + (uint16_t)rx_data[3]<<8;
		STLIB::update();
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
	while (1){}
}
