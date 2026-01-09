#!/usr/bin/env python3
"""
nRF52832 QFAAB GPIO Pin Assignment for TKL Keyboard
===================================================

Package: QFN48 (6x6mm, 48 pins)
Total GPIOs: 32 (P0.00 - P0.31)
Architecture: ARM Cortex-M4F @ 64MHz
Memory: 256KB Flash, 64KB RAM

Wiring Requirements for TKL Keyboard with Azoteq Trackpad
"""

import sys
from pathlib import Path

# nRF52832 QFAAB Pin Capabilities
NRF52832_PINS = {
    # Pin  : Functions
    "P0.00": ["XL1", "GPIO"],  # 32kHz crystal X1
    "P0.01": ["XL2", "GPIO"],  # 32kHz crystal X2
    "P0.02": ["ADC1", "GPIO", "UART_CTS", "I2C_SCL"],  # ADC AIN1 - Use for VBUS sense
    "P0.03": ["ADC2", "GPIO"],  # ADC AIN2 - ROW 0
    "P0.04": ["ADC3", "GPIO"],  # ADC AIN3 - ROW 1
    "P0.05": ["ADC4", "GPIO"],  # ADC AIN4 - ROW 2
    "P0.06": ["ADC5", "GPIO"],  # ADC AIN5 - ROW 3 (also SWD trace data)
    "P0.07": ["ADC6", "GPIO"],  # ADC AIN6 - ROW 4
    "P0.08": ["ADC7", "GPIO"],  # ADC AIN7 - ROW 5
    "P0.09": ["ADC2", "GPIO"],  # ADC AIN2 duplicate - ROW 6
    "P0.10": ["ADC3", "GPIO"],  # ADC AIN3 duplicate - ROW 7
    "P0.11": ["GPIO"],          # COL 0
    "P0.12": ["GPIO"],          # COL 1
    "P0.13": ["GPIO"],          # COL 2
    "P0.14": ["GPIO"],          # COL 3
    "P0.15": ["GPIO"],          # COL 4
    "P0.16": ["GPIO"],          # COL 5
    "P0.17": ["GPIO"],          # COL 6
    "P0.18": ["GPIO"],          # COL 7
    "P0.19": ["GPIO"],          # COL 8
    "P0.20": ["GPIO"],          # COL 9
    "P0.21": ["GPIO"],          # COL 10
    "P0.22": ["GPIO"],          # COL 11
    "P0.23": ["GPIO", "UART_TX"],  # COL 12 / UART TX
    "P0.24": ["GPIO", "UART_RX"],  # COL 13 / UART RX
    "P0.25": ["GPIO"],          # COL 14
    "P0.26": ["GPIO", "I2C_SDA"],  # COL 15 / I2C SDA (for Azoteq)
    "P0.27": ["GPIO", "I2C_SCL"],  # I2C SCL (for Azoteq)
    "P0.28": ["GPIO"],          # Trackpad Interrupt
    "P0.29": ["ADC7", "GPIO"],  # Battery voltage sense (AIN7)
    "P0.30": ["GPIO"],          # LED / Status
    "P0.31": ["GPIO"],          # Reset / SWDCLK (or use dedicated SWD pins)
}

# Recommended GPIO Assignment
TKL_GPIO_ASSIGNMENT = {
    "Keyboard Matrix (8x16)": {
        "Rows": {
            "ROW0": "P0.03",
            "ROW1": "P0.04",
            "ROW2": "P0.05",
            "ROW3": "P0.06",
            "ROW4": "P0.07",
            "ROW5": "P0.08",
            "ROW6": "P0.09",
            "ROW7": "P0.10",
        },
        "Columns": {
            "COL0":  "P0.11",
            "COL1":  "P0.12",
            "COL2":  "P0.13",
            "COL3":  "P0.14",
            "COL4":  "P0.15",
            "COL5":  "P0.16",
            "COL6":  "P0.17",
            "COL7":  "P0.18",
            "COL8":  "P0.19",
            "COL9":  "P0.20",
            "COL10": "P0.21",
            "COL11": "P0.22",
            "COL12": "P0.23",
            "COL13": "P0.24",
            "COL14": "P0.25",
            "COL15": "P0.26",  # Can share with I2C SDA if careful
        },
    },
    "Peripherals": {
        "I2C (Azoteq Trackpad)": {
            "SDA": "P0.26",  # SPIM/SPIS/TWI - Use TWI0
            "SCL": "P0.27",
        },
        "UART (Debug)": {
            "TX": "P0.25",   # Alternative TX pin
            "RX": "P0.31",   # Use P0.31 if available (not in matrix)
        },
        "Power Management": {
            "VBUS_SENSE": "P0.02",   # USB power detection (ADC AIN1)
            "BATTERY_ADC": "P0.29",  # Battery voltage (ADC AIN7)
        },
        "Trackpad": {
            "INTERRUPT": "P0.28",
        },
        "Status": {
            "LED": "P0.30",
        },
        "Debug": {
            "SWDIO": "SWDIO",  # Dedicated SWD pin (not in GPIO numbering)
            "SWDCLK": "SWCLK", # Dedicated SWD pin
        },
    },
    "Reserved / Not Recommended": {
        "Crystal": ["P0.00", "P0.01"],  # 32kHz crystal for RTC
        "Reset": "RESET",  # Dedicated reset pin
    },
}

