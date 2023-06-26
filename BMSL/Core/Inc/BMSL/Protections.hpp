#include "BMS-LIB.hpp"

namespace BMSL {
    namespace Protections {
        void inscribe() {
            //add_protection(&BMSL::Measurements::charging_current, Boundary<double, ABOVE>(300));

            Battery& battery = BMSL::bms.external_adc.battery;
            // for (float* cell : battery.cells) {
            // 	add_protection(cell, Boundary<float, ProtectionType::BELOW>(3.3));
            // 	add_protection(cell, Boundary<float, ProtectionType::ABOVE>(4.2));
            // }

            // add_protection(battery.temperature1, Boundary<float, ProtectionType::ABOVE>(70));
            // add_protection(battery.temperature2, Boundary<float, ProtectionType::ABOVE>(70));
            // add_protection(&battery.disbalance, Boundary<float, ProtectionType::ABOVE>(Battery::MAX_DISBALANCE));
            // add_protection(&battery.total_voltage, Boundary<float, ProtectionType::BELOW>(19.8));
        }
    }
}