#include "waveshare_twai_port.h"

static bool driver_installed = false; // Flag to check if the driver is installed

void setup() {
  // Start Serial 
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate
  driver_installed = waveshare_twai_init(); // Initialize the TWAI driver and store the result
}

void loop() {
  if (!driver_installed) {
    // Driver not installed 
    delay(1000); // Wait for 1 second
    return; // Exit the loop if the driver is not installed
  }
  waveshare_twai_transmit(); // Call the transmit function if the driver is installed
}
