#include "main.h"
#include "lwip.h"

#include "BMSL/BMSL.hpp"
#include "Runes/Runes.hpp"

int main(void) {

	//add_protection(&BMSL::Measurements::charging_current, Boundary<double, ABOVE>(300));

	HeapOrder start_charging_order = {
		800,
		BMSL::Orders::start_charging,
		&BMSL::bms
	};

	HeapOrder stop_charging_order = {
		801,
		BMSL::Orders::stop_charging,
		&BMSL::bms
	};

	HeapOrder set_dclv_frequency_order = {
		802,
		BMSL::Orders::set_dclv_frequency,
		&BMSL::Orders::selected_number
	};

	HeapOrder start_all_pwm_order = {
		890,
		BMSL::Orders::start_all_pwm,
	};

	HeapOrder stop_all_pwm_order = {
		891,
		BMSL::Orders::stop_all_pwm,
	};

	HeapPacket battery_packet = {
		810,
		&BMSL::Packets::battery_info.data
	};

	HeapPacket conditions_packet = {
		811,
		&BMSL::Conditions::charging,
		&BMSL::Conditions::fault,
		&BMSL::Conditions::ready,
		&BMSL::Conditions::want_to_charge,
	};

	HeapPacket avionics_current_packet = HeapPacket(
		300,
		&BMSL::Measurements::avionics_current,
		&BMSL::Measurements::input_charging_current,
		&BMSL::Measurements::output_charging_current,
		&BMSL::Measurements::input_charging_voltage,
		&BMSL::Measurements::output_charging_voltage	
	);
	
	BMSL::inscribe();
	BMSL::start();

	ServerSocket tcp_socket(IPV4("192.168.1.8"), 50500);
	DatagramSocket test_socket(IPV4("192.168.1.8"), 50400, IPV4("192.168.0.9"), 50400);

	BMSL::Orders::start_charging();
	while(1) {
		test_socket.send(avionics_current_packet);
		test_socket.send(battery_packet);
		BMSL::update();
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
}