# Hardware Connections
HARDWARE_CONNECTIONS = """
Hardware Wiring for nRF52832 TKL Keyboard
==========================================

POWER SUPPLY
------------
VDD  -> 3.3V (from DC/DC regulator or LDO)
VSS  -> GND
VDDH -> Battery/USB input (for DC/DC mode)

DC/DC Converter Requirements:
- Input: 3.0V - 5.5V (from USB LiPo charger or battery)
- Output: 3.3V, capable of 50mA+ (TX at 0dBm)
- Inductor: 4.7µH - 10µH (follow nRF52832 PS)
- Decoupling: 0.1µF ceramic near each VDD pin
- Bulk capacitance: 4.7µF - 10µF

CRYSTAL OSCILLATOR
------------------
32.768 kHz Crystal:
- P0.00 (XL1) -> Crystal pin 1
- P0.01 (XL2) -> Crystal pin 2
- Load capacitors: 2x 12pF (or per crystal spec)

32 MHz High-Speed Oscillator:
- Internal, no external crystal needed for most apps

USB (Type-C)
------------
D+ -> P0.30 (through USB D+ pin, varies by package)
D- -> P0.31 (through USB D- pin, varies by package)
VBUS -> P0.02 (for sensing, through divider)

Type-C CC pins:
- CC1, CC2 -> 5.1k resistors to GND
- Use USB PD controller or simple resistive detection

KEYBOARD MATRIX (8x16)
----------------------
Rows (8 pins): P0.03 - P0.10
Cols (16 pins): P0.11 - P0.21, P0.22 - P0.25, P0.26

Diode direction: Column to Row (col2row in ZMK)
Diode type: 1N4148 or equivalent

AZOTEQ TRACKPAD (I2C)
---------------------
I2C Bus: TWI0 (nRF52832)
- SDA -> P0.26 (with 4.7k pull-up to 3.3V)
- SCL -> P0.27 (with 4.7k pull-up to 3.3V)
- INT -> P0.28 (optional, for interrupt-driven mode)
- VDD -> 3.3V
- GND -> GND

I2C Address: Typically 0x74 (check datasheet)

BATTERY MONITORING
------------------
ADC input: P0.29 (AIN7)
- Voltage divider: Battery+ -> 100k -> AIN7 -> 200k -> GND
- Capacitor: 1µF from AIN7 to GND (filtering)

UART (Optional, for debugging)
-------------------------------
UART0:
- TX -> P0.23
- RX -> P0.24
- Baud: 115200 (typical)
- Level shifter to 3.3V if connecting to 5V system

SWD DEBUG INTERFACE
-------------------
SWDIO -> SWDIO pin (dedicated)
SWCLK -> SWCLK pin (dedicated)
GND   -> GND
VDD   -> 3.3V (optional, for powering from debugger)

STATUS LED
----------
LED -> P0.30 (with current-limiting resistor)
- Forward current: 2-20mA typical
- Resistor: (3.3V - Vf) / If
- Example: (3.3 - 2.0) / 0.01 = 130Ω

ANTENNA
-------
For BLE (2.4 GHz):
- PCB antenna: 50Ω trace to antenna pad
- Chip antenna: Follow manufacturer layout
- Keep area clear of ground plane

PCB LAYOUT GUIDELINES
---------------------
1. RF Section:
   - Keep antenna area clear
   - 50Ω impedance matched trace
   - No ground plane under antenna
   - Crystal away from RF section

2. Power Supply:
   - Short, wide traces for VDD/GND
   - Decoupling capacitors close to pins
   - Star ground for GND connections

3. I2C:
   - Pull-ups near nRF52832
   - Keep traces short

4. Keyboard Matrix:
   - Diodes oriented correctly
   - No crossed traces if possible
   - Columns with pull-downs

TESTING CHECKLIST
-----------------
[ ] Power supply voltage (3.3V)
[ ] Crystal oscillation (32.768 kHz)
[ ] USB enumeration
[ ] SWD programming
[ ] Matrix scan (all keys)
[ ] I2C communication (trackpad)
[ ] Battery voltage reading
[ ] BLE advertising
[ ] BLE pairing and data

ZMK CONFIGURATION
-----------------
Enable in Kconfig.defconfig:
- CONFIG_ZMK_USB=y
- CONFIG_ZMK_BLE=y
- CONFIG_ZMK_POINTING=y (for trackpad)
- CONFIG_I2C=y
- CONFIG_ADC=y

Add to devicetree:
- I2C node for TWI0
- ADC node for battery
- Pointing device node for trackpad
"""

