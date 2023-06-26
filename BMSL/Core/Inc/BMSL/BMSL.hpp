#include "BMS-LIB.hpp"
#include "BMSL/Orders.hpp"
#include "BMSL/Sensors.hpp"
#include "BMSL/Packets.hpp"
#include "BMSL/StateMachine.hpp"
#include "BMSL/Protections.hpp"
#include "BMSL/Pins.hpp"

namespace BMSL {
    IncomingOrders orders;
    TCP tcp;
    UDP udp;
    Packets packets;

    void inscribe();
    void update();

    void inscribe() {
        bms = BMSA(SPI::spi3);

        Leds::can = DigitalOutput(LED_CAN);
        Leds::fault = DigitalOutput(LED_FAULT);
        Leds::flash = DigitalOutput(LED_FLASH);
        Leds::full_charge = DigitalOutput(LED_FULL_CHARGE);
        Leds::low_charge = DigitalOutput(LED_LOW_CHARGE);
        Leds::operational = DigitalOutput(LED_OPERATIONAL);
        Leds::sleep = DigitalOutput(LED_SLEEP);

        Sensors::avionics_current = LinearSensor<double>(AVIONICSCURRENTSENSORFW, 2.7, -0.14, &Measurements::avionics_current);
        Sensors::input_charging_current = LinearSensor<double>(INPUTCURRENTFW, 3.78, -0.14, &Measurements::input_charging_current);
        Sensors::output_charging_current = LinearSensor<double>(OUTPUTCURRENTFW, 10.4, -0.14, &Measurements::output_charging_current);
        Sensors::input_charging_voltage = LinearSensor<double>(INPUTVOLTAGEFW, 91.74, -4.05, &Measurements::input_charging_voltage);
        Sensors::output_charging_voltage = LinearSensor<double>(OUTPUTVOLTAGEFW, 8.86, -0.42, &Measurements::output_charging_voltage);

        StateMachines::general = StateMachine(States::General::CONNECTING);
        StateMachines::operational = StateMachine(States::Operational::IDLE);
        charging_control = ChargingControl(&Measurements::input_charging_current, &Conditions::want_to_charge, &bms.external_adc.battery.SOC, DO_INV_PWM_H1, DO_INV_PWM_L1, DO_INV_PWM_H2, DO_INV_PWM_L2, BUFFER_EN);

        Reset_HW = DigitalOutput(HW_FAULT);
    }

    void start() {
        STLIB::start("192.168.1.8");

        udp.init();
        tcp.init();

        bms.initialize();
        StateMachines::start(); 

        battery_info = serialize_battery(bms.external_adc.battery);
        BMSL::Reset_HW.turn_on();


        Time::set_timeout(5000, [&](){
            BMSL::Conditions::ready = true;
        });

        StateMachines::general.check_transitions();    
    }

    void update() {
        STLIB::update();
        StateMachines::general.check_transitions();
    }
};