#include "main.h"
#include "lwip.h"

#include "BMSL/BMSL.hpp"
#include "Runes/Runes.hpp"

using namespace BMSL;

int main(void) {

	static_assert(BMS::EXTERNAL_ADCS == 1, "EXTERNAL_ADCS must be 1");
	
	#ifndef BOARD
		static_assert(false, "Board code can not be run in Nucleo mode");
	#endif

	#ifdef HSE_VALUE
		static_assert(HSE_VALUE == 25000000, "HSE_VALUE must be 25000000");
	#endif

	IncomingOrders orders;

	inscribe();
	start();

	BMSL::charging_control.dclv.turn_on();
	BMSL::charging_control.dclv.set_duty_cycle(50);
	BMSL::charging_control.dclv.set_frequency(100000);
	BMSL::charging_control.dclv.set_phase(10);



	while (not BMSL::Conditions::ready) {
		__NOP();
	}

	Time::register_low_precision_alarm(15, [&](){
		udp.send_to_backend(packets.avionics_current_packet);
		udp.send_to_backend(packets.battery_info_packet);

		update();
	});
	
	while(1) {}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
}
