#include "waveshare_twai_port.h"

static bool driver_installed = false; // Flag to check if the driver is installed

void setup() {
  // Start Serial:
  Serial.begin(115200);// Flag to check if the driver is installed
  driver_installed = waveshare_twai_init(); // Flag to check if the driver is installed
}

void loop() {
  if (!driver_installed) {
    // Driver not installed
    delay(1000);
    return;
  }
  waveshare_twai_receive();// Call the receive function if the driver is installed
}
