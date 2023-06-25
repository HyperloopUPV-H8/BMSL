#include "main.h"
#include "lwip.h"

#include "BMSL/BMSL.hpp"
#include "Runes/Runes.hpp"

int main(void) {

	/************************************************
	 *              PROTECTIONS
	 ***********************************************/

	//add_protection(&BMSL::Measurements::charging_current, Boundary<double, ABOVE>(300));
	add_protection(&ErrorHandlerModel::error_triggered, Boundary<double, ProtectionType::NOT_EQUALS>(0));


	Battery& battery = BMSL::bms.external_adc.battery;
	// for (float* cell : battery.cells) {
	// 	add_protection(cell, Boundary<float, ProtectionType::BELOW>(3.3));
	// 	add_protection(cell, Boundary<float, ProtectionType::ABOVE>(4.2));
	// }

	// add_protection(battery.temperature1, Boundary<float, ProtectionType::ABOVE>(70));
	// add_protection(battery.temperature2, Boundary<float, ProtectionType::ABOVE>(70));
	// add_protection(&battery.disbalance, Boundary<float, ProtectionType::ABOVE>(Battery::MAX_DISBALANCE));
	// add_protection(&battery.total_voltage, Boundary<float, ProtectionType::BELOW>(19.8));


	/************************************************
	 *              	ORDERS
	 ***********************************************/


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

	HeapOrder set_dclv_duty_cycle_order = {
		803,
		BMSL::Orders::set_dclv_duty_cycle,
		&BMSL::Orders::selected_number
	};

	HeapOrder set_dclv_phase_order = {
		804,
		BMSL::Orders::set_dclv_phase,
		&BMSL::Orders::selected_number
	};

	HeapOrder reset_board = {
		805,
		&BMSL::Orders::reset_board
	};
	HeapOrder start_all_pwm_order = {
		890,
		BMSL::Orders::start_all_pwm,
	};

	HeapOrder stop_all_pwm_order = {
		891,
		BMSL::Orders::stop_all_pwm,
	};

	HeapOrder frequency_sweep = {
		892,
		BMSL::Orders::frequency_sweep,
		&BMSL::Orders::selected_number
	};

	HeapOrder stop_frequency_sweep = {
		893,
		BMSL::Orders::stop_frequency_sweep,
	};
	

	/************************************************
	 *              	PACKETS
	 ***********************************************/
	
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
		812,
		&BMSL::Measurements::avionics_current,
		&BMSL::Measurements::input_charging_current,
		&BMSL::Measurements::output_charging_current,
		&BMSL::Measurements::input_charging_voltage,
		&BMSL::Measurements::output_charging_voltage,
		&BMSL::Measurements::pwm_frequency
	);
	
	BMSL::inscribe();
	BMSL::start();

	ServerSocket tcp_socket(IPV4("192.168.1.8"), 50500);
	DatagramSocket test_socket(IPV4("192.168.1.8"), 50400, IPV4("192.168.0.9"), 50400);

	Time::register_low_precision_alarm(15, [&](){
		test_socket.send(avionics_current_packet);
		test_socket.send(battery_packet);
	});

	while(1) {
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
}
