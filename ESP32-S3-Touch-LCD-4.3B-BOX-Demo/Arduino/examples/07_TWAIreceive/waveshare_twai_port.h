#ifndef __TWAI_PORT_H
#define __TWAI_PORT_H

#pragma once

#include <Arduino.h>
#include "driver/twai.h"

// Pins used to connect to CAN bus transceiver:
#define RX_PIN 16
#define TX_PIN 15

// Intervall:
#define POLLING_RATE_MS 1000

bool waveshare_twai_init();
void waveshare_twai_receive();

#endif