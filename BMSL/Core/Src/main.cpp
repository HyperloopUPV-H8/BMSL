#include "main.h"
#include "lwip.h"

#include "BMSL/BMSL.hpp"
#include "Runes/Runes.hpp"

int main(void) {

	HeapOrder start_balancing_order = {
		800,
		BMSL::Orders::start_balancing,
		&BMSL::bms
	};

	HeapOrder stop_balancing_order = {
		801,
		BMSL::Orders::stop_balancing,
		&BMSL::bms
	};

	HeapPacket battery_packet = {
		810,
		&BMSL::Packets::battery_info.data
	};

	float avionics_current = 0;
	HeapPacket avionics_current_packet = HeapPacket(300, &avionics_current);
	
	BMSL::inscribe();
	BMSL::start();

	ServerSocket tcp_socket(IPV4("192.168.1.8"), 50500);
	DatagramSocket test_socket(IPV4("192.168.1.8"), 50400, IPV4("192.168.0.9"), 50400);

	while(1) {
		avionics_current = BMSL::get_avionics_current();
		test_socket.send(avionics_current_packet);
		test_socket.send(battery_packet);
		STLIB::update();
		BMSL::update();
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
}
