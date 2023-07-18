#pragma once

#include "BMS-LIB.hpp"
#include "BMSL/Conditions.hpp"
#include "BMSL/Leds.hpp"
#include "BMSL/Measurements.hpp"
#include "BMSL/StateMachine.hpp"

namespace BMSL {
    namespace Orders {
        int selected_pwm = 0;
        float selected_number = 0;

        void start_charging() {
            Conditions::want_to_charge = true;
        };

        void stop_charging() {
            Conditions::want_to_charge = false;
        };

        void start_all_pwm() {
            charging_control.pwm_frequency = 100000;
            charging_control.dclv.set_duty_cycle(30);
            charging_control.charger_pi.reset();
            charging_control.dclv.turn_on();
            charging_control.dclv.set_duty_cycle(30);
            charging_control.dclv.set_frequency(100000);
        }

        void start_pwm_1() {
        	charging_control.dclv.positive_pwm.turn_on_positive();
        }

        void start_pwm_2() {
        	charging_control.dclv.positive_pwm.turn_on_negated();
        }

        void start_pwm_3() {
        	charging_control.dclv.negative_pwm.turn_on_positive();
        }

        void start_pwm_4() {
        	charging_control.dclv.negative_pwm.turn_on_negated();
        }

        void stop_all_pwm() {
            charging_control.dclv.turn_off();
        }

        void set_pwm_phase() {
        }

        void set_pwm_frequency() {
        }

        void set_dclv_frequency() {
            charging_control.dclv.set_frequency(selected_number);
            Measurements::pwm_frequency = selected_number;
        }

        void set_dclv_duty_cycle() {
            charging_control.dclv.set_duty_cycle(selected_number);
        }

        void set_dclv_phase() {
            charging_control.dclv.set_phase(selected_number);
        }

        void set_pwm_duty_cycle(uint8_t duty_cycle) {
            
        }

        void reset_board() {
            NVIC_SystemReset();
        }

        void turn_on_pwm() {
            if (selected_pwm == 0) {
                charging_control.dclv.positive_pwm.turn_on_positive();
                charging_control.dclv.positive_pwm.set_duty_cycle(50);
            } else if (selected_pwm == 1) {
                charging_control.dclv.positive_pwm.turn_on_negated();
                charging_control.dclv.positive_pwm.set_duty_cycle(50);
            } else if (selected_pwm == 2) {
                charging_control.dclv.negative_pwm.turn_on_positive();
                charging_control.dclv.negative_pwm.set_duty_cycle(50);
            } else if (selected_pwm == 3) {
                charging_control.dclv.negative_pwm.turn_on_negated();
                charging_control.dclv.negative_pwm.set_duty_cycle(50);
            }
        }

        void frequency_sweep() {
            charging_control.pwm_frequency = 200000;
            sweep_action = Time::register_mid_precision_alarm(100,
            [&](){
                if (charging_control.pwm_frequency > 250000) {
                    charging_control.pwm_frequency = 200000;
                }
                charging_control.pwm_frequency += 1;
                charging_control.dclv.set_frequency(charging_control.pwm_frequency);
            });
        }

        void stop_frequency_sweep() {
            Time::unregister_mid_precision_alarm(sweep_action);
        }

        void toggle_HW_reset() {
            Reset_HW.toggle();
        }
    };

    class IncomingOrders {
    public:
        HeapOrder start_charging_order;
        HeapOrder stop_charging_order;
        HeapOrder set_dclv_frequency_order;
        HeapOrder set_dclv_phase_order;
        HeapOrder reset_board;
        HeapOrder start_frequency_sweep_order;
        HeapOrder stop_frequency_sweep_order;
        HeapOrder toggle_HW_reset_order;
        HeapOrder turn_on_pwm1;
        HeapOrder turn_on_pwm2;
        HeapOrder turn_on_pwm3;
        HeapOrder turn_on_pwm4;


        IncomingOrders() : start_charging_order(800, Orders::start_charging),
                           stop_charging_order(801, Orders::stop_charging),
                           set_dclv_frequency_order(802, Orders::set_dclv_frequency),
                           set_dclv_phase_order(803, Orders::set_dclv_phase),
                           reset_board(804, Orders::reset_board),
                           start_frequency_sweep_order(892, Orders::frequency_sweep),
                           stop_frequency_sweep_order(893, Orders::stop_frequency_sweep),
                           toggle_HW_reset_order(894, Orders::toggle_HW_reset),
						   turn_on_pwm1(895, Orders::start_pwm_1),
						   turn_on_pwm2(896, Orders::start_pwm_2),
						   turn_on_pwm3(897, Orders::start_pwm_3),
						   turn_on_pwm4(898, Orders::start_pwm_4)
						   {}
    };

    class TCP {
    public:
        ServerSocket backend;

        TCP() {}

        void init() {
            backend = ServerSocket(IPV4("192.168.1.8"), 50500);
        }
    };
}
