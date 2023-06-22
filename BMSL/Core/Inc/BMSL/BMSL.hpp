#include "BMS-LIB.hpp"

#define ACTIVE_DISCHARGE PE2
#define LAN_INT PC13
#define LSE_IN PC14
#define LSE_OUT PC15
#define BUFFER_EN PF4
#define MDC PC1
#define AVIONICSCURRENTSENSORFW PA0
#define MDIO PA2
#define TEMPINVERTERFW PA3
#define TEMPCAPACITORFW PA4
#define TEMPTRANSFORMERFW PA5
#define TEMPRECTIFIERFW PA6
#define CRS_DV PA7
#define RXD0 PC4
#define RXD1 PC5
#define INPUTVOLTAGEFW PB0
#define INPUTCURRENTFW PB1
#define OUTPUTVOLTAGEFW PF11
#define OUTPUTCURRENTFW PF12
#define PHY_RST PG0
#define DO_INV_PWM_L1 PE8
#define DO_INV_PWM_H1 PE9
#define DO_INV_PWM_L2 PE12
#define DO_INV_PWM_H2 PE13
#define GATE_DRIVERS_EN PE15
#define TXD1 PB13
#define LED_LOW_CHARGE PG2
#define LED_FULL_CHARGE PG3
#define LED_SLEEP PG4
#define LED_FLASH PG5
#define LED_CAN PG6
#define LED_FAULT PG7
#define LED_OPERATIONAL PG8
#define SLNT PA8
#define PGOOD_12V PA11
#define PGOOD_16V PA12
#define SWDIO PA13
#define SWCLK PA14
#define JTDI PA15
#define SPI_CLK PC10
#define SPI_MISO PC11
#define SPI_MOSI PC12
#define CAN1_RX PD0
#define CAN1_TX PD1
#define SPI_CS PD3
#define UART_TX PD5
#define UART_RX PD6
#define TXEN PG11
#define TXD0 PG13
#define SWO PB3
#define HW_FAULT PE0
#define RX_ER PE1

#define CURRENT_SETPOINT 5

namespace BMSL {

    BMSA bms;
    DigitalOutput Reset_HW;
    uint8_t sweep_action;
    ChargingControl charging_control;

    namespace Measurements {
        double avionics_current;
        double input_charging_current;
        double output_charging_current;
        double input_charging_voltage;
        double output_charging_voltage;
        double inverter_temperature;
        double capacitor_temperature;
        double transformer_temperature;
        double rectifier_temperature;
        double pwm_frequency = 37;
    }  
    namespace Sensors {
        LinearSensor<double> avionics_current;
        LinearSensor<double> input_charging_current;
        LinearSensor<double> output_charging_current;
        LinearSensor<double> input_charging_voltage;
        LinearSensor<double> output_charging_voltage;
    }
    namespace States {
        enum General : uint8_t {
            CONNECTING = 0,
            OPERATIONAL = 1,
            FAULT = 2
        };

        enum Operational {
            CHARGING = 10,
            BALANCING = 11,
            IDLE = 12
        };
    }
    namespace Conditions {
        bool ready = false;
        bool want_to_charge = false;
        bool fault = false;
        bool charging = false;
    }

    namespace Leds {
        DigitalOutput low_charge;
        DigitalOutput full_charge;
        DigitalOutput sleep;
        DigitalOutput flash;
        DigitalOutput can;
        DigitalOutput fault;
        DigitalOutput operational;
    }

    namespace Packets {
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
    }
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
            charging_control.dclv.set_duty_cycle(50);
            charging_control.charger_pi.reset();
            charging_control.dclv.turn_on();
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
    };

    namespace StateMachines {
        StateMachine general;
        StateMachine operational;
        StateMachine charging;

        void setup_states() {
            using Gen = States::General;
            using Op = States::Operational;

            general.add_state(Gen::OPERATIONAL);
            general.add_state(Gen::FAULT);
            general.add_state_machine(operational, Gen::OPERATIONAL);
            
            operational.add_state(Op::CHARGING);
            operational.add_state(Op::BALANCING);
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

            operational.add_transition(Op::CHARGING, Op::BALANCING, [&]() {
                if (bms.external_adc.battery.needs_balance()) {
                    return true;
                }
                return false;
            });

            operational.add_transition(Op::BALANCING, Op::CHARGING, [&]() {
                if (bms.external_adc.battery.needs_balance()) {
                    return false;
                }
                return true;
            });

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
            }, ms(200), Gen::OPERATIONAL);

            general.add_mid_precision_cyclic_action([&]() {
                ProtectionManager::check_protections();
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

            operational.add_mid_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.start_adc_conversion_all_cells();

                Time::set_timeout(3, [&]() {
                    bms.wake_up();
                    bms.read_cell_voltages();
                });
            }, ms(10), Op::IDLE);

            HAL_Delay(3);

            operational.add_mid_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.measure_internal_device_parameters();

                Time::set_timeout(5, [&]() {
                    bms.wake_up();
                    bms.read_internal_temperature();
                });
            }, ms(5), Op::IDLE);

            HAL_Delay(3);

            operational.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.start_adc_conversion_gpio();

                Time::set_timeout(2, [&]() {
                    bms.wake_up();
                    bms.read_temperatures();
                });
            }, ms(10), Op::IDLE);

            operational.add_low_precision_cyclic_action([&]() {
                bms.external_adc.battery.update_data();
            }, ms(100), Op::IDLE);
        
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

    namespace Communications {
        DatagramSocket udp_socket;
    }


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
        bms.initialize();
        StateMachines::start(); 

        Packets::battery_info = Packets::serialize_battery(bms.external_adc.battery);
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