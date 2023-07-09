#include "BMS-LIB.hpp"

namespace BMSL {
    namespace Protections {
        void inscribe() {
            // add_protection(&BMSL::Measurements::charging_current, Boundary<double, ABOVE>(300));

            // Battery& battery = BMSL::bms.external_adc.battery;
            // for (float* cell : battery.cells) {
            // 	add_protection(cell, Boundary<float, ProtectionType::BELOW>(3.3));
            // 	add_protection(cell, Boundary<float, ProtectionType::ABOVE>(4.2));
            // }

            // float* fp = &battery.total_voltage;
            // Boundary<float, ProtectionType::BELOW> b(1000);
            // add_protection(fp, b);

            // fp = &battery.disbalance;
            // Boundary<float, ProtectionType::ABOVE> b2(Battery::MAX_DISBALANCE);
            // add_protection(fp, b2);
            
            // fp = &battery.total_voltage;
            // Boundary<float, ProtectionType::ABOVE> b3(19.8);
            // add_protection(fp, b3);

            // add_protection(battery.temperature1, Boundary<float, ProtectionType::ABOVE>(70));
            // add_protection(battery.temperature2, Boundary<float, ProtectionType::ABOVE>(70));



        }
    }
}