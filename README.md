# Modbus slave library for mbed
Uses async serial implementation. Currently only FC3 - Read Holding Registers supported

```cpp
#include <mbed.h>
#include "ModbusSlave.h"

Serial mdbSerial(PD_8, PD_9);
// register size=125, serial object, DE pin, RE pin
ModbusSlave<125> modbus(&mdbSerial, PD_15, PE_5);

#define SLAVE_ID 0x02

int main() {
  modbus.init(SLAVE_ID, 9600); // 9600 baud
  uint16_t counter = 0;

  while (1){
    modbus.write(0, counter); // write to register 0
    counter++;
    Thread::wait(500);
  }
}

```