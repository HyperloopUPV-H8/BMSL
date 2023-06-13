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

    namespace ChargeControl {
        double control_frequency = 5000;
        double pwm_frequency = 100000;
        PI<IntegratorType::Trapezoidal> charger_pi(-20, -1, 0.0002);
        HalfBridge dclv;
    }


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

        enum Charging {
            PRECHARGE = 100,
            CONSTANT_CURRENT = 101,
            CONSTANT_VOLTAGE = 102
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

        void open_contactors() {

        };

        void close_contactors() {

        };

        void start_all_pwm() {
            ChargeControl::dclv.turn_on();
            Leds::fault.turn_on();
        }

        void stop_all_pwm() {
            ChargeControl::dclv.turn_off();
            Leds::fault.turn_off();
        }


        void set_pwm_phase() {
        }

        void set_pwm_frequency() {
        }

        void set_dclv_frequency() {
            ChargeControl::dclv.set_frequency(selected_number);
        }

        void set_dclv_duty_cycle() {
            ChargeControl::dclv.set_duty_cycle(selected_number);
        }

        void set_dclv_phase() {
            ChargeControl::dclv.set_phase(selected_number);
        }

        void set_pwm_duty_cycle(uint8_t duty_cycle) {
            
        }

        void turn_on_pwm() {
            if (selected_pwm == 0) {
                ChargeControl::dclv.positive_pwm.turn_on_positive();
                ChargeControl::dclv.positive_pwm.set_duty_cycle(50);
            } else if (selected_pwm == 1) {
                ChargeControl::dclv.positive_pwm.turn_on_negated();
                ChargeControl::dclv.positive_pwm.set_duty_cycle(50);
            } else if (selected_pwm == 2) {
                ChargeControl::dclv.negative_pwm.turn_on_positive();
                ChargeControl::dclv.negative_pwm.set_duty_cycle(50);
            } else if (selected_pwm == 3) {
                ChargeControl::dclv.negative_pwm.turn_on_negated();
                ChargeControl::dclv.negative_pwm.set_duty_cycle(50);
            }
        }
    };

    namespace StateMachines {
        StateMachine general;
        StateMachine operational;
        StateMachine charging;

        void start() {
        StateMachine& sm = StateMachines::general;
        StateMachine& op_sm = StateMachines::operational;
        StateMachine& ch_sm = StateMachines::charging;

        using Gen = States::General;
        using Op = States::Operational;
        using Ch = States::Charging;

        sm.add_state(Gen::OPERATIONAL);
        sm.add_state(Gen::FAULT);

        sm.add_transition(Gen::CONNECTING, Gen::OPERATIONAL, [&]() {
            return Conditions::ready;
        });

        sm.add_transition(Gen::OPERATIONAL, Gen::FAULT, [&]() {
            return Conditions::fault;
        });

        sm.add_transition(Gen::CONNECTING, Gen::FAULT, [&]() {
            return Conditions::fault;
        });

        sm.add_low_precision_cyclic_action([&]() {
            Leds::operational.toggle();
        }, ms(200), Gen::CONNECTING);

        sm.add_mid_precision_cyclic_action([&]() {
            Sensors::avionics_current.read();
            Sensors::input_charging_current.read();
            Sensors::output_charging_current.read();
            Sensors::input_charging_voltage.read();
            Sensors::output_charging_voltage.read();
        }, ms(200), Gen::OPERATIONAL);

        sm.add_enter_action([&]() {
            Leds::operational.turn_on();
        }, Gen::OPERATIONAL);

        sm.add_enter_action([&]() {
            Leds::fault.turn_on();
        }, Gen::FAULT);

        sm.add_exit_action([&]() {
            Leds::operational.turn_off();
        }, Gen::OPERATIONAL);

        sm.add_exit_action([&]() {
            Leds::fault.turn_off();
        }, Gen::FAULT);

        sm.add_state_machine(op_sm, Gen::OPERATIONAL);

        op_sm.add_state(Op::CHARGING);
        op_sm.add_state(Op::BALANCING);

        op_sm.add_transition(Op::IDLE, Op::CHARGING, [&]() {
            return Conditions::want_to_charge;
        });

        op_sm.add_transition(Op::CHARGING, Op::IDLE, [&]() {
            return not Conditions::want_to_charge;
        });

        op_sm.add_transition(Op::CHARGING, Op::BALANCING, [&]() {
            if (bms.external_adc.battery.needs_balance()) {
                return true;
            }
            return false;
        });

        op_sm.add_transition(Op::BALANCING, Op::CHARGING, [&]() {
            if (bms.external_adc.battery.needs_balance()) {
                return false;
            }
            return true;
        });

        op_sm.add_transition(Op::BALANCING, Op::IDLE, [&]() {
            return not Conditions::want_to_charge;
        });

        op_sm.add_mid_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.start_adc_conversion_all_cells();
        }, us(3000), Op::IDLE);

        HAL_Delay(2);

        op_sm.add_mid_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.read_cell_voltages();
        }, us(3000), Op::IDLE);

        op_sm.add_mid_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.measure_internal_device_parameters();
        }, ms(5), Op::IDLE);

        HAL_Delay(3);

        op_sm.add_mid_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.read_internal_temperature();
        }, ms(5), Op::IDLE);

        HAL_Delay(3);
        
        op_sm.add_low_precision_cyclic_action([&]() {
            bms.external_adc.battery.update_data();
        }, ms(100), Op::IDLE);

        HAL_Delay(3);

        op_sm.add_low_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.start_adc_conversion_gpio();
        }, us(3000), Op::IDLE);

        HAL_Delay(2);

        op_sm.add_low_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.read_temperatures();
        }, us(3000), Op::IDLE);
    
        op_sm.add_mid_precision_cyclic_action([&]() {
            Sensors::input_charging_current.read();
            ChargeControl::charger_pi.input(CURRENT_SETPOINT - Measurements::input_charging_current);
            ChargeControl::charger_pi.execute();
            ChargeControl::pwm_frequency += ChargeControl::charger_pi.output_value;
            //ChargeControl::dclv.set_frequency(ChargeControl::pwm_frequency);
        }, us(200), Op::CHARGING);
        op_sm.add_state_machine(ch_sm, Op::CHARGING);


        ch_sm.add_state(Ch::CONSTANT_CURRENT);
        ch_sm.add_state(Ch::CONSTANT_VOLTAGE);
        
        ch_sm.add_enter_action( [&]() {
            ChargeControl::pwm_frequency = 100000;
            ChargeControl::dclv.set_phase(100);
            ChargeControl::dclv.set_duty_cycle(50);
            ChargeControl::dclv.set_frequency(150000);
            ChargeControl::charger_pi.reset();
            ChargeControl::dclv.turn_on();


        }, Ch::PRECHARGE);

        op_sm.add_enter_action([&]() {
            Leds::low_charge.turn_on();
            BMSL::Conditions::charging = true;
        }, Op::CHARGING);

        op_sm.add_exit_action([&]() {
            Leds::low_charge.turn_off();
            BMSL::Conditions::charging = false;
        }, Op::CHARGING);

        ch_sm.add_low_precision_cyclic_action([&]() {
            if (Measurements::input_charging_current < 1) {
                Conditions::want_to_charge = false;
            }
        }, ms(100), Ch::CONSTANT_VOLTAGE);

        ch_sm.add_low_precision_cyclic_action([&]() {
            if (ChargeControl::dclv.get_phase() > 15) {
                ChargeControl::dclv.set_phase(ChargeControl::dclv.get_phase() - 1);
            }
        }, ms(100), Ch::PRECHARGE);

        ch_sm.add_exit_action( [&]() {
            ChargeControl::dclv.set_phase(15);
        }, Ch::PRECHARGE);

        ch_sm.add_transition(Ch::PRECHARGE, Ch::CONSTANT_CURRENT, [&]() {
            return ChargeControl::dclv.get_phase() <= 16;
        });

        ch_sm.add_transition(Ch::CONSTANT_CURRENT, Ch::CONSTANT_VOLTAGE, [&]() {
            if (bms.external_adc.battery.SOC >= 80) {
                return true;
            }

            return false;
        });

        ch_sm.add_transition(Ch::CONSTANT_VOLTAGE, Ch::CONSTANT_CURRENT, [&]() {
            if (bms.external_adc.battery.SOC <= 60) {
                return true;
            }

            return false;
        });

        sm.check_transitions();
        }
    }

    namespace Communications {
        DatagramSocket udp_socket;
    }


    void inscribe();
    void update();

    void inscribe() {
        bms = BMSA(SPI::spi3);
        ChargeControl::dclv = HalfBridge(DO_INV_PWM_H1, DO_INV_PWM_L1, DO_INV_PWM_H2, DO_INV_PWM_L2, BUFFER_EN);

        Leds::can = DigitalOutput(LED_CAN);
        Leds::fault = DigitalOutput(LED_FAULT);
        Leds::flash = DigitalOutput(LED_FLASH);
        Leds::full_charge = DigitalOutput(LED_FULL_CHARGE);
        Leds::low_charge = DigitalOutput(LED_LOW_CHARGE);
        Leds::operational = DigitalOutput(LED_OPERATIONAL);
        Leds::sleep = DigitalOutput(LED_SLEEP);

        Sensors::avionics_current = LinearSensor<double>(AVIONICSCURRENTSENSORFW, 2.7, -0.14, &Measurements::avionics_current);
        Sensors::input_charging_current = LinearSensor<double>(INPUTCURRENTFW, 2.7, -0.14, &Measurements::input_charging_current);
        Sensors::output_charging_current = LinearSensor<double>(OUTPUTCURRENTFW, 2.7, -0.14, &Measurements::output_charging_current);
        Sensors::input_charging_voltage = LinearSensor<double>(INPUTVOLTAGEFW, 91.74, -4.05, &Measurements::input_charging_voltage);
        Sensors::output_charging_voltage = LinearSensor<double>(OUTPUTVOLTAGEFW, 8.86, -0.42, &Measurements::output_charging_voltage);

        StateMachines::general = StateMachine(States::General::CONNECTING);
        StateMachines::operational = StateMachine(States::Operational::IDLE);
        StateMachines::charging = StateMachine(States::Charging::PRECHARGE);
    }

    void start() {
        STLIB::start("192.168.1.8");
        bms.initialize();
        StateMachines::start();  

        Packets::battery_info = Packets::serialize_battery(bms.external_adc.battery);

        Conditions::ready = true;
        StateMachines::general.check_transitions();    
    }

    void update() {
        STLIB::update();
        StateMachines::general.check_transitions();
    }
};