def print_assignment():
    """Print the GPIO assignment summary."""
    print("=" * 80)
    print("nRF52832 QFAAB GPIO Pin Assignment - TKL Keyboard + Azoteq Trackpad")
    print("=" * 80)

    print("\n📌 SUMMARY")
    print("-" * 80)
    total_matrix = 8 + 16  # rows + cols
    total_peripherals = 2 + 2 + 2 + 1 + 1 + 1  # I2C + UART + Power + INT + LED + SWD
    total_used = total_matrix + total_peripherals
    print(f"Total GPIOs available: 32 (P0.00 - P0.31)")
    print(f"Matrix pins: {total_matrix} (8 rows + 16 columns)")
    print(f"Peripheral pins: {total_peripherals}")
    print(f"Total used: {total_used}")
    print(f"Available: {32 - total_used}")

    print("\n🔌 MATRIX PINS (8x16 = 128 positions)")
    print("-" * 80)
    print("ROWS (8 pins):")
    for row, pin in TKL_GPIO_ASSIGNMENT["Keyboard Matrix (8x16)"]["Rows"].items():
        print(f"  {row:6s} -> {pin}")
    print("\nCOLUMNS (16 pins):")
    for col, pin in TKL_GPIO_ASSIGNMENT["Keyboard Matrix (8x16)"]["Columns"].items():
        print(f"  {col:6s} -> {pin}")

    print("\n🔧 PERIPHERAL PINS")
    print("-" * 80)
    for periph, pins in TKL_GPIO_ASSIGNMENT["Peripherals"].items():
        print(f"\n{periph}:")
        for name, pin in pins.items():
            print(f"  {name:12s} -> {pin}")

    print("\n⚠️  RESERVED PINS")
    print("-" * 80)
    for periph, pins in TKL_GPIO_ASSIGNMENT["Reserved / Not Recommended"].items():
        print(f"{periph}: {pins}")

    print("\n📋 DTS FRAGMENT")
    print("-" * 80)
    print("""
/* nRF52832 TKL Keyboard with Azoteq Trackpad */
#include <dt-bindings/zmk/matrix_transform.h>

/ {
    chosen {
        zmk,kscan = &kscan;
        zmk,matrix_transform = &transform;
    };

    kscan: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        diode-direction = "col2row";

        row-gpios =
            <&gpio0 3 GPIO_ACTIVE_HIGH>,   /* ROW0 */
            <&gpio0 4 GPIO_ACTIVE_HIGH>,   /* ROW1 */
            <&gpio0 5 GPIO_ACTIVE_HIGH>,   /* ROW2 */
            <&gpio0 6 GPIO_ACTIVE_HIGH>,   /* ROW3 */
            <&gpio0 7 GPIO_ACTIVE_HIGH>,   /* ROW4 */
            <&gpio0 8 GPIO_ACTIVE_HIGH>,   /* ROW5 */
            <&gpio0 9 GPIO_ACTIVE_HIGH>,   /* ROW6 */
            <&gpio0 10 GPIO_ACTIVE_HIGH>;  /* ROW7 */

        col-gpios =
            <&gpio0 11 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL0 */
            <&gpio0 12 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL1 */
            <&gpio0 13 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL2 */
            <&gpio0 14 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL3 */
            <&gpio0 15 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL4 */
            <&gpio0 16 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL5 */
            <&gpio0 17 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL6 */
            <&gpio0 18 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL7 */
            <&gpio0 19 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL8 */
            <&gpio0 20 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL9 */
            <&gpio0 21 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL10 */
            <&gpio0 22 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL11 */
            <&gpio0 23 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL12 */
            <&gpio0 24 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL13 */
            <&gpio0 25 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>,  /* COL14 */
            <&gpio0 26 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;  /* COL15 */
    };
};

/* I2C for Azoteq Trackpad */
&i2c0 {
    status = "okay";
    sda-pin = <26>;
    scl-pin = <27>;

    azoteq_trackpad: trackpad@74 {
        compatible = "azoteq,iqs550";
        reg = <0x74>;
    };
};
""")

    print("\n📖 HARDWARE CONNECTIONS")
    print("-" * 80)
    print(HARDWARE_CONNECTIONS)

if __name__ == "__main__":
    print_assignment()
