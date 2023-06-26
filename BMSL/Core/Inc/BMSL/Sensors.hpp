#pragma once

#include "BMS-LIB.hpp"
#include "BMSL/Measurements.hpp"
#include "Pins.hpp"

namespace BMSL {
    namespace Sensors {
        LinearSensor<double> avionics_current;
        LinearSensor<double> input_charging_current;
        LinearSensor<double> output_charging_current;
        LinearSensor<double> input_charging_voltage;
        LinearSensor<double> output_charging_voltage;
        
        void inscribe() {
        Sensors::avionics_current = LinearSensor<double>(AVIONICSCURRENTSENSORFW, 2.7, -0.14, &Measurements::avionics_current);
        Sensors::input_charging_current = LinearSensor<double>(INPUTCURRENTFW, 3.78, -0.14, &Measurements::input_charging_current);
        Sensors::output_charging_current = LinearSensor<double>(OUTPUTCURRENTFW, 10.4, -0.14, &Measurements::output_charging_current);
        Sensors::input_charging_voltage = LinearSensor<double>(INPUTVOLTAGEFW, 91.74, -4.05, &Measurements::input_charging_voltage);
        Sensors::output_charging_voltage = LinearSensor<double>(OUTPUTVOLTAGEFW, 8.86, -0.42, &Measurements::output_charging_voltage);

        }
    };


}