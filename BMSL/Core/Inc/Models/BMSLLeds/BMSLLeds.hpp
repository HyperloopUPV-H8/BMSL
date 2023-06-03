#include "BMS-LIB.hpp"

class BMSLLeds {
public:
    DigitalOutput low_charge;
    DigitalOutput full_charge;
    DigitalOutput sleep;
    DigitalOutput flash;
    DigitalOutput can;
    DigitalOutput fault;
    DigitalOutput operational;
    BMSLLeds() = default;
    BMSLLeds(
        Pin& low_charge,
        Pin& full_charge,
        Pin& sleep,
        Pin& flash,
        Pin& can,
        Pin& fault,
        Pin& operational
    );
};

BMSLLeds::BMSLLeds(Pin& low_charge, Pin& full_charge, Pin& sleep, Pin& flash, Pin& can, Pin& fault, Pin& operational) {
    this->low_charge = DigitalOutput(low_charge);
    this->full_charge = DigitalOutput(full_charge);
    this->sleep = DigitalOutput(sleep);
    this->flash = DigitalOutput(flash);
    this->can = DigitalOutput(can);
    this->fault = DigitalOutput(fault);
    this->operational = DigitalOutput(operational);
}
