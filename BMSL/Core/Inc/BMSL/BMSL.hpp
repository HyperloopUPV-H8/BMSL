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

namespace BMSL {

    BMSA bms;
    HalfBridge dclv;
    uint8_t avionics_current_sensor;

    namespace Measurements {
        double charging_current;
        double inverter_temperature;
        double capacitor_temperature;
        double transformer_temperature;
        double rectifier_temperature;
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
    }
    namespace Packets {
struct battery_data {
            float* data[11];
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
                    (float*)&battery.is_balancing
            };
        }

        battery_data battery_info;
    }
    namespace Orders {
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
    };
    namespace Leds {
        DigitalOutput low_charge;
        DigitalOutput full_charge;
        DigitalOutput sleep;
        DigitalOutput flash;
        DigitalOutput can;
        DigitalOutput fault;
        DigitalOutput operational;
    }
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

        // sm.add_low_precision_cyclic_action([&]() {
        //     bms.update_temperatures();
        // }, ms(100), Gen::OPERATIONAL);
        
        // sm.add_mid_precision_cyclic_action([&]() {
        //     bms.update_cell_voltages();
        // }, us(5000), Gen::OPERATIONAL);

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

        op_sm.add_low_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.measure_internal_device_parameters();
        }, ms(10), Op::IDLE);

        HAL_Delay(3);

        op_sm.add_low_precision_cyclic_action([&]() {
            bms.wake_up();
            bms.read_internal_temperature();
        }, ms(10), Op::IDLE);

        HAL_Delay(3);
        
        op_sm.add_low_precision_cyclic_action([&]() {
            bms.external_adc.battery.update_data();
        }, ms(10), Op::IDLE);

        op_sm.add_state_machine(ch_sm, Op::CHARGING);


        ch_sm.add_state(Ch::CONSTANT_CURRENT);
        ch_sm.add_state(Ch::CONSTANT_VOLTAGE);
        
        ch_sm.add_enter_action( [&]() {
            dclv.set_phase(100);
        }, Ch::PRECHARGE);

        ch_sm.add_low_precision_cyclic_action([&]() {
            if (Measurements::charging_current < 1) {
                Conditions::want_to_charge = false;
            }
        }, ms(100), Ch::CONSTANT_VOLTAGE);

        ch_sm.add_mid_precision_cyclic_action([&]() {
            dclv.set_phase(dclv.get_phase() - 1);
        }, us(50), Ch::PRECHARGE);

        ch_sm.add_exit_action( [&]() {
            dclv.set_phase(0);
        }, Ch::PRECHARGE);

        ch_sm.add_transition(Ch::PRECHARGE, Ch::CONSTANT_CURRENT, [&]() {
            return dclv.get_phase() <= 0;
        });

        ch_sm.add_transition(Ch::CONSTANT_CURRENT, Ch::CONSTANT_VOLTAGE, [&]() {
            if (bms.external_adc.battery.SOC >= 0.8) {
                return true;
            }

            return false;
        });

        ch_sm.add_transition(Ch::CONSTANT_VOLTAGE, Ch::CONSTANT_CURRENT, [&]() {
            if (bms.external_adc.battery.SOC <= 0.6) {
                return true;
            }

            return false;
        });

        sm.check_transitions();
        }
    }



    namespace Orders {
        void start_balancing();
        void stop_balancing();
    }

    namespace Communications {
        DatagramSocket udp_socket;
    }


    void inscribe();
    void update();
    float get_avionics_current();

    void inscribe() {
        bms = BMSA(SPI::spi3);
        dclv = HalfBridge(DO_INV_PWM_H1, DO_INV_PWM_L1, DO_INV_PWM_H2, DO_INV_PWM_L2, GATE_DRIVERS_EN);

        Leds::can = DigitalOutput(LED_CAN);
        Leds::fault = DigitalOutput(LED_FAULT);
        Leds::flash = DigitalOutput(LED_FLASH);
        Leds::full_charge = DigitalOutput(LED_FULL_CHARGE);
        Leds::low_charge = DigitalOutput(LED_LOW_CHARGE);
        Leds::operational = DigitalOutput(LED_OPERATIONAL);
        Leds::sleep = DigitalOutput(LED_SLEEP);

        avionics_current_sensor = ADC::inscribe(AVIONICSCURRENTSENSORFW);


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
    }


    float get_avionics_current() {
        return ADC::get_value(avionics_current_sensor);
    }
    
    namespace Orders {
        void start_balancing() {
            bms.start_balancing();
        }

        void stop_balancing() {
            bms.stop_balancing();
        }
    }
};