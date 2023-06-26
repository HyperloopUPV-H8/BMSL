#pragma once

#include "BMS-LIB.hpp"
#include "BMSL/Measurements.hpp"

namespace BMSL {
    float total_voltage;

    struct battery_data {
        float* data[13];
    };
    
    battery_data serialize_battery(Battery& battery) {
        return {
                &battery.SOC,
                battery.cells[0],
                battery.cells[1],
                battery.cells[2],
                battery.cells[3],
                battery.cells[4],
                battery.cells[5],
                &battery.minimum_cell,
                &battery.maximum_cell,
                battery.temperature1,
                battery.temperature2,
                (float*)&battery.is_balancing,
                &battery.total_voltage
        };
    }

    battery_data battery_info;

    class Packets {
    public:
        HeapPacket battery_info_packet;
        HeapPacket conditions_packet;
        HeapPacket avionics_current_packet;

        Packets() :
        battery_info_packet(810, battery_info.data),
        conditions_packet(811, &Conditions::ready, &Conditions::want_to_charge, &Conditions::charging, &Conditions::fault),
        avionics_current_packet(812, &Measurements::avionics_current, &Measurements::input_charging_current, &Measurements::output_charging_current, &Measurements::input_charging_voltage, &Measurements::output_charging_voltage, &Measurements::pwm_frequency) {}
    };

    class UDP {
    public:
        DatagramSocket backend;
        UDP() {}

        void init() {
            backend = DatagramSocket(IPV4("192.168.1.8"), 50400, IPV4("192.168.0.9"), 50400);
            backend.reconnect();
        }

        void send_to_backend(Packet& packet) {
            backend.send(packet);
        }
    };
}