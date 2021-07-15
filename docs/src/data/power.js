/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * This file holds all current measurements related to ZMK features and hardware
 * All current measurements are in micro amps. Measurements were taken on a Nordic Power Profiler Kit
 * The test device to get these values was three nice!nanos (nRF52840).
 */

export const zmkBase = {
  hostConnection: 23, // How much current it takes to have an idle host connection
  standalone: {
    idle: 0, // No extra idle current
    typing: 315, // Current while holding down a key. Represents polling+BLE notification power
  },
  central: {
    idle: 490, // Idle current for connection to right half
    typing: 380, // Current while holding down a key. Represents polling+BLE notification power
  },
  peripheral: {
    idle: 20, // Idle current for connection to left half
    typing: 365, // Current while holding down a key. Represents polling+BLE notification power
  },
};

/**
 * ZMK board power measurements
 *
 * Power supply can be an LDO or switching
 * Quiescent and other quiescent are measured in micro amps
 *
 * Switching efficiency represents the efficiency of converting from
 * 3.8V (average li-ion voltage) to the output voltage of the power supply
 */
export const zmkBoards = {
  "nice!nano": {
    name: "nice!nano v1",
    powerSupply: {
      type: "LDO",
      outputVoltage: 3.3,
      quiescentMicroA: 55,
    },
    otherQuiescentMicroA: 4,
  },
  "nice!nano v2": {
    name: "nice!nano v2",
    powerSupply: {
      type: "LDO",
      outputVoltage: 3.3,
      quiescentMicroA: 15,
    },
    otherQuiescentMicroA: 3,
  },
  "nice!60": {
    powerSupply: {
      type: "SWITCHING",
      outputVoltage: 3.3,
      efficiency: 0.95,
      quiescentMicroA: 4,
    },
    otherQuiescentMicroA: 4,
  },
};

export const underglowPower = {
  firmware: 60, // ZMK power usage while underglow feature is turned on (SPIM mostly)
  ledOn: 20000, // Estimated power consumption of a WS2812B at 100% (can be anywhere from 10mA to 30mA)
  ledOff: 460, // Quiescent current of a WS2812B
};

export const displayPower = {
  // Based on GoodDisplay's 1.02in epaper
  EPAPER: {
    activePercent: 0.05, // Estimated one refresh per minute taking three seconds
    active: 1500, // Power draw during refresh
    sleep: 5, // Idle power draw of an epaper
  },
  // 128x32 SSD1306
  OLED: {
    activePercent: 0.5, // Estimated sleeping half the time (based on idle)
    active: 10000, // Estimated power draw when about half the pixels are on
    sleep: 7, // Deep sleep power draw (display off)
  },
};
