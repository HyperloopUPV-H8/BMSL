#include "main.h"
#include "lwip.h"

#include "BMSL/BMSL.hpp"
#include "Runes/Runes.hpp"

int main(void) {

	static_assert(BMS::EXTERNAL_ADCS == 1, "EXTERNAL_ADCS must be 1");
	
	BMSL::IncomingOrders orders;

	BMSL::inscribe();
	BMSL::start();

	Time::register_low_precision_alarm(15, [&](){
		BMSL::udp.send_to_backend(BMSL::packets.avionics_current_packet);
		BMSL::udp.send_to_backend(BMSL::packets.conditions_packet);
		BMSL::udp.send_to_backend(BMSL::packets.battery_info_packet);
	});

	while (not BMSL::Conditions::ready) {
		__NOP();
	}

	while(1) {
		BMSL::update();
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
}
