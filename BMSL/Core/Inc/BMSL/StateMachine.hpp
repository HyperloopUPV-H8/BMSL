#pragma once

#include "BMS-LIB.hpp"
#include "BMSL/Conditions.hpp"
#include "BMSL/Leds.hpp"
#include "BMSL/Measurements.hpp"
#include "BMSL/Sensors.hpp"

namespace BMSL {
    
    BMSA bms;
    DigitalOutput Reset_HW;
    uint8_t sweep_action;
    ChargingControl charging_control;

    namespace States {
        enum General : uint8_t {
            CONNECTING = 0,
            OPERATIONAL = 1,
            FAULT = 2
        };
        enum Operational : uint8_t {
            CHARGING = 10,
            BALANCING = 11,
            IDLE = 12
        };
    }

    namespace StateMachines {

        #define CURRENT_SETPOINT 5

        StateMachine general;
        StateMachine operational;
        StateMachine charging;

        void setup_states() {
            using Gen = States::General;
            using Op = States::Operational;


            general.add_state(Gen::OPERATIONAL);
            general.add_state(Gen::FAULT);

            operational.add_state(Op::CHARGING);
            operational.add_state(Op::BALANCING); 
            general.add_state_machine(operational, Gen::OPERATIONAL);
            operational.add_state_machine(charging, Op::CHARGING);
        }

        void setup_transitions() {
            using Gen = States::General;
            using Op = States::Operational;

            general.add_transition(Gen::CONNECTING, Gen::OPERATIONAL, [&]() {
                return Conditions::ready;
            });

            general.add_transition(Gen::OPERATIONAL, Gen::FAULT, [&]() {
                return Conditions::fault;
            });

            general.add_transition(Gen::CONNECTING, Gen::FAULT, [&]() {
                return Conditions::fault;
            });
            
            operational.add_transition(Op::IDLE, Op::CHARGING, [&]() {
                return Conditions::want_to_charge;
            });

            operational.add_transition(Op::CHARGING, Op::IDLE, [&]() {
                return not Conditions::want_to_charge;
            });

            // operational.add_transition(Op::CHARGING, Op::BALANCING, [&]() {
            //     if (bms.external_adc.battery.needs_balance()) {
            //         return true;
            //     }
            //     return false;
            // });

            // operational.add_transition(Op::BALANCING, Op::CHARGING, [&]() {
            //     if (bms.external_adc.battery.needs_balance()) {
            //         return false;
            //     }
            //     return true;
            // });

            operational.add_transition(Op::BALANCING, Op::IDLE, [&]() {
                return not Conditions::want_to_charge;
            });

        }

        void setup_actions() {
            using Gen = States::General;
            using Op = States::Operational;
                        
            general.add_enter_action([&]() {
                Leds::operational.turn_on();
            }, Gen::OPERATIONAL);

            general.add_enter_action([&]() {
                Leds::fault.turn_on();
                Conditions::fault = true;
            }, Gen::FAULT);

            general.add_low_precision_cyclic_action([&]() {
                Leds::operational.toggle();
            }, ms(200), Gen::CONNECTING);

            general.add_mid_precision_cyclic_action([&]() {
                Sensors::avionics_current.read();
                Sensors::input_charging_current.read();
                Sensors::output_charging_current.read();
                Sensors::input_charging_voltage.read();
                Sensors::output_charging_voltage.read();
            }, ms(200), {Gen::OPERATIONAL, Gen::FAULT});


            general.add_mid_precision_cyclic_action([&]() {
                if (Conditions::ready) {
                    ProtectionManager::check_protections();
                }
            }, us(100), Gen::OPERATIONAL);

            general.add_exit_action([&]() {
                Leds::operational.turn_off();
            }, Gen::OPERATIONAL);

            general.add_exit_action([&]() {
                Leds::fault.turn_off();
            }, Gen::FAULT);

            operational.add_enter_action([&]() {
                Leds::low_charge.turn_on();
                BMSL::Conditions::charging = true;
            }, Op::CHARGING);

            general.add_mid_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.start_adc_conversion_all_cells();

                Time::set_timeout(10, [&]() {
                    bms.wake_up();
                    bms.read_cell_voltages();
                });
            }, ms(100), {Gen::OPERATIONAL, Gen::FAULT});

            general.add_mid_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.measure_internal_device_parameters();

                Time::set_timeout(10, [&]() {
                    bms.wake_up();
                    bms.read_internal_temperature();
                });
            }, ms(100), {Gen::OPERATIONAL, Gen::FAULT});

            general.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.start_adc_conversion_temperatures();

                Time::set_timeout(10, [&]() {
                    bms.wake_up();
                    bms.read_temperatures();
                });
            }, ms(100), {Gen::OPERATIONAL, Gen::FAULT});

            general.add_low_precision_cyclic_action([&]() {
                bms.external_adc.battery.update_data();
            }, ms(100), {Gen::OPERATIONAL, Gen::FAULT});
        
            operational.add_mid_precision_cyclic_action([&]() {
                Sensors::input_charging_current.read();
                charging_control.charger_pi.input(CURRENT_SETPOINT - Measurements::input_charging_current);
                charging_control.charger_pi.execute();
                // charging_control.pwm_frequency += charging_control.charger_pi.output_value;
                //charging_control.dclv.set_frequency(charging_control.pwm_frequency);
            }, us(200), Op::CHARGING);
            
            operational.add_exit_action([&]() {
                Leds::low_charge.turn_off();
                charging_control.dclv.turn_off();
                BMSL::Conditions::charging = false;
            }, Op::CHARGING);


        }

        void start() {
            setup_states();
            setup_transitions();
            setup_actions();
            
            general.check_transitions();
        }
    }
} 
