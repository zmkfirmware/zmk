/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Based on HID Usage Tables 1.21,
 * Copyright © 1996-2020, USB Implementers Forum,
 * https://www.usb.org/sites/default/files/hut1_21.pdf
 */

#pragma once

#define HID_USAGE(page, id) ((page << 16) | id)
#define HID_USAGE_ID(usage) (usage & 0xFFFF)
#define HID_USAGE_PAGE(usage) (usage >> 16)

/* WARNING: DEPRECATED from dt-bindings/zmk/keys.h */
#define USAGE_KEYPAD (0x07)   // WARNING: DEPRECATED (DO NOT USE)
#define USAGE_CONSUMER (0x0C) // WARNING: DEPRECATED (DO NOT USE)

/* Generic Desktop Page */
#define HID_USAGE_GD (0x01)

/* Generic Desktop Page: Undefined */
#define HID_USAGE_GD_UNDEFINED (HID_USAGE(HID_USAGE_GD, 0x00))

/* Generic Desktop Page: Pointer [CP] */
#define HID_USAGE_GD_POINTER (HID_USAGE(HID_USAGE_GD, 0x01))

/* Generic Desktop Page: Mouse [CA] */
#define HID_USAGE_GD_MOUSE (HID_USAGE(HID_USAGE_GD, 0x02))

/* Generic Desktop Page: Joystick [CA] */
#define HID_USAGE_GD_JOYSTICK (HID_USAGE(HID_USAGE_GD, 0x04))

/* Generic Desktop Page: Gamepad [CA] */
#define HID_USAGE_GD_GAMEPAD (HID_USAGE(HID_USAGE_GD, 0x05))

/* Generic Desktop Page: Keyboard [CA] */
#define HID_USAGE_GD_KEYBOARD (HID_USAGE(HID_USAGE_GD, 0x06))

/* Generic Desktop Page: Keypad [CA] */
#define HID_USAGE_GD_KEYPAD (HID_USAGE(HID_USAGE_GD, 0x07))

/* Generic Desktop Page: Multi-axis Controller [CA] */
#define HID_USAGE_GD_MULTI_AXIS_CONTROLLER (HID_USAGE(HID_USAGE_GD, 0x08))

/* Generic Desktop Page: Tablet PC System Controls [CA] */
#define HID_USAGE_GD_TABLET_PC_SYSTEM_CONTROLS (HID_USAGE(HID_USAGE_GD, 0x09))

/* Generic Desktop Page: Water Cooling Device [CA] */
#define HID_USAGE_GD_WATER_COOLING_DEVICE (HID_USAGE(HID_USAGE_GD, 0x0A))

/* Generic Desktop Page: Computer Chassis Device [CA] */
#define HID_USAGE_GD_COMPUTER_CHASSIS_DEVICE (HID_USAGE(HID_USAGE_GD, 0x0B))

/* Generic Desktop Page: Wireless Radio Controls [CA] */
#define HID_USAGE_GD_WIRELESS_RADIO_CONTROLS (HID_USAGE(HID_USAGE_GD, 0x0C))

/* Generic Desktop Page: Portable Device Control [CA] */
#define HID_USAGE_GD_PORTABLE_DEVICE_CONTROL (HID_USAGE(HID_USAGE_GD, 0x0D))

/* Generic Desktop Page: System Multi-Axis Controller [CA] */
#define HID_USAGE_GD_SYSTEM_MULTI_AXIS_CONTROLLER (HID_USAGE(HID_USAGE_GD, 0x0E))

/* Generic Desktop Page: Spatial Controller [CA] */
#define HID_USAGE_GD_SPATIAL_CONTROLLER (HID_USAGE(HID_USAGE_GD, 0x0F))

/* Generic Desktop Page: Assistive Control [CA] */
#define HID_USAGE_GD_ASSISTIVE_CONTROL (HID_USAGE(HID_USAGE_GD, 0x10))

/* Generic Desktop Page: Device Dock [CA] */
#define HID_USAGE_GD_DEVICE_DOCK (HID_USAGE(HID_USAGE_GD, 0x11))

/* Generic Desktop Page: Dockable Device [CA] */
#define HID_USAGE_GD_DOCKABLE_DEVICE (HID_USAGE(HID_USAGE_GD, 0x12))

/* Generic Desktop Page: X [DV] */
#define HID_USAGE_GD_X (HID_USAGE(HID_USAGE_GD, 0x30))

/* Generic Desktop Page: Y [DV] */
#define HID_USAGE_GD_Y (HID_USAGE(HID_USAGE_GD, 0x31))

/* Generic Desktop Page: Z [DV] */
#define HID_USAGE_GD_Z (HID_USAGE(HID_USAGE_GD, 0x32))

/* Generic Desktop Page: Rx [DV] */
#define HID_USAGE_GD_RX (HID_USAGE(HID_USAGE_GD, 0x33))

/* Generic Desktop Page: Ry [DV] */
#define HID_USAGE_GD_RY (HID_USAGE(HID_USAGE_GD, 0x34))

/* Generic Desktop Page: Rz [DV] */
#define HID_USAGE_GD_RZ (HID_USAGE(HID_USAGE_GD, 0x35))

/* Generic Desktop Page: Slider [DV] */
#define HID_USAGE_GD_SLIDER (HID_USAGE(HID_USAGE_GD, 0x36))

/* Generic Desktop Page: Dial [DV] */
#define HID_USAGE_GD_DIAL (HID_USAGE(HID_USAGE_GD, 0x37))

/* Generic Desktop Page: Wheel [DV] */
#define HID_USAGE_GD_WHEEL (HID_USAGE(HID_USAGE_GD, 0x38))

/* Generic Desktop Page: Hat Switch [DV] */
#define HID_USAGE_GD_HAT_SWITCH (HID_USAGE(HID_USAGE_GD, 0x39))

/* Generic Desktop Page: Counted Buffer [CL] */
#define HID_USAGE_GD_COUNTED_BUFFER (HID_USAGE(HID_USAGE_GD, 0x3A))

/* Generic Desktop Page: Byte Count [DV] */
#define HID_USAGE_GD_BYTE_COUNT (HID_USAGE(HID_USAGE_GD, 0x3B))

/* Generic Desktop Page: Motion Wakeup [OSC, DF] */
#define HID_USAGE_GD_MOTION_WAKEUP (HID_USAGE(HID_USAGE_GD, 0x3C))

/* Generic Desktop Page: Start [OOC] */
#define HID_USAGE_GD_START (HID_USAGE(HID_USAGE_GD, 0x3D))

/* Generic Desktop Page: Select [OOC] */
#define HID_USAGE_GD_SELECT (HID_USAGE(HID_USAGE_GD, 0x3E))

/* Generic Desktop Page: Vx [DV] */
#define HID_USAGE_GD_VX (HID_USAGE(HID_USAGE_GD, 0x40))

/* Generic Desktop Page: Vy [DV] */
#define HID_USAGE_GD_VY (HID_USAGE(HID_USAGE_GD, 0x41))

/* Generic Desktop Page: Vz [DV] */
#define HID_USAGE_GD_VZ (HID_USAGE(HID_USAGE_GD, 0x42))

/* Generic Desktop Page: Vbrx [DV] */
#define HID_USAGE_GD_VBRX (HID_USAGE(HID_USAGE_GD, 0x43))

/* Generic Desktop Page: Vbry [DV] */
#define HID_USAGE_GD_VBRY (HID_USAGE(HID_USAGE_GD, 0x44))

/* Generic Desktop Page: Vbrz [DV] */
#define HID_USAGE_GD_VBRZ (HID_USAGE(HID_USAGE_GD, 0x45))

/* Generic Desktop Page: Vno [DV] */
#define HID_USAGE_GD_VNO (HID_USAGE(HID_USAGE_GD, 0x46))

/* Generic Desktop Page: Feature Notification [DV, DF] */
#define HID_USAGE_GD_FEATURE_NOTIFICATION (HID_USAGE(HID_USAGE_GD, 0x47))

/* Generic Desktop Page: Resolution Multiplier [DV] */
#define HID_USAGE_GD_RESOLUTION_MULTIPLIER (HID_USAGE(HID_USAGE_GD, 0x48))

/* Generic Desktop Page: Qx [DV] */
#define HID_USAGE_GD_QX (HID_USAGE(HID_USAGE_GD, 0x49))

/* Generic Desktop Page: Qy [DV] */
#define HID_USAGE_GD_QY (HID_USAGE(HID_USAGE_GD, 0x4A))

/* Generic Desktop Page: Qz [DV] */
#define HID_USAGE_GD_QZ (HID_USAGE(HID_USAGE_GD, 0x4B))

/* Generic Desktop Page: Qw [DV] */
#define HID_USAGE_GD_QW (HID_USAGE(HID_USAGE_GD, 0x4C))

/* Generic Desktop Page: System Control [CA] */
#define HID_USAGE_GD_SYSTEM_CONTROL (HID_USAGE(HID_USAGE_GD, 0x80))

/* Generic Desktop Page: System Power Down [OSC] */
#define HID_USAGE_GD_SYSTEM_POWER_DOWN (HID_USAGE(HID_USAGE_GD, 0x81))

/* Generic Desktop Page: System Sleep [OSC] */
#define HID_USAGE_GD_SYSTEM_SLEEP (HID_USAGE(HID_USAGE_GD, 0x82))

/* Generic Desktop Page: System Wake Up [OSC] */
#define HID_USAGE_GD_SYSTEM_WAKE_UP (HID_USAGE(HID_USAGE_GD, 0x83))

/* Generic Desktop Page: System Context Menu [OSC] */
#define HID_USAGE_GD_SYSTEM_CONTEXT_MENU (HID_USAGE(HID_USAGE_GD, 0x84))

/* Generic Desktop Page: System Main Menu [OSC] */
#define HID_USAGE_GD_SYSTEM_MAIN_MENU (HID_USAGE(HID_USAGE_GD, 0x85))

/* Generic Desktop Page: System App Menu [OSC] */
#define HID_USAGE_GD_SYSTEM_APP_MENU (HID_USAGE(HID_USAGE_GD, 0x86))

/* Generic Desktop Page: System Menu Help [OSC] */
#define HID_USAGE_GD_SYSTEM_MENU_HELP (HID_USAGE(HID_USAGE_GD, 0x87))

/* Generic Desktop Page: System Menu Exit [OSC] */
#define HID_USAGE_GD_SYSTEM_MENU_EXIT (HID_USAGE(HID_USAGE_GD, 0x88))

/* Generic Desktop Page: System Menu Select [OSC] */
#define HID_USAGE_GD_SYSTEM_MENU_SELECT (HID_USAGE(HID_USAGE_GD, 0x89))

/* Generic Desktop Page: System Menu Right [RTC] */
#define HID_USAGE_GD_SYSTEM_MENU_RIGHT (HID_USAGE(HID_USAGE_GD, 0x8A))

/* Generic Desktop Page: System Menu Left [RTC] */
#define HID_USAGE_GD_SYSTEM_MENU_LEFT (HID_USAGE(HID_USAGE_GD, 0x8B))

/* Generic Desktop Page: System Menu Up [RTC] */
#define HID_USAGE_GD_SYSTEM_MENU_UP (HID_USAGE(HID_USAGE_GD, 0x8C))

/* Generic Desktop Page: System Menu Down [RTC] */
#define HID_USAGE_GD_SYSTEM_MENU_DOWN (HID_USAGE(HID_USAGE_GD, 0x8D))

/* Generic Desktop Page: System Cold Restart [OSC] */
#define HID_USAGE_GD_SYSTEM_COLD_RESTART (HID_USAGE(HID_USAGE_GD, 0x8E))

/* Generic Desktop Page: System Warm Restart [OSC] */
#define HID_USAGE_GD_SYSTEM_WARM_RESTART (HID_USAGE(HID_USAGE_GD, 0x8F))

/* Generic Desktop Page: D-pad Up [OOC] */
#define HID_USAGE_GD_D_PAD_UP (HID_USAGE(HID_USAGE_GD, 0x90))

/* Generic Desktop Page: D-pad Down [OOC] */
#define HID_USAGE_GD_D_PAD_DOWN (HID_USAGE(HID_USAGE_GD, 0x91))

/* Generic Desktop Page: D-pad Right [OOC] */
#define HID_USAGE_GD_D_PAD_RIGHT (HID_USAGE(HID_USAGE_GD, 0x92))

/* Generic Desktop Page: D-pad Left [OOC] */
#define HID_USAGE_GD_D_PAD_LEFT (HID_USAGE(HID_USAGE_GD, 0x93))

/* Generic Desktop Page: Index Trigger [MC, DV] */
#define HID_USAGE_GD_INDEX_TRIGGER (HID_USAGE(HID_USAGE_GD, 0x94))

/* Generic Desktop Page: Palm Trigger [MC, DV] */
#define HID_USAGE_GD_PALM_TRIGGER (HID_USAGE(HID_USAGE_GD, 0x95))

/* Generic Desktop Page: Thumbstick [CP] */
#define HID_USAGE_GD_THUMBSTICK (HID_USAGE(HID_USAGE_GD, 0x96))

/* Generic Desktop Page: System Function Shift [MC] */
#define HID_USAGE_GD_SYSTEM_FUNCTION_SHIFT (HID_USAGE(HID_USAGE_GD, 0x97))

/* Generic Desktop Page: System Function Shift Lock [OOC] */
#define HID_USAGE_GD_SYSTEM_FUNCTION_SHIFT_LOCK (HID_USAGE(HID_USAGE_GD, 0x98))

/* Generic Desktop Page: System Function Shift Lock Indicator [DV] */
#define HID_USAGE_GD_SYSTEM_FUNCTION_SHIFT_LOCK_INDICATOR (HID_USAGE(HID_USAGE_GD, 0x99))

/* Generic Desktop Page: System Dismiss Notification [OSC] */
#define HID_USAGE_GD_SYSTEM_DISMISS_NOTIFICATION (HID_USAGE(HID_USAGE_GD, 0x9A))

/* Generic Desktop Page: System Do Not Disturb [OOC] */
#define HID_USAGE_GD_SYSTEM_DO_NOT_DISTURB (HID_USAGE(HID_USAGE_GD, 0x9B))

/* Generic Desktop Page: System Dock [OSC] */
#define HID_USAGE_GD_SYSTEM_DOCK (HID_USAGE(HID_USAGE_GD, 0xA0))

/* Generic Desktop Page: System Undock [OSC] */
#define HID_USAGE_GD_SYSTEM_UNDOCK (HID_USAGE(HID_USAGE_GD, 0xA1))

/* Generic Desktop Page: System Setup [OSC] */
#define HID_USAGE_GD_SYSTEM_SETUP (HID_USAGE(HID_USAGE_GD, 0xA2))

/* Generic Desktop Page: System Break [OSC] */
#define HID_USAGE_GD_SYSTEM_BREAK (HID_USAGE(HID_USAGE_GD, 0xA3))

/* Generic Desktop Page: System Debugger Break [OSC] */
#define HID_USAGE_GD_SYSTEM_DEBUGGER_BREAK (HID_USAGE(HID_USAGE_GD, 0xA4))

/* Generic Desktop Page: Application Break [OSC] */
#define HID_USAGE_GD_APPLICATION_BREAK (HID_USAGE(HID_USAGE_GD, 0xA5))

/* Generic Desktop Page: Application Debugger Break [OSC] */
#define HID_USAGE_GD_APPLICATION_DEBUGGER_BREAK (HID_USAGE(HID_USAGE_GD, 0xA6))

/* Generic Desktop Page: System Speaker Mute [OSC] */
#define HID_USAGE_GD_SYSTEM_SPEAKER_MUTE (HID_USAGE(HID_USAGE_GD, 0xA7))

/* Generic Desktop Page: System Hibernate [OSC] */
#define HID_USAGE_GD_SYSTEM_HIBERNATE (HID_USAGE(HID_USAGE_GD, 0xA8))

/* Generic Desktop Page: System Display Invert [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_INVERT (HID_USAGE(HID_USAGE_GD, 0xB0))

/* Generic Desktop Page: System Display Internal [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_INTERNAL (HID_USAGE(HID_USAGE_GD, 0xB1))

/* Generic Desktop Page: System Display External [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_EXTERNAL (HID_USAGE(HID_USAGE_GD, 0xB2))

/* Generic Desktop Page: System Display Both [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_BOTH (HID_USAGE(HID_USAGE_GD, 0xB3))

/* Generic Desktop Page: System Display Dual [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_DUAL (HID_USAGE(HID_USAGE_GD, 0xB4))

/* Generic Desktop Page: System Display Toggle Int/Ext Mode [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_TOGGLE_INT_EXT_MODE (HID_USAGE(HID_USAGE_GD, 0xB5))

/* Generic Desktop Page: System Display Swap Primary/Secondary [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_SWAP_PRIMARY_SECONDARY (HID_USAGE(HID_USAGE_GD, 0xB6))

/* Generic Desktop Page: System Display Toggle LCD Autoscale [OSC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_TOGGLE_LCD_AUTOSCALE (HID_USAGE(HID_USAGE_GD, 0xB7))

/* Generic Desktop Page: Sensor Zone [CL] */
#define HID_USAGE_GD_SENSOR_ZONE (HID_USAGE(HID_USAGE_GD, 0xC0))

/* Generic Desktop Page: RPM [DV] */
#define HID_USAGE_GD_RPM (HID_USAGE(HID_USAGE_GD, 0xC1))

/* Generic Desktop Page: Coolant Level [DV] */
#define HID_USAGE_GD_COOLANT_LEVEL (HID_USAGE(HID_USAGE_GD, 0xC2))

/* Generic Desktop Page: Coolant Critical Level [SV] */
#define HID_USAGE_GD_COOLANT_CRITICAL_LEVEL (HID_USAGE(HID_USAGE_GD, 0xC3))

/* Generic Desktop Page: Coolant Pump [US] */
#define HID_USAGE_GD_COOLANT_PUMP (HID_USAGE(HID_USAGE_GD, 0xC4))

/* Generic Desktop Page: Chassis Enclosure [CL] */
#define HID_USAGE_GD_CHASSIS_ENCLOSURE (HID_USAGE(HID_USAGE_GD, 0xC5))

/* Generic Desktop Page: Wireless Radio Button [OOC] */
#define HID_USAGE_GD_WIRELESS_RADIO_BUTTON (HID_USAGE(HID_USAGE_GD, 0xC6))

/* Generic Desktop Page: Wireless Radio LED [OOC] */
#define HID_USAGE_GD_WIRELESS_RADIO_LED (HID_USAGE(HID_USAGE_GD, 0xC7))

/* Generic Desktop Page: Wireless Radio Slider Switch [OOC] */
#define HID_USAGE_GD_WIRELESS_RADIO_SLIDER_SWITCH (HID_USAGE(HID_USAGE_GD, 0xC8))

/* Generic Desktop Page: System Display Rotation Lock Button [OOC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_ROTATION_LOCK_BUTTON (HID_USAGE(HID_USAGE_GD, 0xC9))

/* Generic Desktop Page: System Display Rotation Lock Slider Switch [OOC] */
#define HID_USAGE_GD_SYSTEM_DISPLAY_ROTATION_LOCK_SLIDER_SWITCH (HID_USAGE(HID_USAGE_GD, 0xCA))

/* Generic Desktop Page: Control Enable [DF] */
#define HID_USAGE_GD_CONTROL_ENABLE (HID_USAGE(HID_USAGE_GD, 0xCB))

/* Generic Desktop Page: Dockable Device Unique ID [DV] */
#define HID_USAGE_GD_DOCKABLE_DEVICE_UNIQUE_ID (HID_USAGE(HID_USAGE_GD, 0xD0))

/* Generic Desktop Page: Dockable Device Vendor ID [DV] */
#define HID_USAGE_GD_DOCKABLE_DEVICE_VENDOR_ID (HID_USAGE(HID_USAGE_GD, 0xD1))

/* Generic Desktop Page: Dockable Device Primary Usage Page [DV] */
#define HID_USAGE_GD_DOCKABLE_DEVICE_PRIMARY_USAGE_PAGE (HID_USAGE(HID_USAGE_GD, 0xD2))

/* Generic Desktop Page: Dockable Device Primary Usage ID [DV] */
#define HID_USAGE_GD_DOCKABLE_DEVICE_PRIMARY_USAGE_ID (HID_USAGE(HID_USAGE_GD, 0xD3))

/* Generic Desktop Page: Dockable Device Docking State [DF] */
#define HID_USAGE_GD_DOCKABLE_DEVICE_DOCKING_STATE (HID_USAGE(HID_USAGE_GD, 0xD4))

/* Generic Desktop Page: Dockable Device Display Occlusion [CL] */
#define HID_USAGE_GD_DOCKABLE_DEVICE_DISPLAY_OCCLUSION (HID_USAGE(HID_USAGE_GD, 0xD5))

/* Generic Desktop Page: Dockable Device Object Type [DV] */
#define HID_USAGE_GD_DOCKABLE_DEVICE_OBJECT_TYPE (HID_USAGE(HID_USAGE_GD, 0xD6))

/* Simulation Controls Page */
#define HID_USAGE_SIM (0x02)

/* Simulation Controls Page: Undefined */
#define HID_USAGE_SIM_UNDEFINED (HID_USAGE(HID_USAGE_SIM, 0x00))

/* Simulation Controls Page: Flight Simulation Device [CA] */
#define HID_USAGE_SIM_FLIGHT_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x01))

/* Simulation Controls Page: Automobile Simulation Device [CA] */
#define HID_USAGE_SIM_AUTOMOBILE_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x02))

/* Simulation Controls Page: Tank Simulation Device [CA] */
#define HID_USAGE_SIM_TANK_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x03))

/* Simulation Controls Page: Spaceship Simulation Device [CA] */
#define HID_USAGE_SIM_SPACESHIP_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x04))

/* Simulation Controls Page: Submarine Simulation Device [CA] */
#define HID_USAGE_SIM_SUBMARINE_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x05))

/* Simulation Controls Page: Sailing Simulation Device [CA] */
#define HID_USAGE_SIM_SAILING_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x06))

/* Simulation Controls Page: Motorcycle Simulation Device [CA] */
#define HID_USAGE_SIM_MOTORCYCLE_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x07))

/* Simulation Controls Page: Sports Simulation Device [CA] */
#define HID_USAGE_SIM_SPORTS_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x08))

/* Simulation Controls Page: Airplane Simulation Device [CA] */
#define HID_USAGE_SIM_AIRPLANE_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x09))

/* Simulation Controls Page: Helicopter Simulation Device [CA] */
#define HID_USAGE_SIM_HELICOPTER_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x0A))

/* Simulation Controls Page: Magic Carpet Simulation Device [CA] */
#define HID_USAGE_SIM_MAGIC_CARPET_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x0B))

/* Simulation Controls Page: Bicycle Simulation Device [CA] */
#define HID_USAGE_SIM_BICYCLE_SIMULATION_DEVICE (HID_USAGE(HID_USAGE_SIM, 0x0C))

/* Simulation Controls Page: Flight Control Stick [CA] */
#define HID_USAGE_SIM_FLIGHT_CONTROL_STICK (HID_USAGE(HID_USAGE_SIM, 0x20))

/* Simulation Controls Page: Flight Stick [CA] */
#define HID_USAGE_SIM_FLIGHT_STICK (HID_USAGE(HID_USAGE_SIM, 0x21))

/* Simulation Controls Page: Cyclic Control [CP] */
#define HID_USAGE_SIM_CYCLIC_CONTROL (HID_USAGE(HID_USAGE_SIM, 0x22))

/* Simulation Controls Page: Cyclic Trim [CP] */
#define HID_USAGE_SIM_CYCLIC_TRIM (HID_USAGE(HID_USAGE_SIM, 0x23))

/* Simulation Controls Page: Flight Yoke [CA] */
#define HID_USAGE_SIM_FLIGHT_YOKE (HID_USAGE(HID_USAGE_SIM, 0x24))

/* Simulation Controls Page: Track Control [CP] */
#define HID_USAGE_SIM_TRACK_CONTROL (HID_USAGE(HID_USAGE_SIM, 0x25))

/* Simulation Controls Page: Aileron [DV] */
#define HID_USAGE_SIM_AILERON (HID_USAGE(HID_USAGE_SIM, 0xB0))

/* Simulation Controls Page: Aileron Trim [DV] */
#define HID_USAGE_SIM_AILERON_TRIM (HID_USAGE(HID_USAGE_SIM, 0xB1))

/* Simulation Controls Page: Anti-Torque Control [DV] */
#define HID_USAGE_SIM_ANTI_TORQUE_CONTROL (HID_USAGE(HID_USAGE_SIM, 0xB2))

/* Simulation Controls Page: Autopilot Enable [OOC] */
#define HID_USAGE_SIM_AUTOPILOT_ENABLE (HID_USAGE(HID_USAGE_SIM, 0xB3))

/* Simulation Controls Page: Chaff Release [OSC] */
#define HID_USAGE_SIM_CHAFF_RELEASE (HID_USAGE(HID_USAGE_SIM, 0xB4))

/* Simulation Controls Page: Collective Control [DV] */
#define HID_USAGE_SIM_COLLECTIVE_CONTROL (HID_USAGE(HID_USAGE_SIM, 0xB5))

/* Simulation Controls Page: Dive Brake [DV] */
#define HID_USAGE_SIM_DIVE_BRAKE (HID_USAGE(HID_USAGE_SIM, 0xB6))

/* Simulation Controls Page: Electronic Countermeasures [OOC] */
#define HID_USAGE_SIM_ELECTRONIC_COUNTERMEASURES (HID_USAGE(HID_USAGE_SIM, 0xB7))

/* Simulation Controls Page: Elevator [DV] */
#define HID_USAGE_SIM_ELEVATOR (HID_USAGE(HID_USAGE_SIM, 0xB8))

/* Simulation Controls Page: Elevator Trim [DV] */
#define HID_USAGE_SIM_ELEVATOR_TRIM (HID_USAGE(HID_USAGE_SIM, 0xB9))

/* Simulation Controls Page: Rudder [DV] */
#define HID_USAGE_SIM_RUDDER (HID_USAGE(HID_USAGE_SIM, 0xBA))

/* Simulation Controls Page: Throttle [DV] */
#define HID_USAGE_SIM_THROTTLE (HID_USAGE(HID_USAGE_SIM, 0xBB))

/* Simulation Controls Page: Flight Communications [OOC] */
#define HID_USAGE_SIM_FLIGHT_COMMUNICATIONS (HID_USAGE(HID_USAGE_SIM, 0xBC))

/* Simulation Controls Page: Flare Release [OSC] */
#define HID_USAGE_SIM_FLARE_RELEASE (HID_USAGE(HID_USAGE_SIM, 0xBD))

/* Simulation Controls Page: Landing Gear [OOC] */
#define HID_USAGE_SIM_LANDING_GEAR (HID_USAGE(HID_USAGE_SIM, 0xBE))

/* Simulation Controls Page: Toe Brake [DV] */
#define HID_USAGE_SIM_TOE_BRAKE (HID_USAGE(HID_USAGE_SIM, 0xBF))

/* Simulation Controls Page: Trigger [MC] */
#define HID_USAGE_SIM_TRIGGER (HID_USAGE(HID_USAGE_SIM, 0xC0))

/* Simulation Controls Page: Weapons Arm [OOC] */
#define HID_USAGE_SIM_WEAPONS_ARM (HID_USAGE(HID_USAGE_SIM, 0xC1))

/* Simulation Controls Page: Weapons Select [OSC] */
#define HID_USAGE_SIM_WEAPONS_SELECT (HID_USAGE(HID_USAGE_SIM, 0xC2))

/* Simulation Controls Page: Wing Flaps [DV] */
#define HID_USAGE_SIM_WING_FLAPS (HID_USAGE(HID_USAGE_SIM, 0xC3))

/* Simulation Controls Page: Accelerator [DV] */
#define HID_USAGE_SIM_ACCELERATOR (HID_USAGE(HID_USAGE_SIM, 0xC4))

/* Simulation Controls Page: Brake [DV] */
#define HID_USAGE_SIM_BRAKE (HID_USAGE(HID_USAGE_SIM, 0xC5))

/* Simulation Controls Page: Clutch [DV] */
#define HID_USAGE_SIM_CLUTCH (HID_USAGE(HID_USAGE_SIM, 0xC6))

/* Simulation Controls Page: Shifter [DV] */
#define HID_USAGE_SIM_SHIFTER (HID_USAGE(HID_USAGE_SIM, 0xC7))

/* Simulation Controls Page: Steering [DV] */
#define HID_USAGE_SIM_STEERING (HID_USAGE(HID_USAGE_SIM, 0xC8))

/* Simulation Controls Page: Turret Direction [DV] */
#define HID_USAGE_SIM_TURRET_DIRECTION (HID_USAGE(HID_USAGE_SIM, 0xC9))

/* Simulation Controls Page: Barrel Elevation [DV] */
#define HID_USAGE_SIM_BARREL_ELEVATION (HID_USAGE(HID_USAGE_SIM, 0xCA))

/* Simulation Controls Page: Dive Plane [DV] */
#define HID_USAGE_SIM_DIVE_PLANE (HID_USAGE(HID_USAGE_SIM, 0xCB))

/* Simulation Controls Page: Ballast [DV] */
#define HID_USAGE_SIM_BALLAST (HID_USAGE(HID_USAGE_SIM, 0xCC))

/* Simulation Controls Page: Bicycle Crank [DV] */
#define HID_USAGE_SIM_BICYCLE_CRANK (HID_USAGE(HID_USAGE_SIM, 0xCD))

/* Simulation Controls Page: Handle Bars [DV] */
#define HID_USAGE_SIM_HANDLE_BARS (HID_USAGE(HID_USAGE_SIM, 0xCE))

/* Simulation Controls Page: Front Brake [DV] */
#define HID_USAGE_SIM_FRONT_BRAKE (HID_USAGE(HID_USAGE_SIM, 0xCF))

/* Simulation Controls Page: Rear Brake [DV] */
#define HID_USAGE_SIM_REAR_BRAKE (HID_USAGE(HID_USAGE_SIM, 0xD0))

/* VR Controls Page */
#define HID_USAGE_VR (0x03)

/* VR Controls Page: Undefined */
#define HID_USAGE_VR_UNDEFINED (HID_USAGE(HID_USAGE_VR, 0x00))

/* VR Controls Page: Belt [CA] */
#define HID_USAGE_VR_BELT (HID_USAGE(HID_USAGE_VR, 0x01))

/* VR Controls Page: Body Suit [CA] */
#define HID_USAGE_VR_BODY_SUIT (HID_USAGE(HID_USAGE_VR, 0x02))

/* VR Controls Page: Flexor [CP] */
#define HID_USAGE_VR_FLEXOR (HID_USAGE(HID_USAGE_VR, 0x03))

/* VR Controls Page: Glove [CA] */
#define HID_USAGE_VR_GLOVE (HID_USAGE(HID_USAGE_VR, 0x04))

/* VR Controls Page: Head Tracker [CP] */
#define HID_USAGE_VR_HEAD_TRACKER (HID_USAGE(HID_USAGE_VR, 0x05))

/* VR Controls Page: Head Mounted Display [CA] */
#define HID_USAGE_VR_HEAD_MOUNTED_DISPLAY (HID_USAGE(HID_USAGE_VR, 0x06))

/* VR Controls Page: Hand Tracker [CA] */
#define HID_USAGE_VR_HAND_TRACKER (HID_USAGE(HID_USAGE_VR, 0x07))

/* VR Controls Page: Oculometer [CA] */
#define HID_USAGE_VR_OCULOMETER (HID_USAGE(HID_USAGE_VR, 0x08))

/* VR Controls Page: Vest [CA] */
#define HID_USAGE_VR_VEST (HID_USAGE(HID_USAGE_VR, 0x09))

/* VR Controls Page: Animatronic Device [CA] */
#define HID_USAGE_VR_ANIMATRONIC_DEVICE (HID_USAGE(HID_USAGE_VR, 0x0A))

/* VR Controls Page: Stereo Enable [OOC] */
#define HID_USAGE_VR_STEREO_ENABLE (HID_USAGE(HID_USAGE_VR, 0x20))

/* VR Controls Page: Display Enable [OOC] */
#define HID_USAGE_VR_DISPLAY_ENABLE (HID_USAGE(HID_USAGE_VR, 0x21))

/* Sport Controls Page */
#define HID_USAGE_SPORT (0x04)

/* Sport Controls Page: Undefined */
#define HID_USAGE_SPORT_UNDEFINED (HID_USAGE(HID_USAGE_SPORT, 0x00))

/* Sport Controls Page: Baseball Bat [CA] */
#define HID_USAGE_SPORT_BASEBALL_BAT (HID_USAGE(HID_USAGE_SPORT, 0x01))

/* Sport Controls Page: Golf Club [CA] */
#define HID_USAGE_SPORT_GOLF_CLUB (HID_USAGE(HID_USAGE_SPORT, 0x02))

/* Sport Controls Page: Rowing Machine [CA] */
#define HID_USAGE_SPORT_ROWING_MACHINE (HID_USAGE(HID_USAGE_SPORT, 0x03))

/* Sport Controls Page: Treadmill [CA] */
#define HID_USAGE_SPORT_TREADMILL (HID_USAGE(HID_USAGE_SPORT, 0x04))

/* Sport Controls Page: Oar [DV] */
#define HID_USAGE_SPORT_OAR (HID_USAGE(HID_USAGE_SPORT, 0x30))

/* Sport Controls Page: Slope [DV] */
#define HID_USAGE_SPORT_SLOPE (HID_USAGE(HID_USAGE_SPORT, 0x31))

/* Sport Controls Page: Rate [DV] */
#define HID_USAGE_SPORT_RATE (HID_USAGE(HID_USAGE_SPORT, 0x32))

/* Sport Controls Page: Stick Speed [DV] */
#define HID_USAGE_SPORT_STICK_SPEED (HID_USAGE(HID_USAGE_SPORT, 0x33))

/* Sport Controls Page: Stick Face Angle [DV] */
#define HID_USAGE_SPORT_STICK_FACE_ANGLE (HID_USAGE(HID_USAGE_SPORT, 0x34))

/* Sport Controls Page: Stick Heel/Toe [DV] */
#define HID_USAGE_SPORT_STICK_HEEL_TOE (HID_USAGE(HID_USAGE_SPORT, 0x35))

/* Sport Controls Page: Stick Follow Through [DV] */
#define HID_USAGE_SPORT_STICK_FOLLOW_THROUGH (HID_USAGE(HID_USAGE_SPORT, 0x36))

/* Sport Controls Page: Stick Tempo [DV] */
#define HID_USAGE_SPORT_STICK_TEMPO (HID_USAGE(HID_USAGE_SPORT, 0x37))

/* Sport Controls Page: Stick Type [NAry] */
#define HID_USAGE_SPORT_STICK_TYPE (HID_USAGE(HID_USAGE_SPORT, 0x38))

/* Sport Controls Page: Stick Height [DV] */
#define HID_USAGE_SPORT_STICK_HEIGHT (HID_USAGE(HID_USAGE_SPORT, 0x39))

/* Sport Controls Page: Putter [Sel] */
#define HID_USAGE_SPORT_PUTTER (HID_USAGE(HID_USAGE_SPORT, 0x50))

/* Sport Controls Page: 1 Iron [Sel] */
#define HID_USAGE_SPORT_1_IRON (HID_USAGE(HID_USAGE_SPORT, 0x51))

/* Sport Controls Page: 2 Iron [Sel] */
#define HID_USAGE_SPORT_2_IRON (HID_USAGE(HID_USAGE_SPORT, 0x52))

/* Sport Controls Page: 3 Iron [Sel] */
#define HID_USAGE_SPORT_3_IRON (HID_USAGE(HID_USAGE_SPORT, 0x53))

/* Sport Controls Page: 4 Iron [Sel] */
#define HID_USAGE_SPORT_4_IRON (HID_USAGE(HID_USAGE_SPORT, 0x54))

/* Sport Controls Page: 5 Iron [Sel] */
#define HID_USAGE_SPORT_5_IRON (HID_USAGE(HID_USAGE_SPORT, 0x55))

/* Sport Controls Page: 6 Iron [Sel] */
#define HID_USAGE_SPORT_6_IRON (HID_USAGE(HID_USAGE_SPORT, 0x56))

/* Sport Controls Page: 7 Iron [Sel] */
#define HID_USAGE_SPORT_7_IRON (HID_USAGE(HID_USAGE_SPORT, 0x57))

/* Sport Controls Page: 8 Iron [Sel] */
#define HID_USAGE_SPORT_8_IRON (HID_USAGE(HID_USAGE_SPORT, 0x58))

/* Sport Controls Page: 9 Iron [Sel] */
#define HID_USAGE_SPORT_9_IRON (HID_USAGE(HID_USAGE_SPORT, 0x59))

/* Sport Controls Page: 10 Iron [Sel] */
#define HID_USAGE_SPORT_10_IRON (HID_USAGE(HID_USAGE_SPORT, 0x5A))

/* Sport Controls Page: 11 Iron [Sel] */
#define HID_USAGE_SPORT_11_IRON (HID_USAGE(HID_USAGE_SPORT, 0x5B))

/* Sport Controls Page: Sand Wedge [Sel] */
#define HID_USAGE_SPORT_SAND_WEDGE (HID_USAGE(HID_USAGE_SPORT, 0x5C))

/* Sport Controls Page: Loft Wedge [Sel] */
#define HID_USAGE_SPORT_LOFT_WEDGE (HID_USAGE(HID_USAGE_SPORT, 0x5D))

/* Sport Controls Page: Power Wedge [Sel] */
#define HID_USAGE_SPORT_POWER_WEDGE (HID_USAGE(HID_USAGE_SPORT, 0x5E))

/* Sport Controls Page: 1 Wood [Sel] */
#define HID_USAGE_SPORT_1_WOOD (HID_USAGE(HID_USAGE_SPORT, 0x5F))

/* Sport Controls Page: 3 Wood [Sel] */
#define HID_USAGE_SPORT_3_WOOD (HID_USAGE(HID_USAGE_SPORT, 0x60))

/* Sport Controls Page: 5 Wood [Sel] */
#define HID_USAGE_SPORT_5_WOOD (HID_USAGE(HID_USAGE_SPORT, 0x61))

/* Sport Controls Page: 7 Wood [Sel] */
#define HID_USAGE_SPORT_7_WOOD (HID_USAGE(HID_USAGE_SPORT, 0x62))

/* Sport Controls Page: 9 Wood [Sel] */
#define HID_USAGE_SPORT_9_WOOD (HID_USAGE(HID_USAGE_SPORT, 0x63))

/* Game Controls Page */
#define HID_USAGE_GAME (0x05)

/* Game Controls Page: Undefined */
#define HID_USAGE_GAME_UNDEFINED (HID_USAGE(HID_USAGE_GAME, 0x00))

/* Game Controls Page: 3D Game Controller [CA] */
#define HID_USAGE_GAME_3D_GAME_CONTROLLER (HID_USAGE(HID_USAGE_GAME, 0x01))

/* Game Controls Page: Pinball Device [CA] */
#define HID_USAGE_GAME_PINBALL_DEVICE (HID_USAGE(HID_USAGE_GAME, 0x02))

/* Game Controls Page: Gun Device [CA] */
#define HID_USAGE_GAME_GUN_DEVICE (HID_USAGE(HID_USAGE_GAME, 0x03))

/* Game Controls Page: Point of View [CP] */
#define HID_USAGE_GAME_POINT_OF_VIEW (HID_USAGE(HID_USAGE_GAME, 0x20))

/* Game Controls Page: Turn Right/Left [DV] */
#define HID_USAGE_GAME_TURN_RIGHT_LEFT (HID_USAGE(HID_USAGE_GAME, 0x21))

/* Game Controls Page: Pitch Forward/Backward [DV] */
#define HID_USAGE_GAME_PITCH_FORWARD_BACKWARD (HID_USAGE(HID_USAGE_GAME, 0x22))

/* Game Controls Page: Roll Right/Left [DV] */
#define HID_USAGE_GAME_ROLL_RIGHT_LEFT (HID_USAGE(HID_USAGE_GAME, 0x23))

/* Game Controls Page: Move Right/Left [DV] */
#define HID_USAGE_GAME_MOVE_RIGHT_LEFT (HID_USAGE(HID_USAGE_GAME, 0x24))

/* Game Controls Page: Move Forward/Backward [DV] */
#define HID_USAGE_GAME_MOVE_FORWARD_BACKWARD (HID_USAGE(HID_USAGE_GAME, 0x25))

/* Game Controls Page: Move Up/Down [DV] */
#define HID_USAGE_GAME_MOVE_UP_DOWN (HID_USAGE(HID_USAGE_GAME, 0x26))

/* Game Controls Page: Lean Right/Left [DV] */
#define HID_USAGE_GAME_LEAN_RIGHT_LEFT (HID_USAGE(HID_USAGE_GAME, 0x27))

/* Game Controls Page: Lean Forward/Backward [DV] */
#define HID_USAGE_GAME_LEAN_FORWARD_BACKWARD (HID_USAGE(HID_USAGE_GAME, 0x28))

/* Game Controls Page: Height of POV [DV] */
#define HID_USAGE_GAME_HEIGHT_OF_POV (HID_USAGE(HID_USAGE_GAME, 0x29))

/* Game Controls Page: Flipper [MC] */
#define HID_USAGE_GAME_FLIPPER (HID_USAGE(HID_USAGE_GAME, 0x2A))

/* Game Controls Page: Secondary Flipper [MC] */
#define HID_USAGE_GAME_SECONDARY_FLIPPER (HID_USAGE(HID_USAGE_GAME, 0x2B))

/* Game Controls Page: Bump [MC] */
#define HID_USAGE_GAME_BUMP (HID_USAGE(HID_USAGE_GAME, 0x2C))

/* Game Controls Page: New Game [OSC] */
#define HID_USAGE_GAME_NEW_GAME (HID_USAGE(HID_USAGE_GAME, 0x2D))

/* Game Controls Page: Shoot Ball [OSC] */
#define HID_USAGE_GAME_SHOOT_BALL (HID_USAGE(HID_USAGE_GAME, 0x2E))

/* Game Controls Page: Player [OSC] */
#define HID_USAGE_GAME_PLAYER (HID_USAGE(HID_USAGE_GAME, 0x2F))

/* Game Controls Page: Gun Bolt [OOC] */
#define HID_USAGE_GAME_GUN_BOLT (HID_USAGE(HID_USAGE_GAME, 0x30))

/* Game Controls Page: Gun Clip [OOC] */
#define HID_USAGE_GAME_GUN_CLIP (HID_USAGE(HID_USAGE_GAME, 0x31))

/* Game Controls Page: Gun Selector [NAry] */
#define HID_USAGE_GAME_GUN_SELECTOR (HID_USAGE(HID_USAGE_GAME, 0x32))

/* Game Controls Page: Gun Single Shot [Sel] */
#define HID_USAGE_GAME_GUN_SINGLE_SHOT (HID_USAGE(HID_USAGE_GAME, 0x33))

/* Game Controls Page: Gun Burst [Sel] */
#define HID_USAGE_GAME_GUN_BURST (HID_USAGE(HID_USAGE_GAME, 0x34))

/* Game Controls Page: Gun Automatic [Sel] */
#define HID_USAGE_GAME_GUN_AUTOMATIC (HID_USAGE(HID_USAGE_GAME, 0x35))

/* Game Controls Page: Gun Safety [OOC] */
#define HID_USAGE_GAME_GUN_SAFETY (HID_USAGE(HID_USAGE_GAME, 0x36))

/* Game Controls Page: Gamepad Fire/Jump [CL] */
#define HID_USAGE_GAME_GAMEPAD_FIRE_JUMP (HID_USAGE(HID_USAGE_GAME, 0x37))

/* Game Controls Page: Gamepad Trigger [CL] */
#define HID_USAGE_GAME_GAMEPAD_TRIGGER (HID_USAGE(HID_USAGE_GAME, 0x39))

/* Game Controls Page: Form-fitting Gamepad [SF] */
#define HID_USAGE_GAME_FORM_FITTING_GAMEPAD (HID_USAGE(HID_USAGE_GAME, 0x3A))

/* Generic Device Controls Page */
#define HID_USAGE_GDV (0x06)

/* Generic Device Controls Page: Undefined */
#define HID_USAGE_GDV_UNDEFINED (HID_USAGE(HID_USAGE_GDV, 0x00))

/* Generic Device Controls Page: Background/Nonuser Controls [CA] */
#define HID_USAGE_GDV_BACKGROUND_NONUSER_CONTROLS (HID_USAGE(HID_USAGE_GDV, 0x01))

/* Generic Device Controls Page: Battery Strength [DV] */
#define HID_USAGE_GDV_BATTERY_STRENGTH (HID_USAGE(HID_USAGE_GDV, 0x20))

/* Generic Device Controls Page: Wireless Channel [DV] */
#define HID_USAGE_GDV_WIRELESS_CHANNEL (HID_USAGE(HID_USAGE_GDV, 0x21))

/* Generic Device Controls Page: Wireless ID [DV] */
#define HID_USAGE_GDV_WIRELESS_ID (HID_USAGE(HID_USAGE_GDV, 0x22))

/* Generic Device Controls Page: Discover Wireless Control [OSC] */
#define HID_USAGE_GDV_DISCOVER_WIRELESS_CONTROL (HID_USAGE(HID_USAGE_GDV, 0x23))

/* Generic Device Controls Page: Security Code Character Entered [OSC] */
#define HID_USAGE_GDV_SECURITY_CODE_CHARACTER_ENTERED (HID_USAGE(HID_USAGE_GDV, 0x24))

/* Generic Device Controls Page: Security Code Character Erased [OSC] */
#define HID_USAGE_GDV_SECURITY_CODE_CHARACTER_ERASED (HID_USAGE(HID_USAGE_GDV, 0x25))

/* Generic Device Controls Page: Security Code Cleared [OSC] */
#define HID_USAGE_GDV_SECURITY_CODE_CLEARED (HID_USAGE(HID_USAGE_GDV, 0x26))

/* Generic Device Controls Page: Sequence ID [DV] */
#define HID_USAGE_GDV_SEQUENCE_ID (HID_USAGE(HID_USAGE_GDV, 0x27))

/* Generic Device Controls Page: Sequence ID Reset [DF] */
#define HID_USAGE_GDV_SEQUENCE_ID_RESET (HID_USAGE(HID_USAGE_GDV, 0x28))

/* Generic Device Controls Page: RF Signal Strength [DV] */
#define HID_USAGE_GDV_RF_SIGNAL_STRENGTH (HID_USAGE(HID_USAGE_GDV, 0x29))

/* Generic Device Controls Page: Software Version [CL] */
#define HID_USAGE_GDV_SOFTWARE_VERSION (HID_USAGE(HID_USAGE_GDV, 0x2A))

/* Generic Device Controls Page: Protocol Version [CL] */
#define HID_USAGE_GDV_PROTOCOL_VERSION (HID_USAGE(HID_USAGE_GDV, 0x2B))

/* Generic Device Controls Page: Hardware Version [CL] */
#define HID_USAGE_GDV_HARDWARE_VERSION (HID_USAGE(HID_USAGE_GDV, 0x2C))

/* Generic Device Controls Page: Major [SV] */
#define HID_USAGE_GDV_MAJOR (HID_USAGE(HID_USAGE_GDV, 0x2D))

/* Generic Device Controls Page: Minor [SV] */
#define HID_USAGE_GDV_MINOR (HID_USAGE(HID_USAGE_GDV, 0x2E))

/* Generic Device Controls Page: Revision [SV] */
#define HID_USAGE_GDV_REVISION (HID_USAGE(HID_USAGE_GDV, 0x2F))

/* Generic Device Controls Page: Handedness [NAry] */
#define HID_USAGE_GDV_HANDEDNESS (HID_USAGE(HID_USAGE_GDV, 0x30))

/* Generic Device Controls Page: Either Hand [Sel] */
#define HID_USAGE_GDV_EITHER_HAND (HID_USAGE(HID_USAGE_GDV, 0x31))

/* Generic Device Controls Page: Left Hand [Sel] */
#define HID_USAGE_GDV_LEFT_HAND (HID_USAGE(HID_USAGE_GDV, 0x32))

/* Generic Device Controls Page: Right Hand [Sel] */
#define HID_USAGE_GDV_RIGHT_HAND (HID_USAGE(HID_USAGE_GDV, 0x33))

/* Generic Device Controls Page: Both Hands [Sel] */
#define HID_USAGE_GDV_BOTH_HANDS (HID_USAGE(HID_USAGE_GDV, 0x34))

/* Generic Device Controls Page: Grip Pose Offset [CP] */
#define HID_USAGE_GDV_GRIP_POSE_OFFSET (HID_USAGE(HID_USAGE_GDV, 0x40))

/* Generic Device Controls Page: Pointer Pose Offset [CP] */
#define HID_USAGE_GDV_POINTER_POSE_OFFSET (HID_USAGE(HID_USAGE_GDV, 0x41))

/* Keyboard/Keypad Page */
#define HID_USAGE_KEY (0x07)

/* Keyboard/Keypad Page: Keyboard ErrorRollOver [Sel] */
#define HID_USAGE_KEY_KEYBOARD_ERRORROLLOVER (HID_USAGE(HID_USAGE_KEY, 0x01))

/* Keyboard/Keypad Page: Keyboard POSTFail [Sel] */
#define HID_USAGE_KEY_KEYBOARD_POSTFAIL (HID_USAGE(HID_USAGE_KEY, 0x02))

/* Keyboard/Keypad Page: Keyboard ErrorUndefined [Sel] */
#define HID_USAGE_KEY_KEYBOARD_ERRORUNDEFINED (HID_USAGE(HID_USAGE_KEY, 0x03))

/* Keyboard/Keypad Page: Keyboard a and A [Sel] */
#define HID_USAGE_KEY_KEYBOARD_A (HID_USAGE(HID_USAGE_KEY, 0x04))

/* Keyboard/Keypad Page: Keyboard b and B [Sel] */
#define HID_USAGE_KEY_KEYBOARD_B (HID_USAGE(HID_USAGE_KEY, 0x05))

/* Keyboard/Keypad Page: Keyboard c and C [Sel] */
#define HID_USAGE_KEY_KEYBOARD_C (HID_USAGE(HID_USAGE_KEY, 0x06))

/* Keyboard/Keypad Page: Keyboard d and D [Sel] */
#define HID_USAGE_KEY_KEYBOARD_D (HID_USAGE(HID_USAGE_KEY, 0x07))

/* Keyboard/Keypad Page: Keyboard e and E [Sel] */
#define HID_USAGE_KEY_KEYBOARD_E (HID_USAGE(HID_USAGE_KEY, 0x08))

/* Keyboard/Keypad Page: Keyboard f and F [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F (HID_USAGE(HID_USAGE_KEY, 0x09))

/* Keyboard/Keypad Page: Keyboard g and G [Sel] */
#define HID_USAGE_KEY_KEYBOARD_G (HID_USAGE(HID_USAGE_KEY, 0x0A))

/* Keyboard/Keypad Page: Keyboard h and H [Sel] */
#define HID_USAGE_KEY_KEYBOARD_H (HID_USAGE(HID_USAGE_KEY, 0x0B))

/* Keyboard/Keypad Page: Keyboard i and I [Sel] */
#define HID_USAGE_KEY_KEYBOARD_I (HID_USAGE(HID_USAGE_KEY, 0x0C))

/* Keyboard/Keypad Page: Keyboard j and J [Sel] */
#define HID_USAGE_KEY_KEYBOARD_J (HID_USAGE(HID_USAGE_KEY, 0x0D))

/* Keyboard/Keypad Page: Keyboard k and K [Sel] */
#define HID_USAGE_KEY_KEYBOARD_K (HID_USAGE(HID_USAGE_KEY, 0x0E))

/* Keyboard/Keypad Page: Keyboard l and L [Sel] */
#define HID_USAGE_KEY_KEYBOARD_L (HID_USAGE(HID_USAGE_KEY, 0x0F))

/* Keyboard/Keypad Page: Keyboard m and M [Sel] */
#define HID_USAGE_KEY_KEYBOARD_M (HID_USAGE(HID_USAGE_KEY, 0x10))

/* Keyboard/Keypad Page: Keyboard n and N [Sel] */
#define HID_USAGE_KEY_KEYBOARD_N (HID_USAGE(HID_USAGE_KEY, 0x11))

/* Keyboard/Keypad Page: Keyboard o and O [Sel] */
#define HID_USAGE_KEY_KEYBOARD_O (HID_USAGE(HID_USAGE_KEY, 0x12))

/* Keyboard/Keypad Page: Keyboard p and P [Sel] */
#define HID_USAGE_KEY_KEYBOARD_P (HID_USAGE(HID_USAGE_KEY, 0x13))

/* Keyboard/Keypad Page: Keyboard q and Q [Sel] */
#define HID_USAGE_KEY_KEYBOARD_Q (HID_USAGE(HID_USAGE_KEY, 0x14))

/* Keyboard/Keypad Page: Keyboard r and R [Sel] */
#define HID_USAGE_KEY_KEYBOARD_R (HID_USAGE(HID_USAGE_KEY, 0x15))

/* Keyboard/Keypad Page: Keyboard s and S [Sel] */
#define HID_USAGE_KEY_KEYBOARD_S (HID_USAGE(HID_USAGE_KEY, 0x16))

/* Keyboard/Keypad Page: Keyboard t and T [Sel] */
#define HID_USAGE_KEY_KEYBOARD_T (HID_USAGE(HID_USAGE_KEY, 0x17))

/* Keyboard/Keypad Page: Keyboard u and U [Sel] */
#define HID_USAGE_KEY_KEYBOARD_U (HID_USAGE(HID_USAGE_KEY, 0x18))

/* Keyboard/Keypad Page: Keyboard v and V [Sel] */
#define HID_USAGE_KEY_KEYBOARD_V (HID_USAGE(HID_USAGE_KEY, 0x19))

/* Keyboard/Keypad Page: Keyboard w and W [Sel] */
#define HID_USAGE_KEY_KEYBOARD_W (HID_USAGE(HID_USAGE_KEY, 0x1A))

/* Keyboard/Keypad Page: Keyboard x and X [Sel] */
#define HID_USAGE_KEY_KEYBOARD_X (HID_USAGE(HID_USAGE_KEY, 0x1B))

/* Keyboard/Keypad Page: Keyboard y and Y [Sel] */
#define HID_USAGE_KEY_KEYBOARD_Y (HID_USAGE(HID_USAGE_KEY, 0x1C))

/* Keyboard/Keypad Page: Keyboard z and Z [Sel] */
#define HID_USAGE_KEY_KEYBOARD_Z (HID_USAGE(HID_USAGE_KEY, 0x1D))

/* Keyboard/Keypad Page: Keyboard 1 and ! [Sel] */
#define HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION (HID_USAGE(HID_USAGE_KEY, 0x1E))

/* Keyboard/Keypad Page: Keyboard 2 and @ [Sel] */
#define HID_USAGE_KEY_KEYBOARD_2_AND_AT (HID_USAGE(HID_USAGE_KEY, 0x1F))

/* Keyboard/Keypad Page: Keyboard 3 and # [Sel] */
#define HID_USAGE_KEY_KEYBOARD_3_AND_HASH (HID_USAGE(HID_USAGE_KEY, 0x20))

/* Keyboard/Keypad Page: Keyboard 4 and $ [Sel] */
#define HID_USAGE_KEY_KEYBOARD_4_AND_DOLLAR (HID_USAGE(HID_USAGE_KEY, 0x21))

/* Keyboard/Keypad Page: Keyboard 5 and % [Sel] */
#define HID_USAGE_KEY_KEYBOARD_5_AND_PERCENT (HID_USAGE(HID_USAGE_KEY, 0x22))

/* Keyboard/Keypad Page: Keyboard 6 and ^ [Sel] */
#define HID_USAGE_KEY_KEYBOARD_6_AND_CARET (HID_USAGE(HID_USAGE_KEY, 0x23))

/* Keyboard/Keypad Page: Keyboard 7 and & [Sel] */
#define HID_USAGE_KEY_KEYBOARD_7_AND_AMPERSAND (HID_USAGE(HID_USAGE_KEY, 0x24))

/* Keyboard/Keypad Page: Keyboard 8 and * [Sel] */
#define HID_USAGE_KEY_KEYBOARD_8_AND_ASTERISK (HID_USAGE(HID_USAGE_KEY, 0x25))

/* Keyboard/Keypad Page: Keyboard 9 and ( [Sel] */
#define HID_USAGE_KEY_KEYBOARD_9_AND_LEFT_PARENTHESIS (HID_USAGE(HID_USAGE_KEY, 0x26))

/* Keyboard/Keypad Page: Keyboard 0 and ) [Sel] */
#define HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS (HID_USAGE(HID_USAGE_KEY, 0x27))

/* Keyboard/Keypad Page: Keyboard Return (ENTER) [Sel] */
#define HID_USAGE_KEY_KEYBOARD_RETURN_ENTER (HID_USAGE(HID_USAGE_KEY, 0x28))

/* Keyboard/Keypad Page: Keyboard ESCAPE [Sel] */
#define HID_USAGE_KEY_KEYBOARD_ESCAPE (HID_USAGE(HID_USAGE_KEY, 0x29))

/* Keyboard/Keypad Page: Keyboard DELETE (Backspace) [Sel] */
#define HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE (HID_USAGE(HID_USAGE_KEY, 0x2A))

/* Keyboard/Keypad Page: Keyboard Tab [Sel] */
#define HID_USAGE_KEY_KEYBOARD_TAB (HID_USAGE(HID_USAGE_KEY, 0x2B))

/* Keyboard/Keypad Page: Keyboard Spacebar [Sel] */
#define HID_USAGE_KEY_KEYBOARD_SPACEBAR (HID_USAGE(HID_USAGE_KEY, 0x2C))

/* Keyboard/Keypad Page: Keyboard - and (underscore) [Sel] */
#define HID_USAGE_KEY_KEYBOARD_MINUS_AND_UNDERSCORE (HID_USAGE(HID_USAGE_KEY, 0x2D))

/* Keyboard/Keypad Page: Keyboard = and + [Sel] */
#define HID_USAGE_KEY_KEYBOARD_EQUAL_AND_PLUS (HID_USAGE(HID_USAGE_KEY, 0x2E))

/* Keyboard/Keypad Page: Keyboard [ and { [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LEFT_BRACKET_AND_LEFT_BRACE (HID_USAGE(HID_USAGE_KEY, 0x2F))

/* Keyboard/Keypad Page: Keyboard ] and } [Sel] */
#define HID_USAGE_KEY_KEYBOARD_RIGHT_BRACKET_AND_RIGHT_BRACE (HID_USAGE(HID_USAGE_KEY, 0x30))

/* Keyboard/Keypad Page: Keyboard \ and | [Sel] */
#define HID_USAGE_KEY_KEYBOARD_BACKSLASH_AND_PIPE (HID_USAGE(HID_USAGE_KEY, 0x31))

/* Keyboard/Keypad Page: Keyboard Non-US # and ˜ [Sel] */
#define HID_USAGE_KEY_KEYBOARD_NON_US_HASH_AND_TILDE (HID_USAGE(HID_USAGE_KEY, 0x32))

/* Keyboard/Keypad Page: Keyboard ; and : [Sel] */
#define HID_USAGE_KEY_KEYBOARD_SEMICOLON_AND_COLON (HID_USAGE(HID_USAGE_KEY, 0x33))

/* Keyboard/Keypad Page: Keyboard ‘ and “ [Sel] */
#define HID_USAGE_KEY_KEYBOARD_APOSTROPHE_AND_QUOTE (HID_USAGE(HID_USAGE_KEY, 0x34))

/* Keyboard/Keypad Page: Keyboard Grave Accent and Tilde [Sel] */
#define HID_USAGE_KEY_KEYBOARD_GRAVE_ACCENT_AND_TILDE (HID_USAGE(HID_USAGE_KEY, 0x35))

/* Keyboard/Keypad Page: Keyboard , and < [Sel] */
#define HID_USAGE_KEY_KEYBOARD_COMMA_AND_LESS_THAN (HID_USAGE(HID_USAGE_KEY, 0x36))

/* Keyboard/Keypad Page: Keyboard . and > [Sel] */
#define HID_USAGE_KEY_KEYBOARD_PERIOD_AND_GREATER_THAN (HID_USAGE(HID_USAGE_KEY, 0x37))

/* Keyboard/Keypad Page: Keyboard / and ? [Sel] */
#define HID_USAGE_KEY_KEYBOARD_SLASH_AND_QUESTION_MARK (HID_USAGE(HID_USAGE_KEY, 0x38))

/* Keyboard/Keypad Page: Keyboard Caps Lock [Sel] */
#define HID_USAGE_KEY_KEYBOARD_CAPS_LOCK (HID_USAGE(HID_USAGE_KEY, 0x39))

/* Keyboard/Keypad Page: Keyboard F1 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F1 (HID_USAGE(HID_USAGE_KEY, 0x3A))

/* Keyboard/Keypad Page: Keyboard F2 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F2 (HID_USAGE(HID_USAGE_KEY, 0x3B))

/* Keyboard/Keypad Page: Keyboard F3 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F3 (HID_USAGE(HID_USAGE_KEY, 0x3C))

/* Keyboard/Keypad Page: Keyboard F4 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F4 (HID_USAGE(HID_USAGE_KEY, 0x3D))

/* Keyboard/Keypad Page: Keyboard F5 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F5 (HID_USAGE(HID_USAGE_KEY, 0x3E))

/* Keyboard/Keypad Page: Keyboard F6 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F6 (HID_USAGE(HID_USAGE_KEY, 0x3F))

/* Keyboard/Keypad Page: Keyboard F7 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F7 (HID_USAGE(HID_USAGE_KEY, 0x40))

/* Keyboard/Keypad Page: Keyboard F8 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F8 (HID_USAGE(HID_USAGE_KEY, 0x41))

/* Keyboard/Keypad Page: Keyboard F9 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F9 (HID_USAGE(HID_USAGE_KEY, 0x42))

/* Keyboard/Keypad Page: Keyboard F10 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F10 (HID_USAGE(HID_USAGE_KEY, 0x43))

/* Keyboard/Keypad Page: Keyboard F11 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F11 (HID_USAGE(HID_USAGE_KEY, 0x44))

/* Keyboard/Keypad Page: Keyboard F12 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F12 (HID_USAGE(HID_USAGE_KEY, 0x45))

/* Keyboard/Keypad Page: Keyboard PrintScreen [Sel] */
#define HID_USAGE_KEY_KEYBOARD_PRINTSCREEN (HID_USAGE(HID_USAGE_KEY, 0x46))

/* Keyboard/Keypad Page: Keyboard Scroll Lock [Sel] */
#define HID_USAGE_KEY_KEYBOARD_SCROLL_LOCK (HID_USAGE(HID_USAGE_KEY, 0x47))

/* Keyboard/Keypad Page: Keyboard Pause [Sel] */
#define HID_USAGE_KEY_KEYBOARD_PAUSE (HID_USAGE(HID_USAGE_KEY, 0x48))

/* Keyboard/Keypad Page: Keyboard Insert [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INSERT (HID_USAGE(HID_USAGE_KEY, 0x49))

/* Keyboard/Keypad Page: Keyboard Home [Sel] */
#define HID_USAGE_KEY_KEYBOARD_HOME (HID_USAGE(HID_USAGE_KEY, 0x4A))

/* Keyboard/Keypad Page: Keyboard PageUp [Sel] */
#define HID_USAGE_KEY_KEYBOARD_PAGEUP (HID_USAGE(HID_USAGE_KEY, 0x4B))

/* Keyboard/Keypad Page: Keyboard Delete Forward [Sel] */
#define HID_USAGE_KEY_KEYBOARD_DELETE_FORWARD (HID_USAGE(HID_USAGE_KEY, 0x4C))

/* Keyboard/Keypad Page: Keyboard End [Sel] */
#define HID_USAGE_KEY_KEYBOARD_END (HID_USAGE(HID_USAGE_KEY, 0x4D))

/* Keyboard/Keypad Page: Keyboard PageDown [Sel] */
#define HID_USAGE_KEY_KEYBOARD_PAGEDOWN (HID_USAGE(HID_USAGE_KEY, 0x4E))

/* Keyboard/Keypad Page: Keyboard RightArrow [Sel] */
#define HID_USAGE_KEY_KEYBOARD_RIGHTARROW (HID_USAGE(HID_USAGE_KEY, 0x4F))

/* Keyboard/Keypad Page: Keyboard LeftArrow [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LEFTARROW (HID_USAGE(HID_USAGE_KEY, 0x50))

/* Keyboard/Keypad Page: Keyboard DownArrow [Sel] */
#define HID_USAGE_KEY_KEYBOARD_DOWNARROW (HID_USAGE(HID_USAGE_KEY, 0x51))

/* Keyboard/Keypad Page: Keyboard UpArrow [Sel] */
#define HID_USAGE_KEY_KEYBOARD_UPARROW (HID_USAGE(HID_USAGE_KEY, 0x52))

/* Keyboard/Keypad Page: Keypad Num Lock and Clear [Sel] */
#define HID_USAGE_KEY_KEYPAD_NUM_LOCK_AND_CLEAR (HID_USAGE(HID_USAGE_KEY, 0x53))

/* Keyboard/Keypad Page: Keypad / [Sel] */
#define HID_USAGE_KEY_KEYPAD_SLASH (HID_USAGE(HID_USAGE_KEY, 0x54))

/* Keyboard/Keypad Page: Keypad * [Sel] */
#define HID_USAGE_KEY_KEYPAD_ASTERISK (HID_USAGE(HID_USAGE_KEY, 0x55))

/* Keyboard/Keypad Page: Keypad - [Sel] */
#define HID_USAGE_KEY_KEYPAD_MINUS (HID_USAGE(HID_USAGE_KEY, 0x56))

/* Keyboard/Keypad Page: Keypad + [Sel] */
#define HID_USAGE_KEY_KEYPAD_PLUS (HID_USAGE(HID_USAGE_KEY, 0x57))

/* Keyboard/Keypad Page: Keypad ENTER [Sel] */
#define HID_USAGE_KEY_KEYPAD_ENTER (HID_USAGE(HID_USAGE_KEY, 0x58))

/* Keyboard/Keypad Page: Keypad 1 and End [Sel] */
#define HID_USAGE_KEY_KEYPAD_1_AND_END (HID_USAGE(HID_USAGE_KEY, 0x59))

/* Keyboard/Keypad Page: Keypad 2 and Down Arrow [Sel] */
#define HID_USAGE_KEY_KEYPAD_2_AND_DOWN_ARROW (HID_USAGE(HID_USAGE_KEY, 0x5A))

/* Keyboard/Keypad Page: Keypad 3 and PageDn [Sel] */
#define HID_USAGE_KEY_KEYPAD_3_AND_PAGEDN (HID_USAGE(HID_USAGE_KEY, 0x5B))

/* Keyboard/Keypad Page: Keypad 4 and Left Arrow [Sel] */
#define HID_USAGE_KEY_KEYPAD_4_AND_LEFT_ARROW (HID_USAGE(HID_USAGE_KEY, 0x5C))

/* Keyboard/Keypad Page: Keypad 5 [Sel] */
#define HID_USAGE_KEY_KEYPAD_5 (HID_USAGE(HID_USAGE_KEY, 0x5D))

/* Keyboard/Keypad Page: Keypad 6 and Right Arrow [Sel] */
#define HID_USAGE_KEY_KEYPAD_6_AND_RIGHT_ARROW (HID_USAGE(HID_USAGE_KEY, 0x5E))

/* Keyboard/Keypad Page: Keypad 7 and Home [Sel] */
#define HID_USAGE_KEY_KEYPAD_7_AND_HOME (HID_USAGE(HID_USAGE_KEY, 0x5F))

/* Keyboard/Keypad Page: Keypad 8 and Up Arrow [Sel] */
#define HID_USAGE_KEY_KEYPAD_8_AND_UP_ARROW (HID_USAGE(HID_USAGE_KEY, 0x60))

/* Keyboard/Keypad Page: Keypad 9 and PageUp [Sel] */
#define HID_USAGE_KEY_KEYPAD_9_AND_PAGEUP (HID_USAGE(HID_USAGE_KEY, 0x61))

/* Keyboard/Keypad Page: Keypad 0 and Insert [Sel] */
#define HID_USAGE_KEY_KEYPAD_0_AND_INSERT (HID_USAGE(HID_USAGE_KEY, 0x62))

/* Keyboard/Keypad Page: Keypad . and Delete [Sel] */
#define HID_USAGE_KEY_KEYPAD_PERIOD_AND_DELETE (HID_USAGE(HID_USAGE_KEY, 0x63))

/* Keyboard/Keypad Page: Keyboard Non-US \ and | [Sel] */
#define HID_USAGE_KEY_KEYBOARD_NON_US_BACKSLASH_AND_PIPE (HID_USAGE(HID_USAGE_KEY, 0x64))

/* Keyboard/Keypad Page: Keyboard Application [Sel] */
#define HID_USAGE_KEY_KEYBOARD_APPLICATION (HID_USAGE(HID_USAGE_KEY, 0x65))

/* Keyboard/Keypad Page: Keyboard Power [Sel] */
#define HID_USAGE_KEY_KEYBOARD_POWER (HID_USAGE(HID_USAGE_KEY, 0x66))

/* Keyboard/Keypad Page: Keypad = [Sel] */
#define HID_USAGE_KEY_KEYPAD_EQUAL (HID_USAGE(HID_USAGE_KEY, 0x67))

/* Keyboard/Keypad Page: Keyboard F13 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F13 (HID_USAGE(HID_USAGE_KEY, 0x68))

/* Keyboard/Keypad Page: Keyboard F14 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F14 (HID_USAGE(HID_USAGE_KEY, 0x69))

/* Keyboard/Keypad Page: Keyboard F15 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F15 (HID_USAGE(HID_USAGE_KEY, 0x6A))

/* Keyboard/Keypad Page: Keyboard F16 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F16 (HID_USAGE(HID_USAGE_KEY, 0x6B))

/* Keyboard/Keypad Page: Keyboard F17 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F17 (HID_USAGE(HID_USAGE_KEY, 0x6C))

/* Keyboard/Keypad Page: Keyboard F18 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F18 (HID_USAGE(HID_USAGE_KEY, 0x6D))

/* Keyboard/Keypad Page: Keyboard F19 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F19 (HID_USAGE(HID_USAGE_KEY, 0x6E))

/* Keyboard/Keypad Page: Keyboard F20 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F20 (HID_USAGE(HID_USAGE_KEY, 0x6F))

/* Keyboard/Keypad Page: Keyboard F21 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F21 (HID_USAGE(HID_USAGE_KEY, 0x70))

/* Keyboard/Keypad Page: Keyboard F22 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F22 (HID_USAGE(HID_USAGE_KEY, 0x71))

/* Keyboard/Keypad Page: Keyboard F23 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F23 (HID_USAGE(HID_USAGE_KEY, 0x72))

/* Keyboard/Keypad Page: Keyboard F24 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_F24 (HID_USAGE(HID_USAGE_KEY, 0x73))

/* Keyboard/Keypad Page: Keyboard Execute [Sel] */
#define HID_USAGE_KEY_KEYBOARD_EXECUTE (HID_USAGE(HID_USAGE_KEY, 0x74))

/* Keyboard/Keypad Page: Keyboard Help [Sel] */
#define HID_USAGE_KEY_KEYBOARD_HELP (HID_USAGE(HID_USAGE_KEY, 0x75))

/* Keyboard/Keypad Page: Keyboard Menu [Sel] */
#define HID_USAGE_KEY_KEYBOARD_MENU (HID_USAGE(HID_USAGE_KEY, 0x76))

/* Keyboard/Keypad Page: Keyboard Select [Sel] */
#define HID_USAGE_KEY_KEYBOARD_SELECT (HID_USAGE(HID_USAGE_KEY, 0x77))

/* Keyboard/Keypad Page: Keyboard Stop [Sel] */
#define HID_USAGE_KEY_KEYBOARD_STOP (HID_USAGE(HID_USAGE_KEY, 0x78))

/* Keyboard/Keypad Page: Keyboard Again [Sel] */
#define HID_USAGE_KEY_KEYBOARD_AGAIN (HID_USAGE(HID_USAGE_KEY, 0x79))

/* Keyboard/Keypad Page: Keyboard Undo [Sel] */
#define HID_USAGE_KEY_KEYBOARD_UNDO (HID_USAGE(HID_USAGE_KEY, 0x7A))

/* Keyboard/Keypad Page: Keyboard Cut [Sel] */
#define HID_USAGE_KEY_KEYBOARD_CUT (HID_USAGE(HID_USAGE_KEY, 0x7B))

/* Keyboard/Keypad Page: Keyboard Copy [Sel] */
#define HID_USAGE_KEY_KEYBOARD_COPY (HID_USAGE(HID_USAGE_KEY, 0x7C))

/* Keyboard/Keypad Page: Keyboard Paste [Sel] */
#define HID_USAGE_KEY_KEYBOARD_PASTE (HID_USAGE(HID_USAGE_KEY, 0x7D))

/* Keyboard/Keypad Page: Keyboard Find [Sel] */
#define HID_USAGE_KEY_KEYBOARD_FIND (HID_USAGE(HID_USAGE_KEY, 0x7E))

/* Keyboard/Keypad Page: Keyboard Mute [Sel] */
#define HID_USAGE_KEY_KEYBOARD_MUTE (HID_USAGE(HID_USAGE_KEY, 0x7F))

/* Keyboard/Keypad Page: Keyboard Volume Up [Sel] */
#define HID_USAGE_KEY_KEYBOARD_VOLUME_UP (HID_USAGE(HID_USAGE_KEY, 0x80))

/* Keyboard/Keypad Page: Keyboard Volume Down [Sel] */
#define HID_USAGE_KEY_KEYBOARD_VOLUME_DOWN (HID_USAGE(HID_USAGE_KEY, 0x81))

/* Keyboard/Keypad Page: Keyboard Locking Caps Lock [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LOCKING_CAPS_LOCK (HID_USAGE(HID_USAGE_KEY, 0x82))

/* Keyboard/Keypad Page: Keyboard Locking Num Lock [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LOCKING_NUM_LOCK (HID_USAGE(HID_USAGE_KEY, 0x83))

/* Keyboard/Keypad Page: Keyboard Locking Scroll Lock [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LOCKING_SCROLL_LOCK (HID_USAGE(HID_USAGE_KEY, 0x84))

/* Keyboard/Keypad Page: Keypad Comma [Sel] */
#define HID_USAGE_KEY_KEYPAD_COMMA (HID_USAGE(HID_USAGE_KEY, 0x85))

/* Keyboard/Keypad Page: Keypad Equal Sign [Sel] */
#define HID_USAGE_KEY_KEYPAD_EQUAL_SIGN (HID_USAGE(HID_USAGE_KEY, 0x86))

/* Keyboard/Keypad Page: Keyboard International1 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL1 (HID_USAGE(HID_USAGE_KEY, 0x87))

/* Keyboard/Keypad Page: Keyboard International2 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL2 (HID_USAGE(HID_USAGE_KEY, 0x88))

/* Keyboard/Keypad Page: Keyboard International3 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL3 (HID_USAGE(HID_USAGE_KEY, 0x89))

/* Keyboard/Keypad Page: Keyboard International4 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL4 (HID_USAGE(HID_USAGE_KEY, 0x8A))

/* Keyboard/Keypad Page: Keyboard International5 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL5 (HID_USAGE(HID_USAGE_KEY, 0x8B))

/* Keyboard/Keypad Page: Keyboard International6 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL6 (HID_USAGE(HID_USAGE_KEY, 0x8C))

/* Keyboard/Keypad Page: Keyboard International7 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL7 (HID_USAGE(HID_USAGE_KEY, 0x8D))

/* Keyboard/Keypad Page: Keyboard International8 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL8 (HID_USAGE(HID_USAGE_KEY, 0x8E))

/* Keyboard/Keypad Page: Keyboard International9 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_INTERNATIONAL9 (HID_USAGE(HID_USAGE_KEY, 0x8F))

/* Keyboard/Keypad Page: Keyboard LANG1 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG1 (HID_USAGE(HID_USAGE_KEY, 0x90))

/* Keyboard/Keypad Page: Keyboard LANG2 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG2 (HID_USAGE(HID_USAGE_KEY, 0x91))

/* Keyboard/Keypad Page: Keyboard LANG3 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG3 (HID_USAGE(HID_USAGE_KEY, 0x92))

/* Keyboard/Keypad Page: Keyboard LANG4 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG4 (HID_USAGE(HID_USAGE_KEY, 0x93))

/* Keyboard/Keypad Page: Keyboard LANG5 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG5 (HID_USAGE(HID_USAGE_KEY, 0x94))

/* Keyboard/Keypad Page: Keyboard LANG6 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG6 (HID_USAGE(HID_USAGE_KEY, 0x95))

/* Keyboard/Keypad Page: Keyboard LANG7 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG7 (HID_USAGE(HID_USAGE_KEY, 0x96))

/* Keyboard/Keypad Page: Keyboard LANG8 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG8 (HID_USAGE(HID_USAGE_KEY, 0x97))

/* Keyboard/Keypad Page: Keyboard LANG9 [Sel] */
#define HID_USAGE_KEY_KEYBOARD_LANG9 (HID_USAGE(HID_USAGE_KEY, 0x98))

/* Keyboard/Keypad Page: Keyboard Alternate Erase [Sel] */
#define HID_USAGE_KEY_KEYBOARD_ALTERNATE_ERASE (HID_USAGE(HID_USAGE_KEY, 0x99))

/* Keyboard/Keypad Page: Keyboard SysReq/Attention [Sel] */
#define HID_USAGE_KEY_KEYBOARD_SYSREQ_ATTENTION (HID_USAGE(HID_USAGE_KEY, 0x9A))

/* Keyboard/Keypad Page: Keyboard Cancel [Sel] */
#define HID_USAGE_KEY_KEYBOARD_CANCEL (HID_USAGE(HID_USAGE_KEY, 0x9B))

/* Keyboard/Keypad Page: Keyboard Clear [Sel] */
#define HID_USAGE_KEY_KEYBOARD_CLEAR (HID_USAGE(HID_USAGE_KEY, 0x9C))

/* Keyboard/Keypad Page: Keyboard Prior [Sel] */
#define HID_USAGE_KEY_KEYBOARD_PRIOR (HID_USAGE(HID_USAGE_KEY, 0x9D))

/* Keyboard/Keypad Page: Keyboard Return [Sel] */
#define HID_USAGE_KEY_KEYBOARD_RETURN (HID_USAGE(HID_USAGE_KEY, 0x9E))

/* Keyboard/Keypad Page: Keyboard Separator [Sel] */
#define HID_USAGE_KEY_KEYBOARD_SEPARATOR (HID_USAGE(HID_USAGE_KEY, 0x9F))

/* Keyboard/Keypad Page: Keyboard Out [Sel] */
#define HID_USAGE_KEY_KEYBOARD_OUT (HID_USAGE(HID_USAGE_KEY, 0xA0))

/* Keyboard/Keypad Page: Keyboard Oper [Sel] */
#define HID_USAGE_KEY_KEYBOARD_OPER (HID_USAGE(HID_USAGE_KEY, 0xA1))

/* Keyboard/Keypad Page: Keyboard Clear/Again [Sel] */
#define HID_USAGE_KEY_KEYBOARD_CLEAR_AGAIN (HID_USAGE(HID_USAGE_KEY, 0xA2))

/* Keyboard/Keypad Page: Keyboard CrSel/Props [Sel] */
#define HID_USAGE_KEY_KEYBOARD_CRSEL_PROPS (HID_USAGE(HID_USAGE_KEY, 0xA3))

/* Keyboard/Keypad Page: Keyboard ExSel [Sel] */
#define HID_USAGE_KEY_KEYBOARD_EXSEL (HID_USAGE(HID_USAGE_KEY, 0xA4))

/* Keyboard/Keypad Page: Keypad 00 [Sel] */
#define HID_USAGE_KEY_KEYPAD_00 (HID_USAGE(HID_USAGE_KEY, 0xB0))

/* Keyboard/Keypad Page: Keypad 000 [Sel] */
#define HID_USAGE_KEY_KEYPAD_000 (HID_USAGE(HID_USAGE_KEY, 0xB1))

/* Keyboard/Keypad Page: Thousands Separator [Sel] */
#define HID_USAGE_KEY_THOUSANDS_SEPARATOR (HID_USAGE(HID_USAGE_KEY, 0xB2))

/* Keyboard/Keypad Page: Decimal Separator [Sel] */
#define HID_USAGE_KEY_DECIMAL_SEPARATOR (HID_USAGE(HID_USAGE_KEY, 0xB3))

/* Keyboard/Keypad Page: Currency Unit [Sel] */
#define HID_USAGE_KEY_CURRENCY_UNIT (HID_USAGE(HID_USAGE_KEY, 0xB4))

/* Keyboard/Keypad Page: Currency Sub-unit [Sel] */
#define HID_USAGE_KEY_CURRENCY_SUB_UNIT (HID_USAGE(HID_USAGE_KEY, 0xB5))

/* Keyboard/Keypad Page: Keypad ( [Sel] */
#define HID_USAGE_KEY_KEYPAD_LEFT_PARENTHESIS (HID_USAGE(HID_USAGE_KEY, 0xB6))

/* Keyboard/Keypad Page: Keypad ) [Sel] */
#define HID_USAGE_KEY_KEYPAD_RIGHT_PARENTHESIS (HID_USAGE(HID_USAGE_KEY, 0xB7))

/* Keyboard/Keypad Page: Keypad { [Sel] */
#define HID_USAGE_KEY_KEYPAD_LEFT_BRACE (HID_USAGE(HID_USAGE_KEY, 0xB8))

/* Keyboard/Keypad Page: Keypad } [Sel] */
#define HID_USAGE_KEY_KEYPAD_RIGHT_BRACE (HID_USAGE(HID_USAGE_KEY, 0xB9))

/* Keyboard/Keypad Page: Keypad Tab [Sel] */
#define HID_USAGE_KEY_KEYPAD_TAB (HID_USAGE(HID_USAGE_KEY, 0xBA))

/* Keyboard/Keypad Page: Keypad Backspace [Sel] */
#define HID_USAGE_KEY_KEYPAD_BACKSPACE (HID_USAGE(HID_USAGE_KEY, 0xBB))

/* Keyboard/Keypad Page: Keypad A [Sel] */
#define HID_USAGE_KEY_KEYPAD_A (HID_USAGE(HID_USAGE_KEY, 0xBC))

/* Keyboard/Keypad Page: Keypad B [Sel] */
#define HID_USAGE_KEY_KEYPAD_B (HID_USAGE(HID_USAGE_KEY, 0xBD))

/* Keyboard/Keypad Page: Keypad C [Sel] */
#define HID_USAGE_KEY_KEYPAD_C (HID_USAGE(HID_USAGE_KEY, 0xBE))

/* Keyboard/Keypad Page: Keypad D [Sel] */
#define HID_USAGE_KEY_KEYPAD_D (HID_USAGE(HID_USAGE_KEY, 0xBF))

/* Keyboard/Keypad Page: Keypad E [Sel] */
#define HID_USAGE_KEY_KEYPAD_E (HID_USAGE(HID_USAGE_KEY, 0xC0))

/* Keyboard/Keypad Page: Keypad F [Sel] */
#define HID_USAGE_KEY_KEYPAD_F (HID_USAGE(HID_USAGE_KEY, 0xC1))

/* Keyboard/Keypad Page: Keypad XOR [Sel] */
#define HID_USAGE_KEY_KEYPAD_XOR (HID_USAGE(HID_USAGE_KEY, 0xC2))

/* Keyboard/Keypad Page: Keypad ^ [Sel] */
#define HID_USAGE_KEY_KEYPAD_CARET (HID_USAGE(HID_USAGE_KEY, 0xC3))

/* Keyboard/Keypad Page: Keypad % [Sel] */
#define HID_USAGE_KEY_KEYPAD_PERCENT (HID_USAGE(HID_USAGE_KEY, 0xC4))

/* Keyboard/Keypad Page: Keypad < [Sel] */
#define HID_USAGE_KEY_KEYPAD_LESS_THAN (HID_USAGE(HID_USAGE_KEY, 0xC5))

/* Keyboard/Keypad Page: Keypad > [Sel] */
#define HID_USAGE_KEY_KEYPAD_GREATER_THAN (HID_USAGE(HID_USAGE_KEY, 0xC6))

/* Keyboard/Keypad Page: Keypad & [Sel] */
#define HID_USAGE_KEY_KEYPAD_AMPERSAND (HID_USAGE(HID_USAGE_KEY, 0xC7))

/* Keyboard/Keypad Page: Keypad && [Sel] */
#define HID_USAGE_KEY_KEYPAD_AMPERSAND_AMPERSAND (HID_USAGE(HID_USAGE_KEY, 0xC8))

/* Keyboard/Keypad Page: Keypad | [Sel] */
#define HID_USAGE_KEY_KEYPAD_PIPE (HID_USAGE(HID_USAGE_KEY, 0xC9))

/* Keyboard/Keypad Page: Keypad || [Sel] */
#define HID_USAGE_KEY_KEYPAD_PIPE_PIPE (HID_USAGE(HID_USAGE_KEY, 0xCA))

/* Keyboard/Keypad Page: Keypad : [Sel] */
#define HID_USAGE_KEY_KEYPAD_COLON (HID_USAGE(HID_USAGE_KEY, 0xCB))

/* Keyboard/Keypad Page: Keypad # [Sel] */
#define HID_USAGE_KEY_KEYPAD_HASH (HID_USAGE(HID_USAGE_KEY, 0xCC))

/* Keyboard/Keypad Page: Keypad Space [Sel] */
#define HID_USAGE_KEY_KEYPAD_SPACE (HID_USAGE(HID_USAGE_KEY, 0xCD))

/* Keyboard/Keypad Page: Keypad @ [Sel] */
#define HID_USAGE_KEY_KEYPAD_AT (HID_USAGE(HID_USAGE_KEY, 0xCE))

/* Keyboard/Keypad Page: Keypad ! [Sel] */
#define HID_USAGE_KEY_KEYPAD_EXCLAMATION (HID_USAGE(HID_USAGE_KEY, 0xCF))

/* Keyboard/Keypad Page: Keypad Memory Store [Sel] */
#define HID_USAGE_KEY_KEYPAD_MEMORY_STORE (HID_USAGE(HID_USAGE_KEY, 0xD0))

/* Keyboard/Keypad Page: Keypad Memory Recall [Sel] */
#define HID_USAGE_KEY_KEYPAD_MEMORY_RECALL (HID_USAGE(HID_USAGE_KEY, 0xD1))

/* Keyboard/Keypad Page: Keypad Memory Clear [Sel] */
#define HID_USAGE_KEY_KEYPAD_MEMORY_CLEAR (HID_USAGE(HID_USAGE_KEY, 0xD2))

/* Keyboard/Keypad Page: Keypad Memory Add [Sel] */
#define HID_USAGE_KEY_KEYPAD_MEMORY_ADD (HID_USAGE(HID_USAGE_KEY, 0xD3))

/* Keyboard/Keypad Page: Keypad Memory Subtract [Sel] */
#define HID_USAGE_KEY_KEYPAD_MEMORY_SUBTRACT (HID_USAGE(HID_USAGE_KEY, 0xD4))

/* Keyboard/Keypad Page: Keypad Memory Multiply [Sel] */
#define HID_USAGE_KEY_KEYPAD_MEMORY_MULTIPLY (HID_USAGE(HID_USAGE_KEY, 0xD5))

/* Keyboard/Keypad Page: Keypad Memory Divide [Sel] */
#define HID_USAGE_KEY_KEYPAD_MEMORY_DIVIDE (HID_USAGE(HID_USAGE_KEY, 0xD6))

/* Keyboard/Keypad Page: Keypad +/- [Sel] */
#define HID_USAGE_KEY_KEYPAD_PLUS_MINUS (HID_USAGE(HID_USAGE_KEY, 0xD7))

/* Keyboard/Keypad Page: Keypad Clear [Sel] */
#define HID_USAGE_KEY_KEYPAD_CLEAR (HID_USAGE(HID_USAGE_KEY, 0xD8))

/* Keyboard/Keypad Page: Keypad Clear Entry [Sel] */
#define HID_USAGE_KEY_KEYPAD_CLEAR_ENTRY (HID_USAGE(HID_USAGE_KEY, 0xD9))

/* Keyboard/Keypad Page: Keypad Binary [Sel] */
#define HID_USAGE_KEY_KEYPAD_BINARY (HID_USAGE(HID_USAGE_KEY, 0xDA))

/* Keyboard/Keypad Page: Keypad Octal [Sel] */
#define HID_USAGE_KEY_KEYPAD_OCTAL (HID_USAGE(HID_USAGE_KEY, 0xDB))

/* Keyboard/Keypad Page: Keypad Decimal [Sel] */
#define HID_USAGE_KEY_KEYPAD_DECIMAL (HID_USAGE(HID_USAGE_KEY, 0xDC))

/* Keyboard/Keypad Page: Keypad Hexadecimal [Sel] */
#define HID_USAGE_KEY_KEYPAD_HEXADECIMAL (HID_USAGE(HID_USAGE_KEY, 0xDD))

/* Keyboard/Keypad Page: Keyboard LeftControl [DV] */
#define HID_USAGE_KEY_KEYBOARD_LEFTCONTROL (HID_USAGE(HID_USAGE_KEY, 0xE0))

/* Keyboard/Keypad Page: Keyboard LeftShift [DV] */
#define HID_USAGE_KEY_KEYBOARD_LEFTSHIFT (HID_USAGE(HID_USAGE_KEY, 0xE1))

/* Keyboard/Keypad Page: Keyboard LeftAlt [DV] */
#define HID_USAGE_KEY_KEYBOARD_LEFTALT (HID_USAGE(HID_USAGE_KEY, 0xE2))

/* Keyboard/Keypad Page: Keyboard Left GUI [DV] */
#define HID_USAGE_KEY_KEYBOARD_LEFT_GUI (HID_USAGE(HID_USAGE_KEY, 0xE3))

/* Keyboard/Keypad Page: Keyboard RightControl [DV] */
#define HID_USAGE_KEY_KEYBOARD_RIGHTCONTROL (HID_USAGE(HID_USAGE_KEY, 0xE4))

/* Keyboard/Keypad Page: Keyboard RightShift [DV] */
#define HID_USAGE_KEY_KEYBOARD_RIGHTSHIFT (HID_USAGE(HID_USAGE_KEY, 0xE5))

/* Keyboard/Keypad Page: Keyboard RightAlt [DV] */
#define HID_USAGE_KEY_KEYBOARD_RIGHTALT (HID_USAGE(HID_USAGE_KEY, 0xE6))

/* Keyboard/Keypad Page: Keyboard Right GUI [DV] */
#define HID_USAGE_KEY_KEYBOARD_RIGHT_GUI (HID_USAGE(HID_USAGE_KEY, 0xE7))

/* LED Page */
#define HID_USAGE_LED (0x08)

/* LED Page: Undefined */
#define HID_USAGE_LED_UNDEFINED (HID_USAGE(HID_USAGE_LED, 0x00))

/* LED Page: Num Lock [OOC] */
#define HID_USAGE_LED_NUM_LOCK (HID_USAGE(HID_USAGE_LED, 0x01))

/* LED Page: Caps Lock [OOC] */
#define HID_USAGE_LED_CAPS_LOCK (HID_USAGE(HID_USAGE_LED, 0x02))

/* LED Page: Scroll Lock [OOC] */
#define HID_USAGE_LED_SCROLL_LOCK (HID_USAGE(HID_USAGE_LED, 0x03))

/* LED Page: Compose [OOC] */
#define HID_USAGE_LED_COMPOSE (HID_USAGE(HID_USAGE_LED, 0x04))

/* LED Page: Kana [OOC] */
#define HID_USAGE_LED_KANA (HID_USAGE(HID_USAGE_LED, 0x05))

/* LED Page: Power [OOC] */
#define HID_USAGE_LED_POWER (HID_USAGE(HID_USAGE_LED, 0x06))

/* LED Page: Shift [OOC] */
#define HID_USAGE_LED_SHIFT (HID_USAGE(HID_USAGE_LED, 0x07))

/* LED Page: Do Not Disturb [OOC] */
#define HID_USAGE_LED_DO_NOT_DISTURB (HID_USAGE(HID_USAGE_LED, 0x08))

/* LED Page: Mute [OOC] */
#define HID_USAGE_LED_MUTE (HID_USAGE(HID_USAGE_LED, 0x09))

/* LED Page: Tone Enable [OOC] */
#define HID_USAGE_LED_TONE_ENABLE (HID_USAGE(HID_USAGE_LED, 0x0A))

/* LED Page: High Cut Filter [OOC] */
#define HID_USAGE_LED_HIGH_CUT_FILTER (HID_USAGE(HID_USAGE_LED, 0x0B))

/* LED Page: Low Cut Filter [OOC] */
#define HID_USAGE_LED_LOW_CUT_FILTER (HID_USAGE(HID_USAGE_LED, 0x0C))

/* LED Page: Equalizer Enable [OOC] */
#define HID_USAGE_LED_EQUALIZER_ENABLE (HID_USAGE(HID_USAGE_LED, 0x0D))

/* LED Page: Sound Field On [OOC] */
#define HID_USAGE_LED_SOUND_FIELD_ON (HID_USAGE(HID_USAGE_LED, 0x0E))

/* LED Page: Surround On [OOC] */
#define HID_USAGE_LED_SURROUND_ON (HID_USAGE(HID_USAGE_LED, 0x0F))

/* LED Page: Repeat [OOC] */
#define HID_USAGE_LED_REPEAT (HID_USAGE(HID_USAGE_LED, 0x10))

/* LED Page: Stereo [OOC] */
#define HID_USAGE_LED_STEREO (HID_USAGE(HID_USAGE_LED, 0x11))

/* LED Page: Sampling Rate Detect [OOC] */
#define HID_USAGE_LED_SAMPLING_RATE_DETECT (HID_USAGE(HID_USAGE_LED, 0x12))

/* LED Page: Spinning [OOC] */
#define HID_USAGE_LED_SPINNING (HID_USAGE(HID_USAGE_LED, 0x13))

/* LED Page: CAV [OOC] */
#define HID_USAGE_LED_CAV (HID_USAGE(HID_USAGE_LED, 0x14))

/* LED Page: CLV [OOC] */
#define HID_USAGE_LED_CLV (HID_USAGE(HID_USAGE_LED, 0x15))

/* LED Page: Recording Format Detect [OOC] */
#define HID_USAGE_LED_RECORDING_FORMAT_DETECT (HID_USAGE(HID_USAGE_LED, 0x16))

/* LED Page: Off-Hook [OOC] */
#define HID_USAGE_LED_OFF_HOOK (HID_USAGE(HID_USAGE_LED, 0x17))

/* LED Page: Ring [OOC] */
#define HID_USAGE_LED_RING (HID_USAGE(HID_USAGE_LED, 0x18))

/* LED Page: Message Waiting [OOC] */
#define HID_USAGE_LED_MESSAGE_WAITING (HID_USAGE(HID_USAGE_LED, 0x19))

/* LED Page: Data Mode [OOC] */
#define HID_USAGE_LED_DATA_MODE (HID_USAGE(HID_USAGE_LED, 0x1A))

/* LED Page: Battery Operation [OOC] */
#define HID_USAGE_LED_BATTERY_OPERATION (HID_USAGE(HID_USAGE_LED, 0x1B))

/* LED Page: Battery OK [OOC] */
#define HID_USAGE_LED_BATTERY_OK (HID_USAGE(HID_USAGE_LED, 0x1C))

/* LED Page: Battery Low [OOC] */
#define HID_USAGE_LED_BATTERY_LOW (HID_USAGE(HID_USAGE_LED, 0x1D))

/* LED Page: Speaker [OOC] */
#define HID_USAGE_LED_SPEAKER (HID_USAGE(HID_USAGE_LED, 0x1E))

/* LED Page: Head Set [OOC] */
#define HID_USAGE_LED_HEAD_SET (HID_USAGE(HID_USAGE_LED, 0x1F))

/* LED Page: Hold [OOC] */
#define HID_USAGE_LED_HOLD (HID_USAGE(HID_USAGE_LED, 0x20))

/* LED Page: Microphone [OOC] */
#define HID_USAGE_LED_MICROPHONE (HID_USAGE(HID_USAGE_LED, 0x21))

/* LED Page: Coverage [OOC] */
#define HID_USAGE_LED_COVERAGE (HID_USAGE(HID_USAGE_LED, 0x22))

/* LED Page: Night Mode [OOC] */
#define HID_USAGE_LED_NIGHT_MODE (HID_USAGE(HID_USAGE_LED, 0x23))

/* LED Page: Send Calls [OOC] */
#define HID_USAGE_LED_SEND_CALLS (HID_USAGE(HID_USAGE_LED, 0x24))

/* LED Page: Call Pickup [OOC] */
#define HID_USAGE_LED_CALL_PICKUP (HID_USAGE(HID_USAGE_LED, 0x25))

/* LED Page: Conference [OOC] */
#define HID_USAGE_LED_CONFERENCE (HID_USAGE(HID_USAGE_LED, 0x26))

/* LED Page: Stand-by [OOC] */
#define HID_USAGE_LED_STAND_BY (HID_USAGE(HID_USAGE_LED, 0x27))

/* LED Page: Camera On [OOC] */
#define HID_USAGE_LED_CAMERA_ON (HID_USAGE(HID_USAGE_LED, 0x28))

/* LED Page: Camera Off [OOC] */
#define HID_USAGE_LED_CAMERA_OFF (HID_USAGE(HID_USAGE_LED, 0x29))

/* LED Page: On-Line [OOC] */
#define HID_USAGE_LED_ON_LINE (HID_USAGE(HID_USAGE_LED, 0x2A))

/* LED Page: Off-Line [OOC] */
#define HID_USAGE_LED_OFF_LINE (HID_USAGE(HID_USAGE_LED, 0x2B))

/* LED Page: Busy [OOC] */
#define HID_USAGE_LED_BUSY (HID_USAGE(HID_USAGE_LED, 0x2C))

/* LED Page: Ready [OOC] */
#define HID_USAGE_LED_READY (HID_USAGE(HID_USAGE_LED, 0x2D))

/* LED Page: Paper-Out [OOC] */
#define HID_USAGE_LED_PAPER_OUT (HID_USAGE(HID_USAGE_LED, 0x2E))

/* LED Page: Paper-Jam [OOC] */
#define HID_USAGE_LED_PAPER_JAM (HID_USAGE(HID_USAGE_LED, 0x2F))

/* LED Page: Remote [OOC] */
#define HID_USAGE_LED_REMOTE (HID_USAGE(HID_USAGE_LED, 0x30))

/* LED Page: Forward [OOC] */
#define HID_USAGE_LED_FORWARD (HID_USAGE(HID_USAGE_LED, 0x31))

/* LED Page: Reverse [OOC] */
#define HID_USAGE_LED_REVERSE (HID_USAGE(HID_USAGE_LED, 0x32))

/* LED Page: Stop [OOC] */
#define HID_USAGE_LED_STOP (HID_USAGE(HID_USAGE_LED, 0x33))

/* LED Page: Rewind [OOC] */
#define HID_USAGE_LED_REWIND (HID_USAGE(HID_USAGE_LED, 0x34))

/* LED Page: Fast Forward [OOC] */
#define HID_USAGE_LED_FAST_FORWARD (HID_USAGE(HID_USAGE_LED, 0x35))

/* LED Page: Play [OOC] */
#define HID_USAGE_LED_PLAY (HID_USAGE(HID_USAGE_LED, 0x36))

/* LED Page: Pause [OOC] */
#define HID_USAGE_LED_PAUSE (HID_USAGE(HID_USAGE_LED, 0x37))

/* LED Page: Record [OOC] */
#define HID_USAGE_LED_RECORD (HID_USAGE(HID_USAGE_LED, 0x38))

/* LED Page: Error [OOC] */
#define HID_USAGE_LED_ERROR (HID_USAGE(HID_USAGE_LED, 0x39))

/* LED Page: Usage Selected Indicator [US] */
#define HID_USAGE_LED_USAGE_SELECTED_INDICATOR (HID_USAGE(HID_USAGE_LED, 0x3A))

/* LED Page: Usage In Use Indicator [US] */
#define HID_USAGE_LED_USAGE_IN_USE_INDICATOR (HID_USAGE(HID_USAGE_LED, 0x3B))

/* LED Page: Usage Multi Mode Indicator [UM] */
#define HID_USAGE_LED_USAGE_MULTI_MODE_INDICATOR (HID_USAGE(HID_USAGE_LED, 0x3C))

/* LED Page: Indicator On [Sel] */
#define HID_USAGE_LED_INDICATOR_ON (HID_USAGE(HID_USAGE_LED, 0x3D))

/* LED Page: Indicator Flash [Sel] */
#define HID_USAGE_LED_INDICATOR_FLASH (HID_USAGE(HID_USAGE_LED, 0x3E))

/* LED Page: Indicator Slow Blink [Sel] */
#define HID_USAGE_LED_INDICATOR_SLOW_BLINK (HID_USAGE(HID_USAGE_LED, 0x3F))

/* LED Page: Indicator Fast Blink [Sel] */
#define HID_USAGE_LED_INDICATOR_FAST_BLINK (HID_USAGE(HID_USAGE_LED, 0x40))

/* LED Page: Indicator Off [Sel] */
#define HID_USAGE_LED_INDICATOR_OFF (HID_USAGE(HID_USAGE_LED, 0x41))

/* LED Page: Flash On Time [DV] */
#define HID_USAGE_LED_FLASH_ON_TIME (HID_USAGE(HID_USAGE_LED, 0x42))

/* LED Page: Slow Blink On Time [DV] */
#define HID_USAGE_LED_SLOW_BLINK_ON_TIME (HID_USAGE(HID_USAGE_LED, 0x43))

/* LED Page: Slow Blink Off Time [DV] */
#define HID_USAGE_LED_SLOW_BLINK_OFF_TIME (HID_USAGE(HID_USAGE_LED, 0x44))

/* LED Page: Fast Blink On Time [DV] */
#define HID_USAGE_LED_FAST_BLINK_ON_TIME (HID_USAGE(HID_USAGE_LED, 0x45))

/* LED Page: Fast Blink Off Time [DV] */
#define HID_USAGE_LED_FAST_BLINK_OFF_TIME (HID_USAGE(HID_USAGE_LED, 0x46))

/* LED Page: Usage Indicator Color [UM] */
#define HID_USAGE_LED_USAGE_INDICATOR_COLOR (HID_USAGE(HID_USAGE_LED, 0x47))

/* LED Page: Indicator Red [Sel] */
#define HID_USAGE_LED_INDICATOR_RED (HID_USAGE(HID_USAGE_LED, 0x48))

/* LED Page: Indicator Green [Sel] */
#define HID_USAGE_LED_INDICATOR_GREEN (HID_USAGE(HID_USAGE_LED, 0x49))

/* LED Page: Indicator Amber [Sel] */
#define HID_USAGE_LED_INDICATOR_AMBER (HID_USAGE(HID_USAGE_LED, 0x4A))

/* LED Page: Generic Indicator [OOC] */
#define HID_USAGE_LED_GENERIC_INDICATOR (HID_USAGE(HID_USAGE_LED, 0x4B))

/* LED Page: System Suspend [OOC] */
#define HID_USAGE_LED_SYSTEM_SUSPEND (HID_USAGE(HID_USAGE_LED, 0x4C))

/* LED Page: External Power Connected [OOC] */
#define HID_USAGE_LED_EXTERNAL_POWER_CONNECTED (HID_USAGE(HID_USAGE_LED, 0x4D))

/* LED Page: Indicator Blue [Sel] */
#define HID_USAGE_LED_INDICATOR_BLUE (HID_USAGE(HID_USAGE_LED, 0x4E))

/* LED Page: Indicator Orange [Sel] */
#define HID_USAGE_LED_INDICATOR_ORANGE (HID_USAGE(HID_USAGE_LED, 0x4F))

/* LED Page: Good Status [OOC] */
#define HID_USAGE_LED_GOOD_STATUS (HID_USAGE(HID_USAGE_LED, 0x50))

/* LED Page: Warning Status [OOC] */
#define HID_USAGE_LED_WARNING_STATUS (HID_USAGE(HID_USAGE_LED, 0x51))

/* LED Page: RGB LED [CL] */
#define HID_USAGE_LED_RGB_LED (HID_USAGE(HID_USAGE_LED, 0x52))

/* LED Page: Red LED Channel [DV] */
#define HID_USAGE_LED_RED_LED_CHANNEL (HID_USAGE(HID_USAGE_LED, 0x53))

/* LED Page: Blue LED Channel [DV] */
#define HID_USAGE_LED_BLUE_LED_CHANNEL (HID_USAGE(HID_USAGE_LED, 0x54))

/* LED Page: Green LED Channel [DV] */
#define HID_USAGE_LED_GREEN_LED_CHANNEL (HID_USAGE(HID_USAGE_LED, 0x55))

/* LED Page: LED Intensity [DV] */
#define HID_USAGE_LED_LED_INTENSITY (HID_USAGE(HID_USAGE_LED, 0x56))

/* LED Page: Player Indicator [NAry] */
#define HID_USAGE_LED_PLAYER_INDICATOR (HID_USAGE(HID_USAGE_LED, 0x60))

/* LED Page: Player 1 [Sel] */
#define HID_USAGE_LED_PLAYER_1 (HID_USAGE(HID_USAGE_LED, 0x61))

/* LED Page: Player 2 [Sel] */
#define HID_USAGE_LED_PLAYER_2 (HID_USAGE(HID_USAGE_LED, 0x62))

/* LED Page: Player 3 [Sel] */
#define HID_USAGE_LED_PLAYER_3 (HID_USAGE(HID_USAGE_LED, 0x63))

/* LED Page: Player 4 [Sel] */
#define HID_USAGE_LED_PLAYER_4 (HID_USAGE(HID_USAGE_LED, 0x64))

/* LED Page: Player 5 [Sel] */
#define HID_USAGE_LED_PLAYER_5 (HID_USAGE(HID_USAGE_LED, 0x65))

/* LED Page: Player 6 [Sel] */
#define HID_USAGE_LED_PLAYER_6 (HID_USAGE(HID_USAGE_LED, 0x66))

/* LED Page: Player 7 [Sel] */
#define HID_USAGE_LED_PLAYER_7 (HID_USAGE(HID_USAGE_LED, 0x67))

/* LED Page: Player 8 [Sel] */
#define HID_USAGE_LED_PLAYER_8 (HID_USAGE(HID_USAGE_LED, 0x68))

/* Telephony Device Page */
#define HID_USAGE_TELEPHONY (0x0B)

/* Telephony Device Page: Undefined */
#define HID_USAGE_TELEPHONY_UNDEFINED (HID_USAGE(HID_USAGE_TELEPHONY, 0x00))

/* Telephony Device Page: Phone [CA] */
#define HID_USAGE_TELEPHONY_PHONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x01))

/* Telephony Device Page: Answering Machine [CA] */
#define HID_USAGE_TELEPHONY_ANSWERING_MACHINE (HID_USAGE(HID_USAGE_TELEPHONY, 0x02))

/* Telephony Device Page: Message Controls [CL] */
#define HID_USAGE_TELEPHONY_MESSAGE_CONTROLS (HID_USAGE(HID_USAGE_TELEPHONY, 0x03))

/* Telephony Device Page: Handset [CL] */
#define HID_USAGE_TELEPHONY_HANDSET (HID_USAGE(HID_USAGE_TELEPHONY, 0x04))

/* Telephony Device Page: Headset [CL] */
#define HID_USAGE_TELEPHONY_HEADSET (HID_USAGE(HID_USAGE_TELEPHONY, 0x05))

/* Telephony Device Page: Telephony Key Pad [NAry] */
#define HID_USAGE_TELEPHONY_TELEPHONY_KEY_PAD (HID_USAGE(HID_USAGE_TELEPHONY, 0x06))

/* Telephony Device Page: Programmable Button [NAry] */
#define HID_USAGE_TELEPHONY_PROGRAMMABLE_BUTTON (HID_USAGE(HID_USAGE_TELEPHONY, 0x07))

/* Telephony Device Page: Hook Switch [OOC] */
#define HID_USAGE_TELEPHONY_HOOK_SWITCH (HID_USAGE(HID_USAGE_TELEPHONY, 0x20))

/* Telephony Device Page: Flash [MC] */
#define HID_USAGE_TELEPHONY_FLASH (HID_USAGE(HID_USAGE_TELEPHONY, 0x21))

/* Telephony Device Page: Feature [OSC] */
#define HID_USAGE_TELEPHONY_FEATURE (HID_USAGE(HID_USAGE_TELEPHONY, 0x22))

/* Telephony Device Page: Hold [OOC] */
#define HID_USAGE_TELEPHONY_HOLD (HID_USAGE(HID_USAGE_TELEPHONY, 0x23))

/* Telephony Device Page: Redial [OSC] */
#define HID_USAGE_TELEPHONY_REDIAL (HID_USAGE(HID_USAGE_TELEPHONY, 0x24))

/* Telephony Device Page: Transfer [OSC] */
#define HID_USAGE_TELEPHONY_TRANSFER (HID_USAGE(HID_USAGE_TELEPHONY, 0x25))

/* Telephony Device Page: Drop [OSC] */
#define HID_USAGE_TELEPHONY_DROP (HID_USAGE(HID_USAGE_TELEPHONY, 0x26))

/* Telephony Device Page: Park [OOC] */
#define HID_USAGE_TELEPHONY_PARK (HID_USAGE(HID_USAGE_TELEPHONY, 0x27))

/* Telephony Device Page: Forward Calls [OOC] */
#define HID_USAGE_TELEPHONY_FORWARD_CALLS (HID_USAGE(HID_USAGE_TELEPHONY, 0x28))

/* Telephony Device Page: Alternate Function [MC] */
#define HID_USAGE_TELEPHONY_ALTERNATE_FUNCTION (HID_USAGE(HID_USAGE_TELEPHONY, 0x29))

/* Telephony Device Page: Line [OSC, NAry] */
#define HID_USAGE_TELEPHONY_LINE (HID_USAGE(HID_USAGE_TELEPHONY, 0x2A))

/* Telephony Device Page: Speaker Phone [OOC] */
#define HID_USAGE_TELEPHONY_SPEAKER_PHONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x2B))

/* Telephony Device Page: Conference [OOC] */
#define HID_USAGE_TELEPHONY_CONFERENCE (HID_USAGE(HID_USAGE_TELEPHONY, 0x2C))

/* Telephony Device Page: Ring Enable [OOC] */
#define HID_USAGE_TELEPHONY_RING_ENABLE (HID_USAGE(HID_USAGE_TELEPHONY, 0x2D))

/* Telephony Device Page: Ring Select [OSC] */
#define HID_USAGE_TELEPHONY_RING_SELECT (HID_USAGE(HID_USAGE_TELEPHONY, 0x2E))

/* Telephony Device Page: Phone Mute [OOC] */
#define HID_USAGE_TELEPHONY_PHONE_MUTE (HID_USAGE(HID_USAGE_TELEPHONY, 0x2F))

/* Telephony Device Page: Caller ID [MC] */
#define HID_USAGE_TELEPHONY_CALLER_ID (HID_USAGE(HID_USAGE_TELEPHONY, 0x30))

/* Telephony Device Page: Send [OOC] */
#define HID_USAGE_TELEPHONY_SEND (HID_USAGE(HID_USAGE_TELEPHONY, 0x31))

/* Telephony Device Page: Speed Dial [OSC] */
#define HID_USAGE_TELEPHONY_SPEED_DIAL (HID_USAGE(HID_USAGE_TELEPHONY, 0x50))

/* Telephony Device Page: Store Number [OSC] */
#define HID_USAGE_TELEPHONY_STORE_NUMBER (HID_USAGE(HID_USAGE_TELEPHONY, 0x51))

/* Telephony Device Page: Recall Number [OSC] */
#define HID_USAGE_TELEPHONY_RECALL_NUMBER (HID_USAGE(HID_USAGE_TELEPHONY, 0x52))

/* Telephony Device Page: Phone Directory [OOC] */
#define HID_USAGE_TELEPHONY_PHONE_DIRECTORY (HID_USAGE(HID_USAGE_TELEPHONY, 0x53))

/* Telephony Device Page: Voice Mail [OOC] */
#define HID_USAGE_TELEPHONY_VOICE_MAIL (HID_USAGE(HID_USAGE_TELEPHONY, 0x70))

/* Telephony Device Page: Screen Calls [OOC] */
#define HID_USAGE_TELEPHONY_SCREEN_CALLS (HID_USAGE(HID_USAGE_TELEPHONY, 0x71))

/* Telephony Device Page: Do Not Disturb [OOC] */
#define HID_USAGE_TELEPHONY_DO_NOT_DISTURB (HID_USAGE(HID_USAGE_TELEPHONY, 0x72))

/* Telephony Device Page: Message [OSC] */
#define HID_USAGE_TELEPHONY_MESSAGE (HID_USAGE(HID_USAGE_TELEPHONY, 0x73))

/* Telephony Device Page: Answer On/Off [OOC] */
#define HID_USAGE_TELEPHONY_ANSWER_ON_OFF (HID_USAGE(HID_USAGE_TELEPHONY, 0x74))

/* Telephony Device Page: Inside Dial Tone [MC] */
#define HID_USAGE_TELEPHONY_INSIDE_DIAL_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x90))

/* Telephony Device Page: Outside Dial Tone [MC] */
#define HID_USAGE_TELEPHONY_OUTSIDE_DIAL_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x91))

/* Telephony Device Page: Inside Ring Tone [MC] */
#define HID_USAGE_TELEPHONY_INSIDE_RING_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x92))

/* Telephony Device Page: Outside Ring Tone [MC] */
#define HID_USAGE_TELEPHONY_OUTSIDE_RING_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x93))

/* Telephony Device Page: Priority Ring Tone [MC] */
#define HID_USAGE_TELEPHONY_PRIORITY_RING_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x94))

/* Telephony Device Page: Inside Ringback [MC] */
#define HID_USAGE_TELEPHONY_INSIDE_RINGBACK (HID_USAGE(HID_USAGE_TELEPHONY, 0x95))

/* Telephony Device Page: Priority Ringback [MC] */
#define HID_USAGE_TELEPHONY_PRIORITY_RINGBACK (HID_USAGE(HID_USAGE_TELEPHONY, 0x96))

/* Telephony Device Page: Line Busy Tone [MC] */
#define HID_USAGE_TELEPHONY_LINE_BUSY_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x97))

/* Telephony Device Page: Reorder Tone [MC] */
#define HID_USAGE_TELEPHONY_REORDER_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x98))

/* Telephony Device Page: Call Waiting Tone [MC] */
#define HID_USAGE_TELEPHONY_CALL_WAITING_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x99))

/* Telephony Device Page: Confirmation Tone 1 [MC] */
#define HID_USAGE_TELEPHONY_CONFIRMATION_TONE_1 (HID_USAGE(HID_USAGE_TELEPHONY, 0x9A))

/* Telephony Device Page: Confirmation Tone 2 [MC] */
#define HID_USAGE_TELEPHONY_CONFIRMATION_TONE_2 (HID_USAGE(HID_USAGE_TELEPHONY, 0x9B))

/* Telephony Device Page: Tones Off [OOC] */
#define HID_USAGE_TELEPHONY_TONES_OFF (HID_USAGE(HID_USAGE_TELEPHONY, 0x9C))

/* Telephony Device Page: Outside Ringback [MC] */
#define HID_USAGE_TELEPHONY_OUTSIDE_RINGBACK (HID_USAGE(HID_USAGE_TELEPHONY, 0x9D))

/* Telephony Device Page: Ringer [OOC] */
#define HID_USAGE_TELEPHONY_RINGER (HID_USAGE(HID_USAGE_TELEPHONY, 0x9E))

/* Telephony Device Page: Phone Key 0 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_0 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB0))

/* Telephony Device Page: Phone Key 1 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_1 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB1))

/* Telephony Device Page: Phone Key 2 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_2 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB2))

/* Telephony Device Page: Phone Key 3 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_3 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB3))

/* Telephony Device Page: Phone Key 4 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_4 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB4))

/* Telephony Device Page: Phone Key 5 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_5 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB5))

/* Telephony Device Page: Phone Key 6 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_6 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB6))

/* Telephony Device Page: Phone Key 7 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_7 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB7))

/* Telephony Device Page: Phone Key 8 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_8 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB8))

/* Telephony Device Page: Phone Key 9 [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_9 (HID_USAGE(HID_USAGE_TELEPHONY, 0xB9))

/* Telephony Device Page: Phone Key Star [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_STAR (HID_USAGE(HID_USAGE_TELEPHONY, 0xBA))

/* Telephony Device Page: Phone Key Pound [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_POUND (HID_USAGE(HID_USAGE_TELEPHONY, 0xBB))

/* Telephony Device Page: Phone Key A [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_A (HID_USAGE(HID_USAGE_TELEPHONY, 0xBC))

/* Telephony Device Page: Phone Key B [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_B (HID_USAGE(HID_USAGE_TELEPHONY, 0xBD))

/* Telephony Device Page: Phone Key C [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_C (HID_USAGE(HID_USAGE_TELEPHONY, 0xBE))

/* Telephony Device Page: Phone Key D [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_KEY_D (HID_USAGE(HID_USAGE_TELEPHONY, 0xBF))

/* Telephony Device Page: Phone Call History Key [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_CALL_HISTORY_KEY (HID_USAGE(HID_USAGE_TELEPHONY, 0xC0))

/* Telephony Device Page: Phone Caller ID Key [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_CALLER_ID_KEY (HID_USAGE(HID_USAGE_TELEPHONY, 0xC1))

/* Telephony Device Page: Phone Settings Key [Sel] */
#define HID_USAGE_TELEPHONY_PHONE_SETTINGS_KEY (HID_USAGE(HID_USAGE_TELEPHONY, 0xC2))

/* Telephony Device Page: Host Control [OOC] */
#define HID_USAGE_TELEPHONY_HOST_CONTROL (HID_USAGE(HID_USAGE_TELEPHONY, 0xF0))

/* Telephony Device Page: Host Available [OOC] */
#define HID_USAGE_TELEPHONY_HOST_AVAILABLE (HID_USAGE(HID_USAGE_TELEPHONY, 0xF1))

/* Telephony Device Page: Host Call Active [OOC] */
#define HID_USAGE_TELEPHONY_HOST_CALL_ACTIVE (HID_USAGE(HID_USAGE_TELEPHONY, 0xF2))

/* Telephony Device Page: Activate Handset Audio [OOC] */
#define HID_USAGE_TELEPHONY_ACTIVATE_HANDSET_AUDIO (HID_USAGE(HID_USAGE_TELEPHONY, 0xF3))

/* Telephony Device Page: Ring Type [NAry] */
#define HID_USAGE_TELEPHONY_RING_TYPE (HID_USAGE(HID_USAGE_TELEPHONY, 0xF4))

/* Telephony Device Page: Re-dialable Phone Number [OOC] */
#define HID_USAGE_TELEPHONY_RE_DIALABLE_PHONE_NUMBER (HID_USAGE(HID_USAGE_TELEPHONY, 0xF5))

/* Telephony Device Page: Stop Ring Tone [Sel] */
#define HID_USAGE_TELEPHONY_STOP_RING_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0xF8))

/* Telephony Device Page: PSTN Ring Tone [Sel] */
#define HID_USAGE_TELEPHONY_PSTN_RING_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0xF9))

/* Telephony Device Page: Host Ring Tone [Sel] */
#define HID_USAGE_TELEPHONY_HOST_RING_TONE (HID_USAGE(HID_USAGE_TELEPHONY, 0xFA))

/* Telephony Device Page: Alert Sound Error [Sel] */
#define HID_USAGE_TELEPHONY_ALERT_SOUND_ERROR (HID_USAGE(HID_USAGE_TELEPHONY, 0xFB))

/* Telephony Device Page: Alert Sound Confirm [Sel] */
#define HID_USAGE_TELEPHONY_ALERT_SOUND_CONFIRM (HID_USAGE(HID_USAGE_TELEPHONY, 0xFC))

/* Telephony Device Page: Alert Sound Notification [Sel] */
#define HID_USAGE_TELEPHONY_ALERT_SOUND_NOTIFICATION (HID_USAGE(HID_USAGE_TELEPHONY, 0xFD))

/* Telephony Device Page: Silent Ring [Sel] */
#define HID_USAGE_TELEPHONY_SILENT_RING (HID_USAGE(HID_USAGE_TELEPHONY, 0xFE))

/* Telephony Device Page: Email Message Waiting [OOC] */
#define HID_USAGE_TELEPHONY_EMAIL_MESSAGE_WAITING (HID_USAGE(HID_USAGE_TELEPHONY, 0x108))

/* Telephony Device Page: Voicemail Message Waiting [OOC] */
#define HID_USAGE_TELEPHONY_VOICEMAIL_MESSAGE_WAITING (HID_USAGE(HID_USAGE_TELEPHONY, 0x109))

/* Telephony Device Page: Host Hold [OOC] */
#define HID_USAGE_TELEPHONY_HOST_HOLD (HID_USAGE(HID_USAGE_TELEPHONY, 0x10A))

/* Telephony Device Page: Incoming Call History Count [DV] */
#define HID_USAGE_TELEPHONY_INCOMING_CALL_HISTORY_COUNT (HID_USAGE(HID_USAGE_TELEPHONY, 0x110))

/* Telephony Device Page: Outgoing Call History Count [DV] */
#define HID_USAGE_TELEPHONY_OUTGOING_CALL_HISTORY_COUNT (HID_USAGE(HID_USAGE_TELEPHONY, 0x111))

/* Telephony Device Page: Incoming Call History [CL] */
#define HID_USAGE_TELEPHONY_INCOMING_CALL_HISTORY (HID_USAGE(HID_USAGE_TELEPHONY, 0x112))

/* Telephony Device Page: Outgoing Call History [CL] */
#define HID_USAGE_TELEPHONY_OUTGOING_CALL_HISTORY (HID_USAGE(HID_USAGE_TELEPHONY, 0x113))

/* Telephony Device Page: Phone Locale [DV] */
#define HID_USAGE_TELEPHONY_PHONE_LOCALE (HID_USAGE(HID_USAGE_TELEPHONY, 0x114))

/* Telephony Device Page: Phone Time Second [DV] */
#define HID_USAGE_TELEPHONY_PHONE_TIME_SECOND (HID_USAGE(HID_USAGE_TELEPHONY, 0x140))

/* Telephony Device Page: Phone Time Minute [DV] */
#define HID_USAGE_TELEPHONY_PHONE_TIME_MINUTE (HID_USAGE(HID_USAGE_TELEPHONY, 0x141))

/* Telephony Device Page: Phone Time Hour [DV] */
#define HID_USAGE_TELEPHONY_PHONE_TIME_HOUR (HID_USAGE(HID_USAGE_TELEPHONY, 0x142))

/* Telephony Device Page: Phone Date Day [DV] */
#define HID_USAGE_TELEPHONY_PHONE_DATE_DAY (HID_USAGE(HID_USAGE_TELEPHONY, 0x143))

/* Telephony Device Page: Phone Date Month [DV] */
#define HID_USAGE_TELEPHONY_PHONE_DATE_MONTH (HID_USAGE(HID_USAGE_TELEPHONY, 0x144))

/* Telephony Device Page: Phone Date Year [DV] */
#define HID_USAGE_TELEPHONY_PHONE_DATE_YEAR (HID_USAGE(HID_USAGE_TELEPHONY, 0x145))

/* Telephony Device Page: Handset Nickname [DV] */
#define HID_USAGE_TELEPHONY_HANDSET_NICKNAME (HID_USAGE(HID_USAGE_TELEPHONY, 0x146))

/* Telephony Device Page: Address Book ID [DV] */
#define HID_USAGE_TELEPHONY_ADDRESS_BOOK_ID (HID_USAGE(HID_USAGE_TELEPHONY, 0x147))

/* Telephony Device Page: Call Duration [DV] */
#define HID_USAGE_TELEPHONY_CALL_DURATION (HID_USAGE(HID_USAGE_TELEPHONY, 0x14A))

/* Telephony Device Page: Dual Mode Phone [CA] */
#define HID_USAGE_TELEPHONY_DUAL_MODE_PHONE (HID_USAGE(HID_USAGE_TELEPHONY, 0x14B))

/* Consumer Page */
#define HID_USAGE_CONSUMER (0x0C)

/* Consumer Page: Undefined */
#define HID_USAGE_CONSUMER_UNDEFINED (HID_USAGE(HID_USAGE_CONSUMER, 0x00))

/* Consumer Page: Consumer Control [CA] */
#define HID_USAGE_CONSUMER_CONSUMER_CONTROL (HID_USAGE(HID_USAGE_CONSUMER, 0x01))

/* Consumer Page: Numeric Key Pad [NAry] */
#define HID_USAGE_CONSUMER_NUMERIC_KEY_PAD (HID_USAGE(HID_USAGE_CONSUMER, 0x02))

/* Consumer Page: Programmable Buttons [NAry] */
#define HID_USAGE_CONSUMER_PROGRAMMABLE_BUTTONS (HID_USAGE(HID_USAGE_CONSUMER, 0x03))

/* Consumer Page: Microphone [CA] */
#define HID_USAGE_CONSUMER_MICROPHONE (HID_USAGE(HID_USAGE_CONSUMER, 0x04))

/* Consumer Page: Headphone [CA] */
#define HID_USAGE_CONSUMER_HEADPHONE (HID_USAGE(HID_USAGE_CONSUMER, 0x05))

/* Consumer Page: Graphic Equalizer [CA] */
#define HID_USAGE_CONSUMER_GRAPHIC_EQUALIZER (HID_USAGE(HID_USAGE_CONSUMER, 0x06))

/* Consumer Page: +10 [OSC] */
#define HID_USAGE_CONSUMER_INCREMENT10 (HID_USAGE(HID_USAGE_CONSUMER, 0x20))

/* Consumer Page: +100 [OSC] */
#define HID_USAGE_CONSUMER_INCREMENT100 (HID_USAGE(HID_USAGE_CONSUMER, 0x21))

/* Consumer Page: AM/PM [OSC] */
#define HID_USAGE_CONSUMER_AM_PM (HID_USAGE(HID_USAGE_CONSUMER, 0x22))

/* Consumer Page: Power [OOC] */
#define HID_USAGE_CONSUMER_POWER (HID_USAGE(HID_USAGE_CONSUMER, 0x30))

/* Consumer Page: Reset [OSC] */
#define HID_USAGE_CONSUMER_RESET (HID_USAGE(HID_USAGE_CONSUMER, 0x31))

/* Consumer Page: Sleep [OSC] */
#define HID_USAGE_CONSUMER_SLEEP (HID_USAGE(HID_USAGE_CONSUMER, 0x32))

/* Consumer Page: Sleep After [OSC] */
#define HID_USAGE_CONSUMER_SLEEP_AFTER (HID_USAGE(HID_USAGE_CONSUMER, 0x33))

/* Consumer Page: Sleep Mode [RTC] */
#define HID_USAGE_CONSUMER_SLEEP_MODE (HID_USAGE(HID_USAGE_CONSUMER, 0x34))

/* Consumer Page: Illumination [OOC] */
#define HID_USAGE_CONSUMER_ILLUMINATION (HID_USAGE(HID_USAGE_CONSUMER, 0x35))

/* Consumer Page: Function Buttons [NAry] */
#define HID_USAGE_CONSUMER_FUNCTION_BUTTONS (HID_USAGE(HID_USAGE_CONSUMER, 0x36))

/* Consumer Page: Menu [OOC] */
#define HID_USAGE_CONSUMER_MENU (HID_USAGE(HID_USAGE_CONSUMER, 0x40))

/* Consumer Page: Menu Pick [OSC] */
#define HID_USAGE_CONSUMER_MENU_PICK (HID_USAGE(HID_USAGE_CONSUMER, 0x41))

/* Consumer Page: Menu Up [OSC] */
#define HID_USAGE_CONSUMER_MENU_UP (HID_USAGE(HID_USAGE_CONSUMER, 0x42))

/* Consumer Page: Menu Down [OSC] */
#define HID_USAGE_CONSUMER_MENU_DOWN (HID_USAGE(HID_USAGE_CONSUMER, 0x43))

/* Consumer Page: Menu Left [OSC] */
#define HID_USAGE_CONSUMER_MENU_LEFT (HID_USAGE(HID_USAGE_CONSUMER, 0x44))

/* Consumer Page: Menu Right [OSC] */
#define HID_USAGE_CONSUMER_MENU_RIGHT (HID_USAGE(HID_USAGE_CONSUMER, 0x45))

/* Consumer Page: Menu Escape [OSC] */
#define HID_USAGE_CONSUMER_MENU_ESCAPE (HID_USAGE(HID_USAGE_CONSUMER, 0x46))

/* Consumer Page: Menu Value Increase [OSC] */
#define HID_USAGE_CONSUMER_MENU_VALUE_INCREASE (HID_USAGE(HID_USAGE_CONSUMER, 0x47))

/* Consumer Page: Menu Value Decrease [OSC] */
#define HID_USAGE_CONSUMER_MENU_VALUE_DECREASE (HID_USAGE(HID_USAGE_CONSUMER, 0x48))

/* Consumer Page: Data On Screen [OOC] */
#define HID_USAGE_CONSUMER_DATA_ON_SCREEN (HID_USAGE(HID_USAGE_CONSUMER, 0x60))

/* Consumer Page: Closed Caption [OOC] */
#define HID_USAGE_CONSUMER_CLOSED_CAPTION (HID_USAGE(HID_USAGE_CONSUMER, 0x61))

/* Consumer Page: Closed Caption Select [OSC] */
#define HID_USAGE_CONSUMER_CLOSED_CAPTION_SELECT (HID_USAGE(HID_USAGE_CONSUMER, 0x62))

/* Consumer Page: VCR/TV [OOC] */
#define HID_USAGE_CONSUMER_VCR_TV (HID_USAGE(HID_USAGE_CONSUMER, 0x63))

/* Consumer Page: Broadcast Mode [OSC] */
#define HID_USAGE_CONSUMER_BROADCAST_MODE (HID_USAGE(HID_USAGE_CONSUMER, 0x64))

/* Consumer Page: Snapshot [OSC] */
#define HID_USAGE_CONSUMER_SNAPSHOT (HID_USAGE(HID_USAGE_CONSUMER, 0x65))

/* Consumer Page: Still [OSC] */
#define HID_USAGE_CONSUMER_STILL (HID_USAGE(HID_USAGE_CONSUMER, 0x66))

/* Consumer Page: Picture-in-Picture Toggle [OSC] */
#define HID_USAGE_CONSUMER_PICTURE_IN_PICTURE_TOGGLE (HID_USAGE(HID_USAGE_CONSUMER, 0x67))

/* Consumer Page: Picture-in-Picture Swap [OSC] */
#define HID_USAGE_CONSUMER_PICTURE_IN_PICTURE_SWAP (HID_USAGE(HID_USAGE_CONSUMER, 0x68))

/* Consumer Page: Red Menu Button [MC] */
#define HID_USAGE_CONSUMER_RED_MENU_BUTTON (HID_USAGE(HID_USAGE_CONSUMER, 0x69))

/* Consumer Page: Green Menu Button [MC] */
#define HID_USAGE_CONSUMER_GREEN_MENU_BUTTON (HID_USAGE(HID_USAGE_CONSUMER, 0x6A))

/* Consumer Page: Blue Menu Button [MC] */
#define HID_USAGE_CONSUMER_BLUE_MENU_BUTTON (HID_USAGE(HID_USAGE_CONSUMER, 0x6B))

/* Consumer Page: Yellow Menu Button [MC] */
#define HID_USAGE_CONSUMER_YELLOW_MENU_BUTTON (HID_USAGE(HID_USAGE_CONSUMER, 0x6C))

/* Consumer Page: Aspect [OSC] */
#define HID_USAGE_CONSUMER_ASPECT (HID_USAGE(HID_USAGE_CONSUMER, 0x6D))

/* Consumer Page: 3D Mode Select [OSC] */
#define HID_USAGE_CONSUMER_3D_MODE_SELECT (HID_USAGE(HID_USAGE_CONSUMER, 0x6E))

/* Consumer Page: Display Brightness Increment [RTC] */
#define HID_USAGE_CONSUMER_DISPLAY_BRIGHTNESS_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x6F))

/* Consumer Page: Display Brightness Decrement [RTC] */
#define HID_USAGE_CONSUMER_DISPLAY_BRIGHTNESS_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x70))

/* Consumer Page: Display Brightness [LC] */
#define HID_USAGE_CONSUMER_DISPLAY_BRIGHTNESS (HID_USAGE(HID_USAGE_CONSUMER, 0x71))

/* Consumer Page: Display Backlight Toggle [OOC] */
#define HID_USAGE_CONSUMER_DISPLAY_BACKLIGHT_TOGGLE (HID_USAGE(HID_USAGE_CONSUMER, 0x72))

/* Consumer Page: Display Set Brightness to Minimum [OSC] */
#define HID_USAGE_CONSUMER_DISPLAY_SET_BRIGHTNESS_TO_MINIMUM (HID_USAGE(HID_USAGE_CONSUMER, 0x73))

/* Consumer Page: Display Set Brightness to Maximum [OSC] */
#define HID_USAGE_CONSUMER_DISPLAY_SET_BRIGHTNESS_TO_MAXIMUM (HID_USAGE(HID_USAGE_CONSUMER, 0x74))

/* Consumer Page: Display Set Auto Brightness [OOC] */
#define HID_USAGE_CONSUMER_DISPLAY_SET_AUTO_BRIGHTNESS (HID_USAGE(HID_USAGE_CONSUMER, 0x75))

/* Consumer Page: Camera Access Enabled [OOC] */
#define HID_USAGE_CONSUMER_CAMERA_ACCESS_ENABLED (HID_USAGE(HID_USAGE_CONSUMER, 0x76))

/* Consumer Page: Camera Access Disabled [OOC] */
#define HID_USAGE_CONSUMER_CAMERA_ACCESS_DISABLED (HID_USAGE(HID_USAGE_CONSUMER, 0x77))

/* Consumer Page: Camera Access Toggle [OOC] */
#define HID_USAGE_CONSUMER_CAMERA_ACCESS_TOGGLE (HID_USAGE(HID_USAGE_CONSUMER, 0x78))

/* Consumer Page: Keyboard Brightness Increment [OSC] */
#define HID_USAGE_CONSUMER_KEYBOARD_BRIGHTNESS_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x79))

/* Consumer Page: Keyboard Brightness Decrement [OSC] */
#define HID_USAGE_CONSUMER_KEYBOARD_BRIGHTNESS_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x7A))

/* Consumer Page: Keyboard Backlight Set Level [LC] */
#define HID_USAGE_CONSUMER_KEYBOARD_BACKLIGHT_SET_LEVEL (HID_USAGE(HID_USAGE_CONSUMER, 0x7B))

/* Consumer Page: Keyboard Backlight OOC [OOC] */
#define HID_USAGE_CONSUMER_KEYBOARD_BACKLIGHT_OOC (HID_USAGE(HID_USAGE_CONSUMER, 0x7C))

/* Consumer Page: Keyboard Backlight Set Minimum [OSC] */
#define HID_USAGE_CONSUMER_KEYBOARD_BACKLIGHT_SET_MINIMUM (HID_USAGE(HID_USAGE_CONSUMER, 0x7D))

/* Consumer Page: Keyboard Backlight Set Maximum [OSC] */
#define HID_USAGE_CONSUMER_KEYBOARD_BACKLIGHT_SET_MAXIMUM (HID_USAGE(HID_USAGE_CONSUMER, 0x7E))

/* Consumer Page: Keyboard Backlight Auto [OOC] */
#define HID_USAGE_CONSUMER_KEYBOARD_BACKLIGHT_AUTO (HID_USAGE(HID_USAGE_CONSUMER, 0x7F))

/* Consumer Page: Selection [NAry] */
#define HID_USAGE_CONSUMER_SELECTION (HID_USAGE(HID_USAGE_CONSUMER, 0x80))

/* Consumer Page: Assign Selection [OSC] */
#define HID_USAGE_CONSUMER_ASSIGN_SELECTION (HID_USAGE(HID_USAGE_CONSUMER, 0x81))

/* Consumer Page: Mode Step [OSC] */
#define HID_USAGE_CONSUMER_MODE_STEP (HID_USAGE(HID_USAGE_CONSUMER, 0x82))

/* Consumer Page: Recall Last [OSC] */
#define HID_USAGE_CONSUMER_RECALL_LAST (HID_USAGE(HID_USAGE_CONSUMER, 0x83))

/* Consumer Page: Enter Channel [OSC] */
#define HID_USAGE_CONSUMER_ENTER_CHANNEL (HID_USAGE(HID_USAGE_CONSUMER, 0x84))

/* Consumer Page: Order Movie [OSC] */
#define HID_USAGE_CONSUMER_ORDER_MOVIE (HID_USAGE(HID_USAGE_CONSUMER, 0x85))

/* Consumer Page: Channel [LC] */
#define HID_USAGE_CONSUMER_CHANNEL (HID_USAGE(HID_USAGE_CONSUMER, 0x86))

/* Consumer Page: Media Selection [NAry] */
#define HID_USAGE_CONSUMER_MEDIA_SELECTION (HID_USAGE(HID_USAGE_CONSUMER, 0x87))

/* Consumer Page: Media Select Computer [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_COMPUTER (HID_USAGE(HID_USAGE_CONSUMER, 0x88))

/* Consumer Page: Media Select TV [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_TV (HID_USAGE(HID_USAGE_CONSUMER, 0x89))

/* Consumer Page: Media Select WWW [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_WWW (HID_USAGE(HID_USAGE_CONSUMER, 0x8A))

/* Consumer Page: Media Select DVD [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_DVD (HID_USAGE(HID_USAGE_CONSUMER, 0x8B))

/* Consumer Page: Media Select Telephone [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_TELEPHONE (HID_USAGE(HID_USAGE_CONSUMER, 0x8C))

/* Consumer Page: Media Select Program Guide [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_PROGRAM_GUIDE (HID_USAGE(HID_USAGE_CONSUMER, 0x8D))

/* Consumer Page: Media Select Video Phone [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_VIDEO_PHONE (HID_USAGE(HID_USAGE_CONSUMER, 0x8E))

/* Consumer Page: Media Select Games [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_GAMES (HID_USAGE(HID_USAGE_CONSUMER, 0x8F))

/* Consumer Page: Media Select Messages [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_MESSAGES (HID_USAGE(HID_USAGE_CONSUMER, 0x90))

/* Consumer Page: Media Select CD [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_CD (HID_USAGE(HID_USAGE_CONSUMER, 0x91))

/* Consumer Page: Media Select VCR [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_VCR (HID_USAGE(HID_USAGE_CONSUMER, 0x92))

/* Consumer Page: Media Select Tuner [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_TUNER (HID_USAGE(HID_USAGE_CONSUMER, 0x93))

/* Consumer Page: Quit [OSC] */
#define HID_USAGE_CONSUMER_QUIT (HID_USAGE(HID_USAGE_CONSUMER, 0x94))

/* Consumer Page: Help [OOC] */
#define HID_USAGE_CONSUMER_HELP (HID_USAGE(HID_USAGE_CONSUMER, 0x95))

/* Consumer Page: Media Select Tape [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_TAPE (HID_USAGE(HID_USAGE_CONSUMER, 0x96))

/* Consumer Page: Media Select Cable [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_CABLE (HID_USAGE(HID_USAGE_CONSUMER, 0x97))

/* Consumer Page: Media Select Satellite [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_SATELLITE (HID_USAGE(HID_USAGE_CONSUMER, 0x98))

/* Consumer Page: Media Select Security [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_SECURITY (HID_USAGE(HID_USAGE_CONSUMER, 0x99))

/* Consumer Page: Media Select Home [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_HOME (HID_USAGE(HID_USAGE_CONSUMER, 0x9A))

/* Consumer Page: Media Select Call [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_CALL (HID_USAGE(HID_USAGE_CONSUMER, 0x9B))

/* Consumer Page: Channel Increment [OSC] */
#define HID_USAGE_CONSUMER_CHANNEL_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x9C))

/* Consumer Page: Channel Decrement [OSC] */
#define HID_USAGE_CONSUMER_CHANNEL_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x9D))

/* Consumer Page: Media Select SAP [Sel] */
#define HID_USAGE_CONSUMER_MEDIA_SELECT_SAP (HID_USAGE(HID_USAGE_CONSUMER, 0x9E))

/* Consumer Page: VCR Plus [OSC] */
#define HID_USAGE_CONSUMER_VCR_PLUS (HID_USAGE(HID_USAGE_CONSUMER, 0xA0))

/* Consumer Page: Once [OSC] */
#define HID_USAGE_CONSUMER_ONCE (HID_USAGE(HID_USAGE_CONSUMER, 0xA1))

/* Consumer Page: Daily [OSC] */
#define HID_USAGE_CONSUMER_DAILY (HID_USAGE(HID_USAGE_CONSUMER, 0xA2))

/* Consumer Page: Weekly [OSC] */
#define HID_USAGE_CONSUMER_WEEKLY (HID_USAGE(HID_USAGE_CONSUMER, 0xA3))

/* Consumer Page: Monthly [OSC] */
#define HID_USAGE_CONSUMER_MONTHLY (HID_USAGE(HID_USAGE_CONSUMER, 0xA4))

/* Consumer Page: Play [OOC] */
#define HID_USAGE_CONSUMER_PLAY (HID_USAGE(HID_USAGE_CONSUMER, 0xB0))

/* Consumer Page: Pause [OOC] */
#define HID_USAGE_CONSUMER_PAUSE (HID_USAGE(HID_USAGE_CONSUMER, 0xB1))

/* Consumer Page: Record [OOC] */
#define HID_USAGE_CONSUMER_RECORD (HID_USAGE(HID_USAGE_CONSUMER, 0xB2))

/* Consumer Page: Fast Forward [OOC] */
#define HID_USAGE_CONSUMER_FAST_FORWARD (HID_USAGE(HID_USAGE_CONSUMER, 0xB3))

/* Consumer Page: Rewind [OOC] */
#define HID_USAGE_CONSUMER_REWIND (HID_USAGE(HID_USAGE_CONSUMER, 0xB4))

/* Consumer Page: Scan Next Track [OSC] */
#define HID_USAGE_CONSUMER_SCAN_NEXT_TRACK (HID_USAGE(HID_USAGE_CONSUMER, 0xB5))

/* Consumer Page: Scan Previous Track [OSC] */
#define HID_USAGE_CONSUMER_SCAN_PREVIOUS_TRACK (HID_USAGE(HID_USAGE_CONSUMER, 0xB6))

/* Consumer Page: Stop [OSC] */
#define HID_USAGE_CONSUMER_STOP (HID_USAGE(HID_USAGE_CONSUMER, 0xB7))

/* Consumer Page: Eject [OSC] */
#define HID_USAGE_CONSUMER_EJECT (HID_USAGE(HID_USAGE_CONSUMER, 0xB8))

/* Consumer Page: Random Play [OOC] */
#define HID_USAGE_CONSUMER_RANDOM_PLAY (HID_USAGE(HID_USAGE_CONSUMER, 0xB9))

/* Consumer Page: Select Disc [NAry] */
#define HID_USAGE_CONSUMER_SELECT_DISC (HID_USAGE(HID_USAGE_CONSUMER, 0xBA))

/* Consumer Page: Enter Disc [MC] */
#define HID_USAGE_CONSUMER_ENTER_DISC (HID_USAGE(HID_USAGE_CONSUMER, 0xBB))

/* Consumer Page: Repeat [OSC] */
#define HID_USAGE_CONSUMER_REPEAT (HID_USAGE(HID_USAGE_CONSUMER, 0xBC))

/* Consumer Page: Tracking [LC] */
#define HID_USAGE_CONSUMER_TRACKING (HID_USAGE(HID_USAGE_CONSUMER, 0xBD))

/* Consumer Page: Track Normal [OSC] */
#define HID_USAGE_CONSUMER_TRACK_NORMAL (HID_USAGE(HID_USAGE_CONSUMER, 0xBE))

/* Consumer Page: Slow Tracking [LC] */
#define HID_USAGE_CONSUMER_SLOW_TRACKING (HID_USAGE(HID_USAGE_CONSUMER, 0xBF))

/* Consumer Page: Frame Forward [RTC] */
#define HID_USAGE_CONSUMER_FRAME_FORWARD (HID_USAGE(HID_USAGE_CONSUMER, 0xC0))

/* Consumer Page: Frame Back [RTC] */
#define HID_USAGE_CONSUMER_FRAME_BACK (HID_USAGE(HID_USAGE_CONSUMER, 0xC1))

/* Consumer Page: Mark [OSC] */
#define HID_USAGE_CONSUMER_MARK (HID_USAGE(HID_USAGE_CONSUMER, 0xC2))

/* Consumer Page: Clear Mark [OSC] */
#define HID_USAGE_CONSUMER_CLEAR_MARK (HID_USAGE(HID_USAGE_CONSUMER, 0xC3))

/* Consumer Page: Repeat From Mark [OOC] */
#define HID_USAGE_CONSUMER_REPEAT_FROM_MARK (HID_USAGE(HID_USAGE_CONSUMER, 0xC4))

/* Consumer Page: Return To Mark [OSC] */
#define HID_USAGE_CONSUMER_RETURN_TO_MARK (HID_USAGE(HID_USAGE_CONSUMER, 0xC5))

/* Consumer Page: Search Mark Forward [OSC] */
#define HID_USAGE_CONSUMER_SEARCH_MARK_FORWARD (HID_USAGE(HID_USAGE_CONSUMER, 0xC6))

/* Consumer Page: Search Mark Backwards [OSC] */
#define HID_USAGE_CONSUMER_SEARCH_MARK_BACKWARDS (HID_USAGE(HID_USAGE_CONSUMER, 0xC7))

/* Consumer Page: Counter Reset [OSC] */
#define HID_USAGE_CONSUMER_COUNTER_RESET (HID_USAGE(HID_USAGE_CONSUMER, 0xC8))

/* Consumer Page: Show Counter [OSC] */
#define HID_USAGE_CONSUMER_SHOW_COUNTER (HID_USAGE(HID_USAGE_CONSUMER, 0xC9))

/* Consumer Page: Tracking Increment [RTC] */
#define HID_USAGE_CONSUMER_TRACKING_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0xCA))

/* Consumer Page: Tracking Decrement [RTC] */
#define HID_USAGE_CONSUMER_TRACKING_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0xCB))

/* Consumer Page: Stop/Eject [OSC] */
#define HID_USAGE_CONSUMER_STOP_EJECT (HID_USAGE(HID_USAGE_CONSUMER, 0xCC))

/* Consumer Page: Play/Pause [OSC] */
#define HID_USAGE_CONSUMER_PLAY_PAUSE (HID_USAGE(HID_USAGE_CONSUMER, 0xCD))

/* Consumer Page: Play/Skip [OSC] */
#define HID_USAGE_CONSUMER_PLAY_SKIP (HID_USAGE(HID_USAGE_CONSUMER, 0xCE))

/* Consumer Page: Voice Command [OSC] */
#define HID_USAGE_CONSUMER_VOICE_COMMAND (HID_USAGE(HID_USAGE_CONSUMER, 0xCF))

/* Consumer Page: Invoke Capture Interface [Sel] */
#define HID_USAGE_CONSUMER_INVOKE_CAPTURE_INTERFACE (HID_USAGE(HID_USAGE_CONSUMER, 0xD0))

/* Consumer Page: Start or Stop Game Recording [Sel] */
#define HID_USAGE_CONSUMER_START_OR_STOP_GAME_RECORDING (HID_USAGE(HID_USAGE_CONSUMER, 0xD1))

/* Consumer Page: Historical Game Capture [Sel] */
#define HID_USAGE_CONSUMER_HISTORICAL_GAME_CAPTURE (HID_USAGE(HID_USAGE_CONSUMER, 0xD2))

/* Consumer Page: Capture Game Screenshot [Sel] */
#define HID_USAGE_CONSUMER_CAPTURE_GAME_SCREENSHOT (HID_USAGE(HID_USAGE_CONSUMER, 0xD3))

/* Consumer Page: Show or Hide Recording Indicator [Sel] */
#define HID_USAGE_CONSUMER_SHOW_OR_HIDE_RECORDING_INDICATOR (HID_USAGE(HID_USAGE_CONSUMER, 0xD4))

/* Consumer Page: Start or Stop Microphone Capture [Sel] */
#define HID_USAGE_CONSUMER_START_OR_STOP_MICROPHONE_CAPTURE (HID_USAGE(HID_USAGE_CONSUMER, 0xD5))

/* Consumer Page: Start or Stop Camera Capture [Sel] */
#define HID_USAGE_CONSUMER_START_OR_STOP_CAMERA_CAPTURE (HID_USAGE(HID_USAGE_CONSUMER, 0xD6))

/* Consumer Page: Start or Stop Game Broadcast [Sel] */
#define HID_USAGE_CONSUMER_START_OR_STOP_GAME_BROADCAST (HID_USAGE(HID_USAGE_CONSUMER, 0xD7))

/* Consumer Page: Volume [LC] */
#define HID_USAGE_CONSUMER_VOLUME (HID_USAGE(HID_USAGE_CONSUMER, 0xE0))

/* Consumer Page: Balance [LC] */
#define HID_USAGE_CONSUMER_BALANCE (HID_USAGE(HID_USAGE_CONSUMER, 0xE1))

/* Consumer Page: Mute [OOC] */
#define HID_USAGE_CONSUMER_MUTE (HID_USAGE(HID_USAGE_CONSUMER, 0xE2))

/* Consumer Page: Bass [LC] */
#define HID_USAGE_CONSUMER_BASS (HID_USAGE(HID_USAGE_CONSUMER, 0xE3))

/* Consumer Page: Treble [LC] */
#define HID_USAGE_CONSUMER_TREBLE (HID_USAGE(HID_USAGE_CONSUMER, 0xE4))

/* Consumer Page: Bass Boost [OOC] */
#define HID_USAGE_CONSUMER_BASS_BOOST (HID_USAGE(HID_USAGE_CONSUMER, 0xE5))

/* Consumer Page: Surround Mode [OSC] */
#define HID_USAGE_CONSUMER_SURROUND_MODE (HID_USAGE(HID_USAGE_CONSUMER, 0xE6))

/* Consumer Page: Loudness [OOC] */
#define HID_USAGE_CONSUMER_LOUDNESS (HID_USAGE(HID_USAGE_CONSUMER, 0xE7))

/* Consumer Page: MPX [OOC] */
#define HID_USAGE_CONSUMER_MPX (HID_USAGE(HID_USAGE_CONSUMER, 0xE8))

/* Consumer Page: Volume Increment [RTC] */
#define HID_USAGE_CONSUMER_VOLUME_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0xE9))

/* Consumer Page: Volume Decrement [RTC] */
#define HID_USAGE_CONSUMER_VOLUME_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0xEA))

/* Consumer Page: Speed Select [OSC] */
#define HID_USAGE_CONSUMER_SPEED_SELECT (HID_USAGE(HID_USAGE_CONSUMER, 0xF0))

/* Consumer Page: Playback Speed [NAry] */
#define HID_USAGE_CONSUMER_PLAYBACK_SPEED (HID_USAGE(HID_USAGE_CONSUMER, 0xF1))

/* Consumer Page: Standard Play [Sel] */
#define HID_USAGE_CONSUMER_STANDARD_PLAY (HID_USAGE(HID_USAGE_CONSUMER, 0xF2))

/* Consumer Page: Long Play [Sel] */
#define HID_USAGE_CONSUMER_LONG_PLAY (HID_USAGE(HID_USAGE_CONSUMER, 0xF3))

/* Consumer Page: Extended Play [Sel] */
#define HID_USAGE_CONSUMER_EXTENDED_PLAY (HID_USAGE(HID_USAGE_CONSUMER, 0xF4))

/* Consumer Page: Slow [OSC] */
#define HID_USAGE_CONSUMER_SLOW (HID_USAGE(HID_USAGE_CONSUMER, 0xF5))

/* Consumer Page: Fan Enable [OOC] */
#define HID_USAGE_CONSUMER_FAN_ENABLE (HID_USAGE(HID_USAGE_CONSUMER, 0x100))

/* Consumer Page: Fan Speed [LC] */
#define HID_USAGE_CONSUMER_FAN_SPEED (HID_USAGE(HID_USAGE_CONSUMER, 0x101))

/* Consumer Page: Light Enable [OOC] */
#define HID_USAGE_CONSUMER_LIGHT_ENABLE (HID_USAGE(HID_USAGE_CONSUMER, 0x102))

/* Consumer Page: Light Illumination Level [LC] */
#define HID_USAGE_CONSUMER_LIGHT_ILLUMINATION_LEVEL (HID_USAGE(HID_USAGE_CONSUMER, 0x103))

/* Consumer Page: Climate Control Enable [OOC] */
#define HID_USAGE_CONSUMER_CLIMATE_CONTROL_ENABLE (HID_USAGE(HID_USAGE_CONSUMER, 0x104))

/* Consumer Page: Room Temperature [LC] */
#define HID_USAGE_CONSUMER_ROOM_TEMPERATURE (HID_USAGE(HID_USAGE_CONSUMER, 0x105))

/* Consumer Page: Security Enable [OOC] */
#define HID_USAGE_CONSUMER_SECURITY_ENABLE (HID_USAGE(HID_USAGE_CONSUMER, 0x106))

/* Consumer Page: Fire Alarm [OSC] */
#define HID_USAGE_CONSUMER_FIRE_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x107))

/* Consumer Page: Police Alarm [OSC] */
#define HID_USAGE_CONSUMER_POLICE_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x108))

/* Consumer Page: Proximity [LC] */
#define HID_USAGE_CONSUMER_PROXIMITY (HID_USAGE(HID_USAGE_CONSUMER, 0x109))

/* Consumer Page: Motion [OSC] */
#define HID_USAGE_CONSUMER_MOTION (HID_USAGE(HID_USAGE_CONSUMER, 0x10A))

/* Consumer Page: Duress Alarm [OSC] */
#define HID_USAGE_CONSUMER_DURESS_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x10B))

/* Consumer Page: Holdup Alarm [OSC] */
#define HID_USAGE_CONSUMER_HOLDUP_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x10C))

/* Consumer Page: Medical Alarm [OSC] */
#define HID_USAGE_CONSUMER_MEDICAL_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x10D))

/* Consumer Page: Balance Right [RTC] */
#define HID_USAGE_CONSUMER_BALANCE_RIGHT (HID_USAGE(HID_USAGE_CONSUMER, 0x150))

/* Consumer Page: Balance Left [RTC] */
#define HID_USAGE_CONSUMER_BALANCE_LEFT (HID_USAGE(HID_USAGE_CONSUMER, 0x151))

/* Consumer Page: Bass Increment [RTC] */
#define HID_USAGE_CONSUMER_BASS_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x152))

/* Consumer Page: Bass Decrement [RTC] */
#define HID_USAGE_CONSUMER_BASS_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x153))

/* Consumer Page: Treble Increment [RTC] */
#define HID_USAGE_CONSUMER_TREBLE_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x154))

/* Consumer Page: Treble Decrement [RTC] */
#define HID_USAGE_CONSUMER_TREBLE_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x155))

/* Consumer Page: Speaker System [CL] */
#define HID_USAGE_CONSUMER_SPEAKER_SYSTEM (HID_USAGE(HID_USAGE_CONSUMER, 0x160))

/* Consumer Page: Channel Left [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_LEFT (HID_USAGE(HID_USAGE_CONSUMER, 0x161))

/* Consumer Page: Channel Right [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_RIGHT (HID_USAGE(HID_USAGE_CONSUMER, 0x162))

/* Consumer Page: Channel Center [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_CENTER (HID_USAGE(HID_USAGE_CONSUMER, 0x163))

/* Consumer Page: Channel Front [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_FRONT (HID_USAGE(HID_USAGE_CONSUMER, 0x164))

/* Consumer Page: Channel Center Front [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_CENTER_FRONT (HID_USAGE(HID_USAGE_CONSUMER, 0x165))

/* Consumer Page: Channel Side [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_SIDE (HID_USAGE(HID_USAGE_CONSUMER, 0x166))

/* Consumer Page: Channel Surround [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_SURROUND (HID_USAGE(HID_USAGE_CONSUMER, 0x167))

/* Consumer Page: Channel Low Frequency Enhancement [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_LOW_FREQUENCY_ENHANCEMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x168))

/* Consumer Page: Channel Top [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_TOP (HID_USAGE(HID_USAGE_CONSUMER, 0x169))

/* Consumer Page: Channel Unknown [CL] */
#define HID_USAGE_CONSUMER_CHANNEL_UNKNOWN (HID_USAGE(HID_USAGE_CONSUMER, 0x16A))

/* Consumer Page: Sub-channel [LC] */
#define HID_USAGE_CONSUMER_SUB_CHANNEL (HID_USAGE(HID_USAGE_CONSUMER, 0x170))

/* Consumer Page: Sub-channel Increment [OSC] */
#define HID_USAGE_CONSUMER_SUB_CHANNEL_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x171))

/* Consumer Page: Sub-channel Decrement [OSC] */
#define HID_USAGE_CONSUMER_SUB_CHANNEL_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x172))

/* Consumer Page: Alternate Audio Increment [OSC] */
#define HID_USAGE_CONSUMER_ALTERNATE_AUDIO_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x173))

/* Consumer Page: Alternate Audio Decrement [OSC] */
#define HID_USAGE_CONSUMER_ALTERNATE_AUDIO_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x174))

/* Consumer Page: Application Launch Buttons [NAry] */
#define HID_USAGE_CONSUMER_APPLICATION_LAUNCH_BUTTONS (HID_USAGE(HID_USAGE_CONSUMER, 0x180))

/* Consumer Page: AL Launch Button Configuration Tool [Sel] */
#define HID_USAGE_CONSUMER_AL_LAUNCH_BUTTON_CONFIGURATION_TOOL                                     \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x181))

/* Consumer Page: AL Programmable Button Configuration [Sel] */
#define HID_USAGE_CONSUMER_AL_PROGRAMMABLE_BUTTON_CONFIGURATION                                    \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x182))

/* Consumer Page: AL Consumer Control Configuration [Sel] */
#define HID_USAGE_CONSUMER_AL_CONSUMER_CONTROL_CONFIGURATION (HID_USAGE(HID_USAGE_CONSUMER, 0x183))

/* Consumer Page: AL Word Processor [Sel] */
#define HID_USAGE_CONSUMER_AL_WORD_PROCESSOR (HID_USAGE(HID_USAGE_CONSUMER, 0x184))

/* Consumer Page: AL Text Editor [Sel] */
#define HID_USAGE_CONSUMER_AL_TEXT_EDITOR (HID_USAGE(HID_USAGE_CONSUMER, 0x185))

/* Consumer Page: AL Spreadsheet [Sel] */
#define HID_USAGE_CONSUMER_AL_SPREADSHEET (HID_USAGE(HID_USAGE_CONSUMER, 0x186))

/* Consumer Page: AL Graphics Editor [Sel] */
#define HID_USAGE_CONSUMER_AL_GRAPHICS_EDITOR (HID_USAGE(HID_USAGE_CONSUMER, 0x187))

/* Consumer Page: AL Presentation App [Sel] */
#define HID_USAGE_CONSUMER_AL_PRESENTATION_APP (HID_USAGE(HID_USAGE_CONSUMER, 0x188))

/* Consumer Page: AL Database App [Sel] */
#define HID_USAGE_CONSUMER_AL_DATABASE_APP (HID_USAGE(HID_USAGE_CONSUMER, 0x189))

/* Consumer Page: AL Email Reader [Sel] */
#define HID_USAGE_CONSUMER_AL_EMAIL_READER (HID_USAGE(HID_USAGE_CONSUMER, 0x18A))

/* Consumer Page: AL Newsreader [Sel] */
#define HID_USAGE_CONSUMER_AL_NEWSREADER (HID_USAGE(HID_USAGE_CONSUMER, 0x18B))

/* Consumer Page: AL Voicemail [Sel] */
#define HID_USAGE_CONSUMER_AL_VOICEMAIL (HID_USAGE(HID_USAGE_CONSUMER, 0x18C))

/* Consumer Page: AL Contacts/Address Book [Sel] */
#define HID_USAGE_CONSUMER_AL_CONTACTS_ADDRESS_BOOK (HID_USAGE(HID_USAGE_CONSUMER, 0x18D))

/* Consumer Page: AL Calendar/Schedule [Sel] */
#define HID_USAGE_CONSUMER_AL_CALENDAR_SCHEDULE (HID_USAGE(HID_USAGE_CONSUMER, 0x18E))

/* Consumer Page: AL Task/Project Manager [Sel] */
#define HID_USAGE_CONSUMER_AL_TASK_PROJECT_MANAGER (HID_USAGE(HID_USAGE_CONSUMER, 0x18F))

/* Consumer Page: AL Log/Journal/Timecard [Sel] */
#define HID_USAGE_CONSUMER_AL_LOG_JOURNAL_TIMECARD (HID_USAGE(HID_USAGE_CONSUMER, 0x190))

/* Consumer Page: AL Checkbook/Finance [Sel] */
#define HID_USAGE_CONSUMER_AL_CHECKBOOK_FINANCE (HID_USAGE(HID_USAGE_CONSUMER, 0x191))

/* Consumer Page: AL Calculator [Sel] */
#define HID_USAGE_CONSUMER_AL_CALCULATOR (HID_USAGE(HID_USAGE_CONSUMER, 0x192))

/* Consumer Page: AL A/V Capture/Playback [Sel] */
#define HID_USAGE_CONSUMER_AL_A_V_CAPTURE_PLAYBACK (HID_USAGE(HID_USAGE_CONSUMER, 0x193))

/* Consumer Page: AL Local Machine Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_LOCAL_MACHINE_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x194))

/* Consumer Page: AL LAN/WAN Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_LAN_WAN_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x195))

/* Consumer Page: AL Internet Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_INTERNET_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x196))

/* Consumer Page: AL Remote Networking/ISP Connect [Sel] */
#define HID_USAGE_CONSUMER_AL_REMOTE_NETWORKING_ISP_CONNECT (HID_USAGE(HID_USAGE_CONSUMER, 0x197))

/* Consumer Page: AL Network Conference [Sel] */
#define HID_USAGE_CONSUMER_AL_NETWORK_CONFERENCE (HID_USAGE(HID_USAGE_CONSUMER, 0x198))

/* Consumer Page: AL Network Chat [Sel] */
#define HID_USAGE_CONSUMER_AL_NETWORK_CHAT (HID_USAGE(HID_USAGE_CONSUMER, 0x199))

/* Consumer Page: AL Telephony/Dialer [Sel] */
#define HID_USAGE_CONSUMER_AL_TELEPHONY_DIALER (HID_USAGE(HID_USAGE_CONSUMER, 0x19A))

/* Consumer Page: AL Logon [Sel] */
#define HID_USAGE_CONSUMER_AL_LOGON (HID_USAGE(HID_USAGE_CONSUMER, 0x19B))

/* Consumer Page: AL Logoff [Sel] */
#define HID_USAGE_CONSUMER_AL_LOGOFF (HID_USAGE(HID_USAGE_CONSUMER, 0x19C))

/* Consumer Page: AL Logon/Logoff [Sel] */
#define HID_USAGE_CONSUMER_AL_LOGON_LOGOFF (HID_USAGE(HID_USAGE_CONSUMER, 0x19D))

/* Consumer Page: AL Terminal Lock/Screensaver [Sel] */
#define HID_USAGE_CONSUMER_AL_TERMINAL_LOCK_SCREENSAVER (HID_USAGE(HID_USAGE_CONSUMER, 0x19E))

/* Consumer Page: AL Control Panel [Sel] */
#define HID_USAGE_CONSUMER_AL_CONTROL_PANEL (HID_USAGE(HID_USAGE_CONSUMER, 0x19F))

/* Consumer Page: AL Command Line Processor/Run [Sel] */
#define HID_USAGE_CONSUMER_AL_COMMAND_LINE_PROCESSOR_RUN (HID_USAGE(HID_USAGE_CONSUMER, 0x1A0))

/* Consumer Page: AL Process/Task Manager [Sel] */
#define HID_USAGE_CONSUMER_AL_PROCESS_TASK_MANAGER (HID_USAGE(HID_USAGE_CONSUMER, 0x1A1))

/* Consumer Page: AL Select Task/Application [Sel] */
#define HID_USAGE_CONSUMER_AL_SELECT_TASK_APPLICATION (HID_USAGE(HID_USAGE_CONSUMER, 0x1A2))

/* Consumer Page: AL Next Task/Application [Sel] */
#define HID_USAGE_CONSUMER_AL_NEXT_TASK_APPLICATION (HID_USAGE(HID_USAGE_CONSUMER, 0x1A3))

/* Consumer Page: AL Previous Task/Application [Sel] */
#define HID_USAGE_CONSUMER_AL_PREVIOUS_TASK_APPLICATION (HID_USAGE(HID_USAGE_CONSUMER, 0x1A4))

/* Consumer Page: AL Preemptive Halt Task/Application [Sel] */
#define HID_USAGE_CONSUMER_AL_PREEMPTIVE_HALT_TASK_APPLICATION                                     \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x1A5))

/* Consumer Page: AL Integrated Help Center [Sel] */
#define HID_USAGE_CONSUMER_AL_INTEGRATED_HELP_CENTER (HID_USAGE(HID_USAGE_CONSUMER, 0x1A6))

/* Consumer Page: AL Documents [Sel] */
#define HID_USAGE_CONSUMER_AL_DOCUMENTS (HID_USAGE(HID_USAGE_CONSUMER, 0x1A7))

/* Consumer Page: AL Thesaurus [Sel] */
#define HID_USAGE_CONSUMER_AL_THESAURUS (HID_USAGE(HID_USAGE_CONSUMER, 0x1A8))

/* Consumer Page: AL Dictionary [Sel] */
#define HID_USAGE_CONSUMER_AL_DICTIONARY (HID_USAGE(HID_USAGE_CONSUMER, 0x1A9))

/* Consumer Page: AL Desktop [Sel] */
#define HID_USAGE_CONSUMER_AL_DESKTOP (HID_USAGE(HID_USAGE_CONSUMER, 0x1AA))

/* Consumer Page: AL Spell Check [Sel] */
#define HID_USAGE_CONSUMER_AL_SPELL_CHECK (HID_USAGE(HID_USAGE_CONSUMER, 0x1AB))

/* Consumer Page: AL Grammar Check [Sel] */
#define HID_USAGE_CONSUMER_AL_GRAMMAR_CHECK (HID_USAGE(HID_USAGE_CONSUMER, 0x1AC))

/* Consumer Page: AL Wireless Status [Sel] */
#define HID_USAGE_CONSUMER_AL_WIRELESS_STATUS (HID_USAGE(HID_USAGE_CONSUMER, 0x1AD))

/* Consumer Page: AL Keyboard Layout [Sel] */
#define HID_USAGE_CONSUMER_AL_KEYBOARD_LAYOUT (HID_USAGE(HID_USAGE_CONSUMER, 0x1AE))

/* Consumer Page: AL Virus Protection [Sel] */
#define HID_USAGE_CONSUMER_AL_VIRUS_PROTECTION (HID_USAGE(HID_USAGE_CONSUMER, 0x1AF))

/* Consumer Page: AL Encryption [Sel] */
#define HID_USAGE_CONSUMER_AL_ENCRYPTION (HID_USAGE(HID_USAGE_CONSUMER, 0x1B0))

/* Consumer Page: AL Screen Saver [Sel] */
#define HID_USAGE_CONSUMER_AL_SCREEN_SAVER (HID_USAGE(HID_USAGE_CONSUMER, 0x1B1))

/* Consumer Page: AL Alarms [Sel] */
#define HID_USAGE_CONSUMER_AL_ALARMS (HID_USAGE(HID_USAGE_CONSUMER, 0x1B2))

/* Consumer Page: AL Clock [Sel] */
#define HID_USAGE_CONSUMER_AL_CLOCK (HID_USAGE(HID_USAGE_CONSUMER, 0x1B3))

/* Consumer Page: AL File Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_FILE_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1B4))

/* Consumer Page: AL Power Status [Sel] */
#define HID_USAGE_CONSUMER_AL_POWER_STATUS (HID_USAGE(HID_USAGE_CONSUMER, 0x1B5))

/* Consumer Page: AL Image Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_IMAGE_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1B6))

/* Consumer Page: AL Audio Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_AUDIO_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1B7))

/* Consumer Page: AL Movie Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_MOVIE_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1B8))

/* Consumer Page: AL Digital Rights Manager [Sel] */
#define HID_USAGE_CONSUMER_AL_DIGITAL_RIGHTS_MANAGER (HID_USAGE(HID_USAGE_CONSUMER, 0x1B9))

/* Consumer Page: AL Digital Wallet [Sel] */
#define HID_USAGE_CONSUMER_AL_DIGITAL_WALLET (HID_USAGE(HID_USAGE_CONSUMER, 0x1BA))

/* Consumer Page: AL Instant Messaging [Sel] */
#define HID_USAGE_CONSUMER_AL_INSTANT_MESSAGING (HID_USAGE(HID_USAGE_CONSUMER, 0x1BC))

/* Consumer Page: AL OEM Features/Tips/Tutorial Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_OEM_FEATURES_TIPS_TUTORIAL_BROWSER                                   \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x1BD))

/* Consumer Page: AL OEM Help [Sel] */
#define HID_USAGE_CONSUMER_AL_OEM_HELP (HID_USAGE(HID_USAGE_CONSUMER, 0x1BE))

/* Consumer Page: AL Online Community [Sel] */
#define HID_USAGE_CONSUMER_AL_ONLINE_COMMUNITY (HID_USAGE(HID_USAGE_CONSUMER, 0x1BF))

/* Consumer Page: AL Entertainment Content Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_ENTERTAINMENT_CONTENT_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1C0))

/* Consumer Page: AL Online Shopping Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_ONLINE_SHOPPING_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1C1))

/* Consumer Page: AL SmartCard Information/Help [Sel] */
#define HID_USAGE_CONSUMER_AL_SMARTCARD_INFORMATION_HELP (HID_USAGE(HID_USAGE_CONSUMER, 0x1C2))

/* Consumer Page: AL Market Monitor/Finance Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_MARKET_MONITOR_FINANCE_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1C3))

/* Consumer Page: AL Customized Corporate News Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_CUSTOMIZED_CORPORATE_NEWS_BROWSER                                    \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x1C4))

/* Consumer Page: AL Online Activity Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_ONLINE_ACTIVITY_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1C5))

/* Consumer Page: AL Research/Search Browser [Sel] */
#define HID_USAGE_CONSUMER_AL_RESEARCH_SEARCH_BROWSER (HID_USAGE(HID_USAGE_CONSUMER, 0x1C6))

/* Consumer Page: AL Audio Player [Sel] */
#define HID_USAGE_CONSUMER_AL_AUDIO_PLAYER (HID_USAGE(HID_USAGE_CONSUMER, 0x1C7))

/* Consumer Page: AL Message Status [Sel] */
#define HID_USAGE_CONSUMER_AL_MESSAGE_STATUS (HID_USAGE(HID_USAGE_CONSUMER, 0x1C8))

/* Consumer Page: AL Contact Sync [Sel] */
#define HID_USAGE_CONSUMER_AL_CONTACT_SYNC (HID_USAGE(HID_USAGE_CONSUMER, 0x1C9))

/* Consumer Page: AL Navigation [Sel] */
#define HID_USAGE_CONSUMER_AL_NAVIGATION (HID_USAGE(HID_USAGE_CONSUMER, 0x1CA))

/* Consumer Page: AL Context-aware Desktop Assistant [Sel] */
#define HID_USAGE_CONSUMER_AL_CONTEXT_AWARE_DESKTOP_ASSISTANT (HID_USAGE(HID_USAGE_CONSUMER, 0x1CB))

/* Consumer Page: Generic GUI Application Controls [NAry] */
#define HID_USAGE_CONSUMER_GENERIC_GUI_APPLICATION_CONTROLS (HID_USAGE(HID_USAGE_CONSUMER, 0x200))

/* Consumer Page: AC New [Sel] */
#define HID_USAGE_CONSUMER_AC_NEW (HID_USAGE(HID_USAGE_CONSUMER, 0x201))

/* Consumer Page: AC Open [Sel] */
#define HID_USAGE_CONSUMER_AC_OPEN (HID_USAGE(HID_USAGE_CONSUMER, 0x202))

/* Consumer Page: AC Close [Sel] */
#define HID_USAGE_CONSUMER_AC_CLOSE (HID_USAGE(HID_USAGE_CONSUMER, 0x203))

/* Consumer Page: AC Exit [Sel] */
#define HID_USAGE_CONSUMER_AC_EXIT (HID_USAGE(HID_USAGE_CONSUMER, 0x204))

/* Consumer Page: AC Maximize [Sel] */
#define HID_USAGE_CONSUMER_AC_MAXIMIZE (HID_USAGE(HID_USAGE_CONSUMER, 0x205))

/* Consumer Page: AC Minimize [Sel] */
#define HID_USAGE_CONSUMER_AC_MINIMIZE (HID_USAGE(HID_USAGE_CONSUMER, 0x206))

/* Consumer Page: AC Save [Sel] */
#define HID_USAGE_CONSUMER_AC_SAVE (HID_USAGE(HID_USAGE_CONSUMER, 0x207))

/* Consumer Page: AC Print [Sel] */
#define HID_USAGE_CONSUMER_AC_PRINT (HID_USAGE(HID_USAGE_CONSUMER, 0x208))

/* Consumer Page: AC Properties [Sel] */
#define HID_USAGE_CONSUMER_AC_PROPERTIES (HID_USAGE(HID_USAGE_CONSUMER, 0x209))

/* Consumer Page: AC Undo [Sel] */
#define HID_USAGE_CONSUMER_AC_UNDO (HID_USAGE(HID_USAGE_CONSUMER, 0x21A))

/* Consumer Page: AC Copy [Sel] */
#define HID_USAGE_CONSUMER_AC_COPY (HID_USAGE(HID_USAGE_CONSUMER, 0x21B))

/* Consumer Page: AC Cut [Sel] */
#define HID_USAGE_CONSUMER_AC_CUT (HID_USAGE(HID_USAGE_CONSUMER, 0x21C))

/* Consumer Page: AC Paste [Sel] */
#define HID_USAGE_CONSUMER_AC_PASTE (HID_USAGE(HID_USAGE_CONSUMER, 0x21D))

/* Consumer Page: AC Select All [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_ALL (HID_USAGE(HID_USAGE_CONSUMER, 0x21E))

/* Consumer Page: AC Find [Sel] */
#define HID_USAGE_CONSUMER_AC_FIND (HID_USAGE(HID_USAGE_CONSUMER, 0x21F))

/* Consumer Page: AC Find and Replace [Sel] */
#define HID_USAGE_CONSUMER_AC_FIND_AND_REPLACE (HID_USAGE(HID_USAGE_CONSUMER, 0x220))

/* Consumer Page: AC Search [Sel] */
#define HID_USAGE_CONSUMER_AC_SEARCH (HID_USAGE(HID_USAGE_CONSUMER, 0x221))

/* Consumer Page: AC Go To [Sel] */
#define HID_USAGE_CONSUMER_AC_GO_TO (HID_USAGE(HID_USAGE_CONSUMER, 0x222))

/* Consumer Page: AC Home [Sel] */
#define HID_USAGE_CONSUMER_AC_HOME (HID_USAGE(HID_USAGE_CONSUMER, 0x223))

/* Consumer Page: AC Back [Sel] */
#define HID_USAGE_CONSUMER_AC_BACK (HID_USAGE(HID_USAGE_CONSUMER, 0x224))

/* Consumer Page: AC Forward [Sel] */
#define HID_USAGE_CONSUMER_AC_FORWARD (HID_USAGE(HID_USAGE_CONSUMER, 0x225))

/* Consumer Page: AC Stop [Sel] */
#define HID_USAGE_CONSUMER_AC_STOP (HID_USAGE(HID_USAGE_CONSUMER, 0x226))

/* Consumer Page: AC Refresh [Sel] */
#define HID_USAGE_CONSUMER_AC_REFRESH (HID_USAGE(HID_USAGE_CONSUMER, 0x227))

/* Consumer Page: AC Previous Link [Sel] */
#define HID_USAGE_CONSUMER_AC_PREVIOUS_LINK (HID_USAGE(HID_USAGE_CONSUMER, 0x228))

/* Consumer Page: AC Next Link [Sel] */
#define HID_USAGE_CONSUMER_AC_NEXT_LINK (HID_USAGE(HID_USAGE_CONSUMER, 0x229))

/* Consumer Page: AC Bookmarks [Sel] */
#define HID_USAGE_CONSUMER_AC_BOOKMARKS (HID_USAGE(HID_USAGE_CONSUMER, 0x22A))

/* Consumer Page: AC History [Sel] */
#define HID_USAGE_CONSUMER_AC_HISTORY (HID_USAGE(HID_USAGE_CONSUMER, 0x22B))

/* Consumer Page: AC Subscriptions [Sel] */
#define HID_USAGE_CONSUMER_AC_SUBSCRIPTIONS (HID_USAGE(HID_USAGE_CONSUMER, 0x22C))

/* Consumer Page: AC Zoom In [Sel] */
#define HID_USAGE_CONSUMER_AC_ZOOM_IN (HID_USAGE(HID_USAGE_CONSUMER, 0x22D))

/* Consumer Page: AC Zoom Out [Sel] */
#define HID_USAGE_CONSUMER_AC_ZOOM_OUT (HID_USAGE(HID_USAGE_CONSUMER, 0x22E))

/* Consumer Page: AC Zoom [LC] */
#define HID_USAGE_CONSUMER_AC_ZOOM (HID_USAGE(HID_USAGE_CONSUMER, 0x22F))

/* Consumer Page: AC Full Screen View [Sel] */
#define HID_USAGE_CONSUMER_AC_FULL_SCREEN_VIEW (HID_USAGE(HID_USAGE_CONSUMER, 0x230))

/* Consumer Page: AC Normal View [Sel] */
#define HID_USAGE_CONSUMER_AC_NORMAL_VIEW (HID_USAGE(HID_USAGE_CONSUMER, 0x231))

/* Consumer Page: AC View Toggle [Sel] */
#define HID_USAGE_CONSUMER_AC_VIEW_TOGGLE (HID_USAGE(HID_USAGE_CONSUMER, 0x232))

/* Consumer Page: AC Scroll Up [Sel] */
#define HID_USAGE_CONSUMER_AC_SCROLL_UP (HID_USAGE(HID_USAGE_CONSUMER, 0x233))

/* Consumer Page: AC Scroll Down [Sel] */
#define HID_USAGE_CONSUMER_AC_SCROLL_DOWN (HID_USAGE(HID_USAGE_CONSUMER, 0x234))

/* Consumer Page: AC Scroll [LC] */
#define HID_USAGE_CONSUMER_AC_SCROLL (HID_USAGE(HID_USAGE_CONSUMER, 0x235))

/* Consumer Page: AC Pan Left [Sel] */
#define HID_USAGE_CONSUMER_AC_PAN_LEFT (HID_USAGE(HID_USAGE_CONSUMER, 0x236))

/* Consumer Page: AC Pan Right [Sel] */
#define HID_USAGE_CONSUMER_AC_PAN_RIGHT (HID_USAGE(HID_USAGE_CONSUMER, 0x237))

/* Consumer Page: AC Pan [LC] */
#define HID_USAGE_CONSUMER_AC_PAN (HID_USAGE(HID_USAGE_CONSUMER, 0x238))

/* Consumer Page: AC New Window [Sel] */
#define HID_USAGE_CONSUMER_AC_NEW_WINDOW (HID_USAGE(HID_USAGE_CONSUMER, 0x239))

/* Consumer Page: AC Tile Horizontally [Sel] */
#define HID_USAGE_CONSUMER_AC_TILE_HORIZONTALLY (HID_USAGE(HID_USAGE_CONSUMER, 0x23A))

/* Consumer Page: AC Tile Vertically [Sel] */
#define HID_USAGE_CONSUMER_AC_TILE_VERTICALLY (HID_USAGE(HID_USAGE_CONSUMER, 0x23B))

/* Consumer Page: AC Format [Sel] */
#define HID_USAGE_CONSUMER_AC_FORMAT (HID_USAGE(HID_USAGE_CONSUMER, 0x23C))

/* Consumer Page: AC Edit [Sel] */
#define HID_USAGE_CONSUMER_AC_EDIT (HID_USAGE(HID_USAGE_CONSUMER, 0x23D))

/* Consumer Page: AC Bold [Sel] */
#define HID_USAGE_CONSUMER_AC_BOLD (HID_USAGE(HID_USAGE_CONSUMER, 0x23E))

/* Consumer Page: AC Italics [Sel] */
#define HID_USAGE_CONSUMER_AC_ITALICS (HID_USAGE(HID_USAGE_CONSUMER, 0x23F))

/* Consumer Page: AC Underline [Sel] */
#define HID_USAGE_CONSUMER_AC_UNDERLINE (HID_USAGE(HID_USAGE_CONSUMER, 0x240))

/* Consumer Page: AC Strikethrough [Sel] */
#define HID_USAGE_CONSUMER_AC_STRIKETHROUGH (HID_USAGE(HID_USAGE_CONSUMER, 0x241))

/* Consumer Page: AC Subscript [Sel] */
#define HID_USAGE_CONSUMER_AC_SUBSCRIPT (HID_USAGE(HID_USAGE_CONSUMER, 0x242))

/* Consumer Page: AC Superscript [Sel] */
#define HID_USAGE_CONSUMER_AC_SUPERSCRIPT (HID_USAGE(HID_USAGE_CONSUMER, 0x243))

/* Consumer Page: AC All Caps [Sel] */
#define HID_USAGE_CONSUMER_AC_ALL_CAPS (HID_USAGE(HID_USAGE_CONSUMER, 0x244))

/* Consumer Page: AC Rotate [Sel] */
#define HID_USAGE_CONSUMER_AC_ROTATE (HID_USAGE(HID_USAGE_CONSUMER, 0x245))

/* Consumer Page: AC Resize [Sel] */
#define HID_USAGE_CONSUMER_AC_RESIZE (HID_USAGE(HID_USAGE_CONSUMER, 0x246))

/* Consumer Page: AC Flip Horizontal [Sel] */
#define HID_USAGE_CONSUMER_AC_FLIP_HORIZONTAL (HID_USAGE(HID_USAGE_CONSUMER, 0x247))

/* Consumer Page: AC Flip Vertical [Sel] */
#define HID_USAGE_CONSUMER_AC_FLIP_VERTICAL (HID_USAGE(HID_USAGE_CONSUMER, 0x248))

/* Consumer Page: AC Mirror Horizontal [Sel] */
#define HID_USAGE_CONSUMER_AC_MIRROR_HORIZONTAL (HID_USAGE(HID_USAGE_CONSUMER, 0x249))

/* Consumer Page: AC Mirror Vertical [Sel] */
#define HID_USAGE_CONSUMER_AC_MIRROR_VERTICAL (HID_USAGE(HID_USAGE_CONSUMER, 0x24A))

/* Consumer Page: AC Font Select [Sel] */
#define HID_USAGE_CONSUMER_AC_FONT_SELECT (HID_USAGE(HID_USAGE_CONSUMER, 0x24B))

/* Consumer Page: AC Font Color [Sel] */
#define HID_USAGE_CONSUMER_AC_FONT_COLOR (HID_USAGE(HID_USAGE_CONSUMER, 0x24C))

/* Consumer Page: AC Font Size [Sel] */
#define HID_USAGE_CONSUMER_AC_FONT_SIZE (HID_USAGE(HID_USAGE_CONSUMER, 0x24D))

/* Consumer Page: AC Justify Left [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_LEFT (HID_USAGE(HID_USAGE_CONSUMER, 0x24E))

/* Consumer Page: AC Justify Center H [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_CENTER_H (HID_USAGE(HID_USAGE_CONSUMER, 0x24F))

/* Consumer Page: AC Justify Right [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_RIGHT (HID_USAGE(HID_USAGE_CONSUMER, 0x250))

/* Consumer Page: AC Justify Block H [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_BLOCK_H (HID_USAGE(HID_USAGE_CONSUMER, 0x251))

/* Consumer Page: AC Justify Top [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_TOP (HID_USAGE(HID_USAGE_CONSUMER, 0x252))

/* Consumer Page: AC Justify Center V [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_CENTER_V (HID_USAGE(HID_USAGE_CONSUMER, 0x253))

/* Consumer Page: AC Justify Bottom [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_BOTTOM (HID_USAGE(HID_USAGE_CONSUMER, 0x254))

/* Consumer Page: AC Justify Block V [Sel] */
#define HID_USAGE_CONSUMER_AC_JUSTIFY_BLOCK_V (HID_USAGE(HID_USAGE_CONSUMER, 0x255))

/* Consumer Page: AC Indent Decrease [Sel] */
#define HID_USAGE_CONSUMER_AC_INDENT_DECREASE (HID_USAGE(HID_USAGE_CONSUMER, 0x256))

/* Consumer Page: AC Indent Increase [Sel] */
#define HID_USAGE_CONSUMER_AC_INDENT_INCREASE (HID_USAGE(HID_USAGE_CONSUMER, 0x257))

/* Consumer Page: AC Numbered List [Sel] */
#define HID_USAGE_CONSUMER_AC_NUMBERED_LIST (HID_USAGE(HID_USAGE_CONSUMER, 0x258))

/* Consumer Page: AC Restart Numbering [Sel] */
#define HID_USAGE_CONSUMER_AC_RESTART_NUMBERING (HID_USAGE(HID_USAGE_CONSUMER, 0x259))

/* Consumer Page: AC Bulleted List [Sel] */
#define HID_USAGE_CONSUMER_AC_BULLETED_LIST (HID_USAGE(HID_USAGE_CONSUMER, 0x25A))

/* Consumer Page: AC Promote [Sel] */
#define HID_USAGE_CONSUMER_AC_PROMOTE (HID_USAGE(HID_USAGE_CONSUMER, 0x25B))

/* Consumer Page: AC Demote [Sel] */
#define HID_USAGE_CONSUMER_AC_DEMOTE (HID_USAGE(HID_USAGE_CONSUMER, 0x25C))

/* Consumer Page: AC Yes [Sel] */
#define HID_USAGE_CONSUMER_AC_YES (HID_USAGE(HID_USAGE_CONSUMER, 0x25D))

/* Consumer Page: AC No [Sel] */
#define HID_USAGE_CONSUMER_AC_NO (HID_USAGE(HID_USAGE_CONSUMER, 0x25E))

/* Consumer Page: AC Cancel [Sel] */
#define HID_USAGE_CONSUMER_AC_CANCEL (HID_USAGE(HID_USAGE_CONSUMER, 0x25F))

/* Consumer Page: AC Catalog [Sel] */
#define HID_USAGE_CONSUMER_AC_CATALOG (HID_USAGE(HID_USAGE_CONSUMER, 0x260))

/* Consumer Page: AC Buy/Checkout [Sel] */
#define HID_USAGE_CONSUMER_AC_BUY_CHECKOUT (HID_USAGE(HID_USAGE_CONSUMER, 0x261))

/* Consumer Page: AC Add to Cart [Sel] */
#define HID_USAGE_CONSUMER_AC_ADD_TO_CART (HID_USAGE(HID_USAGE_CONSUMER, 0x262))

/* Consumer Page: AC Expand [Sel] */
#define HID_USAGE_CONSUMER_AC_EXPAND (HID_USAGE(HID_USAGE_CONSUMER, 0x263))

/* Consumer Page: AC Expand All [Sel] */
#define HID_USAGE_CONSUMER_AC_EXPAND_ALL (HID_USAGE(HID_USAGE_CONSUMER, 0x264))

/* Consumer Page: AC Collapse [Sel] */
#define HID_USAGE_CONSUMER_AC_COLLAPSE (HID_USAGE(HID_USAGE_CONSUMER, 0x265))

/* Consumer Page: AC Collapse All [Sel] */
#define HID_USAGE_CONSUMER_AC_COLLAPSE_ALL (HID_USAGE(HID_USAGE_CONSUMER, 0x266))

/* Consumer Page: AC Print Preview [Sel] */
#define HID_USAGE_CONSUMER_AC_PRINT_PREVIEW (HID_USAGE(HID_USAGE_CONSUMER, 0x267))

/* Consumer Page: AC Paste Special [Sel] */
#define HID_USAGE_CONSUMER_AC_PASTE_SPECIAL (HID_USAGE(HID_USAGE_CONSUMER, 0x268))

/* Consumer Page: AC Insert Mode [Sel] */
#define HID_USAGE_CONSUMER_AC_INSERT_MODE (HID_USAGE(HID_USAGE_CONSUMER, 0x269))

/* Consumer Page: AC Delete [Sel] */
#define HID_USAGE_CONSUMER_AC_DELETE (HID_USAGE(HID_USAGE_CONSUMER, 0x26A))

/* Consumer Page: AC Lock [Sel] */
#define HID_USAGE_CONSUMER_AC_LOCK (HID_USAGE(HID_USAGE_CONSUMER, 0x26B))

/* Consumer Page: AC Unlock [Sel] */
#define HID_USAGE_CONSUMER_AC_UNLOCK (HID_USAGE(HID_USAGE_CONSUMER, 0x26C))

/* Consumer Page: AC Protect [Sel] */
#define HID_USAGE_CONSUMER_AC_PROTECT (HID_USAGE(HID_USAGE_CONSUMER, 0x26D))

/* Consumer Page: AC Unprotect [Sel] */
#define HID_USAGE_CONSUMER_AC_UNPROTECT (HID_USAGE(HID_USAGE_CONSUMER, 0x26E))

/* Consumer Page: AC Attach Comment [Sel] */
#define HID_USAGE_CONSUMER_AC_ATTACH_COMMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x26F))

/* Consumer Page: AC Delete Comment [Sel] */
#define HID_USAGE_CONSUMER_AC_DELETE_COMMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x270))

/* Consumer Page: AC View Comment [Sel] */
#define HID_USAGE_CONSUMER_AC_VIEW_COMMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x271))

/* Consumer Page: AC Select Word [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_WORD (HID_USAGE(HID_USAGE_CONSUMER, 0x272))

/* Consumer Page: AC Select Sentence [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_SENTENCE (HID_USAGE(HID_USAGE_CONSUMER, 0x273))

/* Consumer Page: AC Select Paragraph [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_PARAGRAPH (HID_USAGE(HID_USAGE_CONSUMER, 0x274))

/* Consumer Page: AC Select Column [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_COLUMN (HID_USAGE(HID_USAGE_CONSUMER, 0x275))

/* Consumer Page: AC Select Row [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_ROW (HID_USAGE(HID_USAGE_CONSUMER, 0x276))

/* Consumer Page: AC Select Table [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_TABLE (HID_USAGE(HID_USAGE_CONSUMER, 0x277))

/* Consumer Page: AC Select Object [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_OBJECT (HID_USAGE(HID_USAGE_CONSUMER, 0x278))

/* Consumer Page: AC Redo/Repeat [Sel] */
#define HID_USAGE_CONSUMER_AC_REDO_REPEAT (HID_USAGE(HID_USAGE_CONSUMER, 0x279))

/* Consumer Page: AC Sort [Sel] */
#define HID_USAGE_CONSUMER_AC_SORT (HID_USAGE(HID_USAGE_CONSUMER, 0x27A))

/* Consumer Page: AC Sort Ascending [Sel] */
#define HID_USAGE_CONSUMER_AC_SORT_ASCENDING (HID_USAGE(HID_USAGE_CONSUMER, 0x27B))

/* Consumer Page: AC Sort Descending [Sel] */
#define HID_USAGE_CONSUMER_AC_SORT_DESCENDING (HID_USAGE(HID_USAGE_CONSUMER, 0x27C))

/* Consumer Page: AC Filter [Sel] */
#define HID_USAGE_CONSUMER_AC_FILTER (HID_USAGE(HID_USAGE_CONSUMER, 0x27D))

/* Consumer Page: AC Set Clock [Sel] */
#define HID_USAGE_CONSUMER_AC_SET_CLOCK (HID_USAGE(HID_USAGE_CONSUMER, 0x27E))

/* Consumer Page: AC View Clock [Sel] */
#define HID_USAGE_CONSUMER_AC_VIEW_CLOCK (HID_USAGE(HID_USAGE_CONSUMER, 0x27F))

/* Consumer Page: AC Select Time Zone [Sel] */
#define HID_USAGE_CONSUMER_AC_SELECT_TIME_ZONE (HID_USAGE(HID_USAGE_CONSUMER, 0x280))

/* Consumer Page: AC Edit Time Zones [Sel] */
#define HID_USAGE_CONSUMER_AC_EDIT_TIME_ZONES (HID_USAGE(HID_USAGE_CONSUMER, 0x281))

/* Consumer Page: AC Set Alarm [Sel] */
#define HID_USAGE_CONSUMER_AC_SET_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x282))

/* Consumer Page: AC Clear Alarm [Sel] */
#define HID_USAGE_CONSUMER_AC_CLEAR_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x283))

/* Consumer Page: AC Snooze Alarm [Sel] */
#define HID_USAGE_CONSUMER_AC_SNOOZE_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x284))

/* Consumer Page: AC Reset Alarm [Sel] */
#define HID_USAGE_CONSUMER_AC_RESET_ALARM (HID_USAGE(HID_USAGE_CONSUMER, 0x285))

/* Consumer Page: AC Synchronize [Sel] */
#define HID_USAGE_CONSUMER_AC_SYNCHRONIZE (HID_USAGE(HID_USAGE_CONSUMER, 0x286))

/* Consumer Page: AC Send/Receive [Sel] */
#define HID_USAGE_CONSUMER_AC_SEND_RECEIVE (HID_USAGE(HID_USAGE_CONSUMER, 0x287))

/* Consumer Page: AC Send To [Sel] */
#define HID_USAGE_CONSUMER_AC_SEND_TO (HID_USAGE(HID_USAGE_CONSUMER, 0x288))

/* Consumer Page: AC Reply [Sel] */
#define HID_USAGE_CONSUMER_AC_REPLY (HID_USAGE(HID_USAGE_CONSUMER, 0x289))

/* Consumer Page: AC Reply All [Sel] */
#define HID_USAGE_CONSUMER_AC_REPLY_ALL (HID_USAGE(HID_USAGE_CONSUMER, 0x28A))

/* Consumer Page: AC Forward Msg [Sel] */
#define HID_USAGE_CONSUMER_AC_FORWARD_MSG (HID_USAGE(HID_USAGE_CONSUMER, 0x28B))

/* Consumer Page: AC Send [Sel] */
#define HID_USAGE_CONSUMER_AC_SEND (HID_USAGE(HID_USAGE_CONSUMER, 0x28C))

/* Consumer Page: AC Attach File [Sel] */
#define HID_USAGE_CONSUMER_AC_ATTACH_FILE (HID_USAGE(HID_USAGE_CONSUMER, 0x28D))

/* Consumer Page: AC Upload [Sel] */
#define HID_USAGE_CONSUMER_AC_UPLOAD (HID_USAGE(HID_USAGE_CONSUMER, 0x28E))

/* Consumer Page: AC Download (Save Target As) [Sel] */
#define HID_USAGE_CONSUMER_AC_DOWNLOAD_SAVE_TARGET_AS (HID_USAGE(HID_USAGE_CONSUMER, 0x28F))

/* Consumer Page: AC Set Borders [Sel] */
#define HID_USAGE_CONSUMER_AC_SET_BORDERS (HID_USAGE(HID_USAGE_CONSUMER, 0x290))

/* Consumer Page: AC Insert Row [Sel] */
#define HID_USAGE_CONSUMER_AC_INSERT_ROW (HID_USAGE(HID_USAGE_CONSUMER, 0x291))

/* Consumer Page: AC Insert Column [Sel] */
#define HID_USAGE_CONSUMER_AC_INSERT_COLUMN (HID_USAGE(HID_USAGE_CONSUMER, 0x292))

/* Consumer Page: AC Insert File [Sel] */
#define HID_USAGE_CONSUMER_AC_INSERT_FILE (HID_USAGE(HID_USAGE_CONSUMER, 0x293))

/* Consumer Page: AC Insert Picture [Sel] */
#define HID_USAGE_CONSUMER_AC_INSERT_PICTURE (HID_USAGE(HID_USAGE_CONSUMER, 0x294))

/* Consumer Page: AC Insert Object [Sel] */
#define HID_USAGE_CONSUMER_AC_INSERT_OBJECT (HID_USAGE(HID_USAGE_CONSUMER, 0x295))

/* Consumer Page: AC Insert Symbol [Sel] */
#define HID_USAGE_CONSUMER_AC_INSERT_SYMBOL (HID_USAGE(HID_USAGE_CONSUMER, 0x296))

/* Consumer Page: AC Save and Close [Sel] */
#define HID_USAGE_CONSUMER_AC_SAVE_AND_CLOSE (HID_USAGE(HID_USAGE_CONSUMER, 0x297))

/* Consumer Page: AC Rename [Sel] */
#define HID_USAGE_CONSUMER_AC_RENAME (HID_USAGE(HID_USAGE_CONSUMER, 0x298))

/* Consumer Page: AC Merge [Sel] */
#define HID_USAGE_CONSUMER_AC_MERGE (HID_USAGE(HID_USAGE_CONSUMER, 0x299))

/* Consumer Page: AC Split [Sel] */
#define HID_USAGE_CONSUMER_AC_SPLIT (HID_USAGE(HID_USAGE_CONSUMER, 0x29A))

/* Consumer Page: AC Disribute Horizontally [Sel] */
#define HID_USAGE_CONSUMER_AC_DISRIBUTE_HORIZONTALLY (HID_USAGE(HID_USAGE_CONSUMER, 0x29B))

/* Consumer Page: AC Distribute Vertically [Sel] */
#define HID_USAGE_CONSUMER_AC_DISTRIBUTE_VERTICALLY (HID_USAGE(HID_USAGE_CONSUMER, 0x29C))

/* Consumer Page: AC Next Keyboard Layout Select [Sel] */
#define HID_USAGE_CONSUMER_AC_NEXT_KEYBOARD_LAYOUT_SELECT (HID_USAGE(HID_USAGE_CONSUMER, 0x29D))

/* Consumer Page: AC Navigation Guidance [Sel] */
#define HID_USAGE_CONSUMER_AC_NAVIGATION_GUIDANCE (HID_USAGE(HID_USAGE_CONSUMER, 0x29E))

/* Consumer Page: AC Desktop Show All Windows [Sel] */
#define HID_USAGE_CONSUMER_AC_DESKTOP_SHOW_ALL_WINDOWS (HID_USAGE(HID_USAGE_CONSUMER, 0x29F))

/* Consumer Page: AC Soft Key Left, [Sel] */
#define HID_USAGE_CONSUMER_AC_SOFT_KEY_LEFT (HID_USAGE(HID_USAGE_CONSUMER, 0x2A0))

/* Consumer Page: AC Soft Key Right [Sel] */
#define HID_USAGE_CONSUMER_AC_SOFT_KEY_RIGHT (HID_USAGE(HID_USAGE_CONSUMER, 0x2A1))

/* Consumer Page: AC Desktop Show All Applications [Sel] */
#define HID_USAGE_CONSUMER_AC_DESKTOP_SHOW_ALL_APPLICATIONS (HID_USAGE(HID_USAGE_CONSUMER, 0x2A2))

/* Consumer Page: AC Idle Keep Alive [Sel] */
#define HID_USAGE_CONSUMER_AC_IDLE_KEEP_ALIVE (HID_USAGE(HID_USAGE_CONSUMER, 0x2B0))

/* Consumer Page: Extended Keyboard Attributes Collection [CL] */
#define HID_USAGE_CONSUMER_EXTENDED_KEYBOARD_ATTRIBUTES_COLLECTION                                 \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x2C0))

/* Consumer Page: Keyboard Form Factor [SV] */
#define HID_USAGE_CONSUMER_KEYBOARD_FORM_FACTOR (HID_USAGE(HID_USAGE_CONSUMER, 0x2C1))

/* Consumer Page: Keyboard Key Type [SV] */
#define HID_USAGE_CONSUMER_KEYBOARD_KEY_TYPE (HID_USAGE(HID_USAGE_CONSUMER, 0x2C2))

/* Consumer Page: Keyboard Physical Layout [SV] */
#define HID_USAGE_CONSUMER_KEYBOARD_PHYSICAL_LAYOUT (HID_USAGE(HID_USAGE_CONSUMER, 0x2C3))

/* Consumer Page: Vendor-Specific Keyboard Physical Layout [SV] */
#define HID_USAGE_CONSUMER_VENDOR_SPECIFIC_KEYBOARD_PHYSICAL_LAYOUT                                \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x2C4))

/* Consumer Page: Keyboard IETF Language Tag Index [SV] */
#define HID_USAGE_CONSUMER_KEYBOARD_IETF_LANGUAGE_TAG_INDEX (HID_USAGE(HID_USAGE_CONSUMER, 0x2C5))

/* Consumer Page: Implemented Keyboard Input Assist Controls [SV] */
#define HID_USAGE_CONSUMER_IMPLEMENTED_KEYBOARD_INPUT_ASSIST_CONTROLS                              \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x2C6))

/* Consumer Page: Keyboard Input Assist Previous [Sel] */
#define HID_USAGE_CONSUMER_KEYBOARD_INPUT_ASSIST_PREVIOUS (HID_USAGE(HID_USAGE_CONSUMER, 0x2C7))

/* Consumer Page: Keyboard Input Assist Next [Sel] */
#define HID_USAGE_CONSUMER_KEYBOARD_INPUT_ASSIST_NEXT (HID_USAGE(HID_USAGE_CONSUMER, 0x2C8))

/* Consumer Page: Keyboard Input Assist Previous Group [Sel] */
#define HID_USAGE_CONSUMER_KEYBOARD_INPUT_ASSIST_PREVIOUS_GROUP                                    \
    (HID_USAGE(HID_USAGE_CONSUMER, 0x2C9))

/* Consumer Page: Keyboard Input Assist Next Group [Sel] */
#define HID_USAGE_CONSUMER_KEYBOARD_INPUT_ASSIST_NEXT_GROUP (HID_USAGE(HID_USAGE_CONSUMER, 0x2CA))

/* Consumer Page: Keyboard Input Assist Accept [Sel] */
#define HID_USAGE_CONSUMER_KEYBOARD_INPUT_ASSIST_ACCEPT (HID_USAGE(HID_USAGE_CONSUMER, 0x2CB))

/* Consumer Page: Keyboard Input Assist Cancel [Sel] */
#define HID_USAGE_CONSUMER_KEYBOARD_INPUT_ASSIST_CANCEL (HID_USAGE(HID_USAGE_CONSUMER, 0x2CC))

/* Consumer Page: Privacy Screen Toggle [OOC] */
#define HID_USAGE_CONSUMER_PRIVACY_SCREEN_TOGGLE (HID_USAGE(HID_USAGE_CONSUMER, 0x2D0))

/* Consumer Page: Privacy Screen Level Decrement [RTC] */
#define HID_USAGE_CONSUMER_PRIVACY_SCREEN_LEVEL_DECREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x2D1))

/* Consumer Page: Privacy Screen Level Increment [RTC] */
#define HID_USAGE_CONSUMER_PRIVACY_SCREEN_LEVEL_INCREMENT (HID_USAGE(HID_USAGE_CONSUMER, 0x2D2))

/* Consumer Page: Privacy Screen Level Minimum [OSC] */
#define HID_USAGE_CONSUMER_PRIVACY_SCREEN_LEVEL_MINIMUM (HID_USAGE(HID_USAGE_CONSUMER, 0x2D3))

/* Consumer Page: Privacy Screen Level Maximum [OSC] */
#define HID_USAGE_CONSUMER_PRIVACY_SCREEN_LEVEL_MAXIMUM (HID_USAGE(HID_USAGE_CONSUMER, 0x2D4))

/* Consumer Page: Contact Edited [OOC] */
#define HID_USAGE_CONSUMER_CONTACT_EDITED (HID_USAGE(HID_USAGE_CONSUMER, 0x500))

/* Consumer Page: Contact Added [OOC] */
#define HID_USAGE_CONSUMER_CONTACT_ADDED (HID_USAGE(HID_USAGE_CONSUMER, 0x501))

/* Consumer Page: Contact Record Active [OOC] */
#define HID_USAGE_CONSUMER_CONTACT_RECORD_ACTIVE (HID_USAGE(HID_USAGE_CONSUMER, 0x502))

/* Consumer Page: Contact Index [DV] */
#define HID_USAGE_CONSUMER_CONTACT_INDEX (HID_USAGE(HID_USAGE_CONSUMER, 0x503))

/* Consumer Page: Contact Nickname [DV] */
#define HID_USAGE_CONSUMER_CONTACT_NICKNAME (HID_USAGE(HID_USAGE_CONSUMER, 0x504))

/* Consumer Page: Contact First Name [DV] */
#define HID_USAGE_CONSUMER_CONTACT_FIRST_NAME (HID_USAGE(HID_USAGE_CONSUMER, 0x505))

/* Consumer Page: Contact Last Name [DV] */
#define HID_USAGE_CONSUMER_CONTACT_LAST_NAME (HID_USAGE(HID_USAGE_CONSUMER, 0x506))

/* Consumer Page: Contact Full Name [DV] */
#define HID_USAGE_CONSUMER_CONTACT_FULL_NAME (HID_USAGE(HID_USAGE_CONSUMER, 0x507))

/* Consumer Page: Contact Phone Number Personal [DV] */
#define HID_USAGE_CONSUMER_CONTACT_PHONE_NUMBER_PERSONAL (HID_USAGE(HID_USAGE_CONSUMER, 0x508))

/* Consumer Page: Contact Phone Number Business [DV] */
#define HID_USAGE_CONSUMER_CONTACT_PHONE_NUMBER_BUSINESS (HID_USAGE(HID_USAGE_CONSUMER, 0x509))

/* Consumer Page: Contact Phone Number Mobile [DV] */
#define HID_USAGE_CONSUMER_CONTACT_PHONE_NUMBER_MOBILE (HID_USAGE(HID_USAGE_CONSUMER, 0x50A))

/* Consumer Page: Contact Phone Number Pager [DV] */
#define HID_USAGE_CONSUMER_CONTACT_PHONE_NUMBER_PAGER (HID_USAGE(HID_USAGE_CONSUMER, 0x50B))

/* Consumer Page: Contact Phone Number Fax [DV] */
#define HID_USAGE_CONSUMER_CONTACT_PHONE_NUMBER_FAX (HID_USAGE(HID_USAGE_CONSUMER, 0x50C))

/* Consumer Page: Contact Phone Number Other [DV] */
#define HID_USAGE_CONSUMER_CONTACT_PHONE_NUMBER_OTHER (HID_USAGE(HID_USAGE_CONSUMER, 0x50D))

/* Consumer Page: Contact Email Personal [DV] */
#define HID_USAGE_CONSUMER_CONTACT_EMAIL_PERSONAL (HID_USAGE(HID_USAGE_CONSUMER, 0x50E))

/* Consumer Page: Contact Email Business [DV] */
#define HID_USAGE_CONSUMER_CONTACT_EMAIL_BUSINESS (HID_USAGE(HID_USAGE_CONSUMER, 0x50F))

/* Consumer Page: Contact Email Other [DV] */
#define HID_USAGE_CONSUMER_CONTACT_EMAIL_OTHER (HID_USAGE(HID_USAGE_CONSUMER, 0x510))

/* Consumer Page: Contact Email Main [DV] */
#define HID_USAGE_CONSUMER_CONTACT_EMAIL_MAIN (HID_USAGE(HID_USAGE_CONSUMER, 0x511))

/* Consumer Page: Contact Speed Dial Number [DV] */
#define HID_USAGE_CONSUMER_CONTACT_SPEED_DIAL_NUMBER (HID_USAGE(HID_USAGE_CONSUMER, 0x512))

/* Consumer Page: Contact Status Flag [DV] */
#define HID_USAGE_CONSUMER_CONTACT_STATUS_FLAG (HID_USAGE(HID_USAGE_CONSUMER, 0x513))

/* Consumer Page: Contact Misc. [DV] */
#define HID_USAGE_CONSUMER_CONTACT_MISC (HID_USAGE(HID_USAGE_CONSUMER, 0x514))

/* Digitizers Page */
#define HID_USAGE_DIGITIZERS (0x0D)

/* Digitizers Page: Undefined */
#define HID_USAGE_DIGITIZERS_UNDEFINED (HID_USAGE(HID_USAGE_DIGITIZERS, 0x00))

/* Digitizers Page: Digitizer [CA] */
#define HID_USAGE_DIGITIZERS_DIGITIZER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x01))

/* Digitizers Page: Pen [CA] */
#define HID_USAGE_DIGITIZERS_PEN (HID_USAGE(HID_USAGE_DIGITIZERS, 0x02))

/* Digitizers Page: Light Pen [CA] */
#define HID_USAGE_DIGITIZERS_LIGHT_PEN (HID_USAGE(HID_USAGE_DIGITIZERS, 0x03))

/* Digitizers Page: Touch Screen [CA] */
#define HID_USAGE_DIGITIZERS_TOUCH_SCREEN (HID_USAGE(HID_USAGE_DIGITIZERS, 0x04))

/* Digitizers Page: Touch Pad [CA] */
#define HID_USAGE_DIGITIZERS_TOUCH_PAD (HID_USAGE(HID_USAGE_DIGITIZERS, 0x05))

/* Digitizers Page: Whiteboard [CA] */
#define HID_USAGE_DIGITIZERS_WHITEBOARD (HID_USAGE(HID_USAGE_DIGITIZERS, 0x06))

/* Digitizers Page: Coordinate Measuring Machine [CA] */
#define HID_USAGE_DIGITIZERS_COORDINATE_MEASURING_MACHINE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x07))

/* Digitizers Page: 3D Digitizer [CA] */
#define HID_USAGE_DIGITIZERS_3D_DIGITIZER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x08))

/* Digitizers Page: Stereo Plotter [CA] */
#define HID_USAGE_DIGITIZERS_STEREO_PLOTTER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x09))

/* Digitizers Page: Articulated Arm [CA] */
#define HID_USAGE_DIGITIZERS_ARTICULATED_ARM (HID_USAGE(HID_USAGE_DIGITIZERS, 0x0A))

/* Digitizers Page: Armature [CA] */
#define HID_USAGE_DIGITIZERS_ARMATURE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x0B))

/* Digitizers Page: Multiple Point Digitizer [CA] */
#define HID_USAGE_DIGITIZERS_MULTIPLE_POINT_DIGITIZER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x0C))

/* Digitizers Page: Free Space Wand [CA] */
#define HID_USAGE_DIGITIZERS_FREE_SPACE_WAND (HID_USAGE(HID_USAGE_DIGITIZERS, 0x0D))

/* Digitizers Page: Device Configuration [CA] */
#define HID_USAGE_DIGITIZERS_DEVICE_CONFIGURATION (HID_USAGE(HID_USAGE_DIGITIZERS, 0x0E))

/* Digitizers Page: Capacitive Heat Map Digitizer [CA] */
#define HID_USAGE_DIGITIZERS_CAPACITIVE_HEAT_MAP_DIGITIZER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x0F))

/* Digitizers Page: Stylus [CA, CL] */
#define HID_USAGE_DIGITIZERS_STYLUS (HID_USAGE(HID_USAGE_DIGITIZERS, 0x20))

/* Digitizers Page: Puck [CL] */
#define HID_USAGE_DIGITIZERS_PUCK (HID_USAGE(HID_USAGE_DIGITIZERS, 0x21))

/* Digitizers Page: Finger [CL] */
#define HID_USAGE_DIGITIZERS_FINGER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x22))

/* Digitizers Page: Device settings [CL] */
#define HID_USAGE_DIGITIZERS_DEVICE_SETTINGS (HID_USAGE(HID_USAGE_DIGITIZERS, 0x23))

/* Digitizers Page: Character Gesture [CL] */
#define HID_USAGE_DIGITIZERS_CHARACTER_GESTURE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x24))

/* Digitizers Page: Tip Pressure [DV] */
#define HID_USAGE_DIGITIZERS_TIP_PRESSURE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x30))

/* Digitizers Page: Barrel Pressure [DV] */
#define HID_USAGE_DIGITIZERS_BARREL_PRESSURE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x31))

/* Digitizers Page: In Range [MC] */
#define HID_USAGE_DIGITIZERS_IN_RANGE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x32))

/* Digitizers Page: Touch [MC] */
#define HID_USAGE_DIGITIZERS_TOUCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x33))

/* Digitizers Page: Untouch [OSC] */
#define HID_USAGE_DIGITIZERS_UNTOUCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x34))

/* Digitizers Page: Tap [OSC] */
#define HID_USAGE_DIGITIZERS_TAP (HID_USAGE(HID_USAGE_DIGITIZERS, 0x35))

/* Digitizers Page: Quality [DV] */
#define HID_USAGE_DIGITIZERS_QUALITY (HID_USAGE(HID_USAGE_DIGITIZERS, 0x36))

/* Digitizers Page: Data Valid [MC] */
#define HID_USAGE_DIGITIZERS_DATA_VALID (HID_USAGE(HID_USAGE_DIGITIZERS, 0x37))

/* Digitizers Page: Transducer Index [DV] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_INDEX (HID_USAGE(HID_USAGE_DIGITIZERS, 0x38))

/* Digitizers Page: Tablet Function Keys [CL] */
#define HID_USAGE_DIGITIZERS_TABLET_FUNCTION_KEYS (HID_USAGE(HID_USAGE_DIGITIZERS, 0x39))

/* Digitizers Page: Program Change Keys [CL] */
#define HID_USAGE_DIGITIZERS_PROGRAM_CHANGE_KEYS (HID_USAGE(HID_USAGE_DIGITIZERS, 0x3A))

/* Digitizers Page: Battery Strength [DV] */
#define HID_USAGE_DIGITIZERS_BATTERY_STRENGTH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x3B))

/* Digitizers Page: Invert [MC] */
#define HID_USAGE_DIGITIZERS_INVERT (HID_USAGE(HID_USAGE_DIGITIZERS, 0x3C))

/* Digitizers Page: X Tilt [DV] */
#define HID_USAGE_DIGITIZERS_X_TILT (HID_USAGE(HID_USAGE_DIGITIZERS, 0x3D))

/* Digitizers Page: Y Tilt [DV] */
#define HID_USAGE_DIGITIZERS_Y_TILT (HID_USAGE(HID_USAGE_DIGITIZERS, 0x3E))

/* Digitizers Page: Azimuth [DV] */
#define HID_USAGE_DIGITIZERS_AZIMUTH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x3F))

/* Digitizers Page: Altitude [DV] */
#define HID_USAGE_DIGITIZERS_ALTITUDE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x40))

/* Digitizers Page: Twist [DV] */
#define HID_USAGE_DIGITIZERS_TWIST (HID_USAGE(HID_USAGE_DIGITIZERS, 0x41))

/* Digitizers Page: Tip Switch [MC] */
#define HID_USAGE_DIGITIZERS_TIP_SWITCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x42))

/* Digitizers Page: Secondary Tip Switch [MC] */
#define HID_USAGE_DIGITIZERS_SECONDARY_TIP_SWITCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x43))

/* Digitizers Page: Barrel Switch [MC] */
#define HID_USAGE_DIGITIZERS_BARREL_SWITCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x44))

/* Digitizers Page: Eraser [MC] */
#define HID_USAGE_DIGITIZERS_ERASER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x45))

/* Digitizers Page: Tablet Pick [MC] */
#define HID_USAGE_DIGITIZERS_TABLET_PICK (HID_USAGE(HID_USAGE_DIGITIZERS, 0x46))

/* Digitizers Page: Touch Valid [MC] */
#define HID_USAGE_DIGITIZERS_TOUCH_VALID (HID_USAGE(HID_USAGE_DIGITIZERS, 0x47))

/* Digitizers Page: Width [DV] */
#define HID_USAGE_DIGITIZERS_WIDTH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x48))

/* Digitizers Page: Height [DV] */
#define HID_USAGE_DIGITIZERS_HEIGHT (HID_USAGE(HID_USAGE_DIGITIZERS, 0x49))

/* Digitizers Page: Contact Identifier [DV] */
#define HID_USAGE_DIGITIZERS_CONTACT_IDENTIFIER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x51))

/* Digitizers Page: Device Mode [DV] */
#define HID_USAGE_DIGITIZERS_DEVICE_MODE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x52))

/* Digitizers Page: Device Identifier [DV, SV] */
#define HID_USAGE_DIGITIZERS_DEVICE_IDENTIFIER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x53))

/* Digitizers Page: Contact Count [DV] */
#define HID_USAGE_DIGITIZERS_CONTACT_COUNT (HID_USAGE(HID_USAGE_DIGITIZERS, 0x54))

/* Digitizers Page: Contact Count Maximum [SV] */
#define HID_USAGE_DIGITIZERS_CONTACT_COUNT_MAXIMUM (HID_USAGE(HID_USAGE_DIGITIZERS, 0x55))

/* Digitizers Page: Scan Time [DV] */
#define HID_USAGE_DIGITIZERS_SCAN_TIME (HID_USAGE(HID_USAGE_DIGITIZERS, 0x56))

/* Digitizers Page: Surface Switch [DF] */
#define HID_USAGE_DIGITIZERS_SURFACE_SWITCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x57))

/* Digitizers Page: Button Switch [DF] */
#define HID_USAGE_DIGITIZERS_BUTTON_SWITCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x58))

/* Digitizers Page: Pad Type [SF] */
#define HID_USAGE_DIGITIZERS_PAD_TYPE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x59))

/* Digitizers Page: Secondary Barrel Switch [MC] */
#define HID_USAGE_DIGITIZERS_SECONDARY_BARREL_SWITCH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x5A))

/* Digitizers Page: Transducer Serial Number [SV] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_SERIAL_NUMBER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x5B))

/* Digitizers Page: Preferred Color [DV] */
#define HID_USAGE_DIGITIZERS_PREFERRED_COLOR (HID_USAGE(HID_USAGE_DIGITIZERS, 0x5C))

/* Digitizers Page: Preferred Color is Locked [MC] */
#define HID_USAGE_DIGITIZERS_PREFERRED_COLOR_IS_LOCKED (HID_USAGE(HID_USAGE_DIGITIZERS, 0x5D))

/* Digitizers Page: Preferred Line Width [DV] */
#define HID_USAGE_DIGITIZERS_PREFERRED_LINE_WIDTH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x5E))

/* Digitizers Page: Preferred Line Width is Locked [MC] */
#define HID_USAGE_DIGITIZERS_PREFERRED_LINE_WIDTH_IS_LOCKED (HID_USAGE(HID_USAGE_DIGITIZERS, 0x5F))

/* Digitizers Page: Latency Mode [DF] */
#define HID_USAGE_DIGITIZERS_LATENCY_MODE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x60))

/* Digitizers Page: Gesture Character Quality [DV] */
#define HID_USAGE_DIGITIZERS_GESTURE_CHARACTER_QUALITY (HID_USAGE(HID_USAGE_DIGITIZERS, 0x61))

/* Digitizers Page: Character Gesture Data Length [DV] */
#define HID_USAGE_DIGITIZERS_CHARACTER_GESTURE_DATA_LENGTH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x62))

/* Digitizers Page: Character Gesture Data [DV] */
#define HID_USAGE_DIGITIZERS_CHARACTER_GESTURE_DATA (HID_USAGE(HID_USAGE_DIGITIZERS, 0x63))

/* Digitizers Page: Gesture Character Encoding [NAry] */
#define HID_USAGE_DIGITIZERS_GESTURE_CHARACTER_ENCODING (HID_USAGE(HID_USAGE_DIGITIZERS, 0x64))

/* Digitizers Page: UTF8 Character Gesture Encoding [Sel] */
#define HID_USAGE_DIGITIZERS_UTF8_CHARACTER_GESTURE_ENCODING (HID_USAGE(HID_USAGE_DIGITIZERS, 0x65))

/* Digitizers Page: UTF16 Little Endian Character Gesture Encoding [Sel] */
#define HID_USAGE_DIGITIZERS_UTF16_LITTLE_ENDIAN_CHARACTER_GESTURE_ENCODING                        \
    (HID_USAGE(HID_USAGE_DIGITIZERS, 0x66))

/* Digitizers Page: UTF16 Big Endian Character Gesture Encoding [Sel] */
#define HID_USAGE_DIGITIZERS_UTF16_BIG_ENDIAN_CHARACTER_GESTURE_ENCODING                           \
    (HID_USAGE(HID_USAGE_DIGITIZERS, 0x67))

/* Digitizers Page: UTF32 Little Endian Character Gesture Encoding [Sel] */
#define HID_USAGE_DIGITIZERS_UTF32_LITTLE_ENDIAN_CHARACTER_GESTURE_ENCODING                        \
    (HID_USAGE(HID_USAGE_DIGITIZERS, 0x68))

/* Digitizers Page: UTF32 Big Endian Character Gesture Encoding [Sel] */
#define HID_USAGE_DIGITIZERS_UTF32_BIG_ENDIAN_CHARACTER_GESTURE_ENCODING                           \
    (HID_USAGE(HID_USAGE_DIGITIZERS, 0x69))

/* Digitizers Page: Capacitive Heat Map Protocol Vendor ID [SV] */
#define HID_USAGE_DIGITIZERS_CAPACITIVE_HEAT_MAP_PROTOCOL_VENDOR_ID                                \
    (HID_USAGE(HID_USAGE_DIGITIZERS, 0x6A))

/* Digitizers Page: Capacitive Heat Map Protocol Version [SV] */
#define HID_USAGE_DIGITIZERS_CAPACITIVE_HEAT_MAP_PROTOCOL_VERSION                                  \
    (HID_USAGE(HID_USAGE_DIGITIZERS, 0x6B))

/* Digitizers Page: Capacitive Heat Map Frame Data [DV] */
#define HID_USAGE_DIGITIZERS_CAPACITIVE_HEAT_MAP_FRAME_DATA (HID_USAGE(HID_USAGE_DIGITIZERS, 0x6C))

/* Digitizers Page: Gesture Character Enable [DF] */
#define HID_USAGE_DIGITIZERS_GESTURE_CHARACTER_ENABLE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x6D))

/* Digitizers Page: Preferred Line Style [NAry] */
#define HID_USAGE_DIGITIZERS_PREFERRED_LINE_STYLE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x70))

/* Digitizers Page: Preferred Line Style is Locked [MC] */
#define HID_USAGE_DIGITIZERS_PREFERRED_LINE_STYLE_IS_LOCKED (HID_USAGE(HID_USAGE_DIGITIZERS, 0x71))

/* Digitizers Page: Ink [Sel] */
#define HID_USAGE_DIGITIZERS_INK (HID_USAGE(HID_USAGE_DIGITIZERS, 0x72))

/* Digitizers Page: Pencil [Sel] */
#define HID_USAGE_DIGITIZERS_PENCIL (HID_USAGE(HID_USAGE_DIGITIZERS, 0x73))

/* Digitizers Page: Highlighter [Sel] */
#define HID_USAGE_DIGITIZERS_HIGHLIGHTER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x74))

/* Digitizers Page: Chisel Marker [Sel] */
#define HID_USAGE_DIGITIZERS_CHISEL_MARKER (HID_USAGE(HID_USAGE_DIGITIZERS, 0x75))

/* Digitizers Page: Brush [Sel] */
#define HID_USAGE_DIGITIZERS_BRUSH (HID_USAGE(HID_USAGE_DIGITIZERS, 0x76))

/* Digitizers Page: No Preference [Sel] */
#define HID_USAGE_DIGITIZERS_NO_PREFERENCE (HID_USAGE(HID_USAGE_DIGITIZERS, 0x77))

/* Digitizers Page: Digitizer Diagnostic [CL] */
#define HID_USAGE_DIGITIZERS_DIGITIZER_DIAGNOSTIC (HID_USAGE(HID_USAGE_DIGITIZERS, 0x80))

/* Digitizers Page: Digitizer Error [NAry] */
#define HID_USAGE_DIGITIZERS_DIGITIZER_ERROR (HID_USAGE(HID_USAGE_DIGITIZERS, 0x81))

/* Digitizers Page: Err Normal Status [Sel] */
#define HID_USAGE_DIGITIZERS_ERR_NORMAL_STATUS (HID_USAGE(HID_USAGE_DIGITIZERS, 0x82))

/* Digitizers Page: Err Transducers Exceeded [Sel] */
#define HID_USAGE_DIGITIZERS_ERR_TRANSDUCERS_EXCEEDED (HID_USAGE(HID_USAGE_DIGITIZERS, 0x83))

/* Digitizers Page: Err Full Trans Features Unavailable [Sel] */
#define HID_USAGE_DIGITIZERS_ERR_FULL_TRANS_FEATURES_UNAVAILABLE                                   \
    (HID_USAGE(HID_USAGE_DIGITIZERS, 0x84))

/* Digitizers Page: Err Charge Low [Sel] */
#define HID_USAGE_DIGITIZERS_ERR_CHARGE_LOW (HID_USAGE(HID_USAGE_DIGITIZERS, 0x85))

/* Digitizers Page: Transducer Software Info [CL] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_SOFTWARE_INFO (HID_USAGE(HID_USAGE_DIGITIZERS, 0x90))

/* Digitizers Page: Transducer Vendor Id [SV] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_VENDOR_ID (HID_USAGE(HID_USAGE_DIGITIZERS, 0x91))

/* Digitizers Page: Transducer Product Id [SV] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_PRODUCT_ID (HID_USAGE(HID_USAGE_DIGITIZERS, 0x92))

/* Digitizers Page: Device Supported Protocols [NAry, CL] */
#define HID_USAGE_DIGITIZERS_DEVICE_SUPPORTED_PROTOCOLS (HID_USAGE(HID_USAGE_DIGITIZERS, 0x93))

/* Digitizers Page: Transducer Supported Protocols [NAry, CL] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_SUPPORTED_PROTOCOLS (HID_USAGE(HID_USAGE_DIGITIZERS, 0x94))

/* Digitizers Page: No Protocol [Sel] */
#define HID_USAGE_DIGITIZERS_NO_PROTOCOL (HID_USAGE(HID_USAGE_DIGITIZERS, 0x95))

/* Digitizers Page: Wacom AES Protocol [Sel] */
#define HID_USAGE_DIGITIZERS_WACOM_AES_PROTOCOL (HID_USAGE(HID_USAGE_DIGITIZERS, 0x96))

/* Digitizers Page: USI Protocol [Sel] */
#define HID_USAGE_DIGITIZERS_USI_PROTOCOL (HID_USAGE(HID_USAGE_DIGITIZERS, 0x97))

/* Digitizers Page: Microsoft Pen Protocol [Sel] */
#define HID_USAGE_DIGITIZERS_MICROSOFT_PEN_PROTOCOL (HID_USAGE(HID_USAGE_DIGITIZERS, 0x98))

/* Digitizers Page: Supported Report Rates [SV, CL] */
#define HID_USAGE_DIGITIZERS_SUPPORTED_REPORT_RATES (HID_USAGE(HID_USAGE_DIGITIZERS, 0xA0))

/* Digitizers Page: Report Rate [DV] */
#define HID_USAGE_DIGITIZERS_REPORT_RATE (HID_USAGE(HID_USAGE_DIGITIZERS, 0xA1))

/* Digitizers Page: Transducer Connected [SF] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_CONNECTED (HID_USAGE(HID_USAGE_DIGITIZERS, 0xA2))

/* Digitizers Page: Switch Disabled [Sel] */
#define HID_USAGE_DIGITIZERS_SWITCH_DISABLED (HID_USAGE(HID_USAGE_DIGITIZERS, 0xA3))

/* Digitizers Page: Switch Unimplemented [Sel] */
#define HID_USAGE_DIGITIZERS_SWITCH_UNIMPLEMENTED (HID_USAGE(HID_USAGE_DIGITIZERS, 0xA4))

/* Digitizers Page: Transducer Switches [Sel] */
#define HID_USAGE_DIGITIZERS_TRANSDUCER_SWITCHES (HID_USAGE(HID_USAGE_DIGITIZERS, 0xA5))

/* Haptics Page */
#define HID_USAGE_HAPTICS (0x0E)

/* Haptics Page: Undefined */
#define HID_USAGE_HAPTICS_UNDEFINED (HID_USAGE(HID_USAGE_HAPTICS, 0x00))

/* Haptics Page: Simple Haptic Controller [CA, CL] */
#define HID_USAGE_HAPTICS_SIMPLE_HAPTIC_CONTROLLER (HID_USAGE(HID_USAGE_HAPTICS, 0x01))

/* Haptics Page: Waveform List [NAry] */
#define HID_USAGE_HAPTICS_WAVEFORM_LIST (HID_USAGE(HID_USAGE_HAPTICS, 0x10))

/* Haptics Page: Duration List [NAry] */
#define HID_USAGE_HAPTICS_DURATION_LIST (HID_USAGE(HID_USAGE_HAPTICS, 0x11))

/* Haptics Page: Auto Trigger [DV] */
#define HID_USAGE_HAPTICS_AUTO_TRIGGER (HID_USAGE(HID_USAGE_HAPTICS, 0x20))

/* Haptics Page: Manual Trigger [DV] */
#define HID_USAGE_HAPTICS_MANUAL_TRIGGER (HID_USAGE(HID_USAGE_HAPTICS, 0x21))

/* Haptics Page: Auto Trigger Associated Control [SV] */
#define HID_USAGE_HAPTICS_AUTO_TRIGGER_ASSOCIATED_CONTROL (HID_USAGE(HID_USAGE_HAPTICS, 0x22))

/* Haptics Page: Intensity [DV] */
#define HID_USAGE_HAPTICS_INTENSITY (HID_USAGE(HID_USAGE_HAPTICS, 0x23))

/* Haptics Page: Repeat Count [DV] */
#define HID_USAGE_HAPTICS_REPEAT_COUNT (HID_USAGE(HID_USAGE_HAPTICS, 0x24))

/* Haptics Page: Retrigger Period [DV] */
#define HID_USAGE_HAPTICS_RETRIGGER_PERIOD (HID_USAGE(HID_USAGE_HAPTICS, 0x25))

/* Haptics Page: Waveform Vendor Page [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_VENDOR_PAGE (HID_USAGE(HID_USAGE_HAPTICS, 0x26))

/* Haptics Page: Waveform Vendor ID [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_VENDOR_ID (HID_USAGE(HID_USAGE_HAPTICS, 0x27))

/* Haptics Page: Waveform Cutoff Time [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME (HID_USAGE(HID_USAGE_HAPTICS, 0x28))

/* Haptics Page: Waveform None [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_NONE (HID_USAGE(HID_USAGE_HAPTICS, 0x1001))

/* Haptics Page: Waveform Stop [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_STOP (HID_USAGE(HID_USAGE_HAPTICS, 0x1002))

/* Haptics Page: Waveform Click [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_CLICK (HID_USAGE(HID_USAGE_HAPTICS, 0x1003))

/* Haptics Page: Waveform Buzz Continuous [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_BUZZ_CONTINUOUS (HID_USAGE(HID_USAGE_HAPTICS, 0x1004))

/* Haptics Page: Waveform Rumble Continuous [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_RUMBLE_CONTINUOUS (HID_USAGE(HID_USAGE_HAPTICS, 0x1005))

/* Haptics Page: Waveform Press [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_PRESS (HID_USAGE(HID_USAGE_HAPTICS, 0x1006))

/* Haptics Page: Waveform Release [SV] */
#define HID_USAGE_HAPTICS_WAVEFORM_RELEASE (HID_USAGE(HID_USAGE_HAPTICS, 0x1007))

/* PID Page */
#define HID_USAGE_PID (0x0F)

/* PID Page: Undefined */
#define HID_USAGE_PID_UNDEFINED (HID_USAGE(HID_USAGE_PID, 0x00))

/* PID Page: Physical Interface Device */
#define HID_USAGE_PID_PHYSICAL_INTERFACE_DEVICE (HID_USAGE(HID_USAGE_PID, 0x01))

/* PID Page: Normal */
#define HID_USAGE_PID_NORMAL (HID_USAGE(HID_USAGE_PID, 0x20))

/* PID Page: Set Effect Report */
#define HID_USAGE_PID_SET_EFFECT_REPORT (HID_USAGE(HID_USAGE_PID, 0x21))

/* PID Page: Effect Block Index */
#define HID_USAGE_PID_EFFECT_BLOCK_INDEX (HID_USAGE(HID_USAGE_PID, 0x22))

/* PID Page: Parameter Block Offset */
#define HID_USAGE_PID_PARAMETER_BLOCK_OFFSET (HID_USAGE(HID_USAGE_PID, 0x23))

/* PID Page: ROM Flag */
#define HID_USAGE_PID_ROM_FLAG (HID_USAGE(HID_USAGE_PID, 0x24))

/* PID Page: Effect Type */
#define HID_USAGE_PID_EFFECT_TYPE (HID_USAGE(HID_USAGE_PID, 0x25))

/* PID Page: ET Constant Force */
#define HID_USAGE_PID_ET_CONSTANT_FORCE (HID_USAGE(HID_USAGE_PID, 0x26))

/* PID Page: ET Ramp */
#define HID_USAGE_PID_ET_RAMP (HID_USAGE(HID_USAGE_PID, 0x27))

/* PID Page: ET Custom Force Data */
#define HID_USAGE_PID_ET_CUSTOM_FORCE_DATA (HID_USAGE(HID_USAGE_PID, 0x28))

/* PID Page: ET Square */
#define HID_USAGE_PID_ET_SQUARE (HID_USAGE(HID_USAGE_PID, 0x30))

/* PID Page: ET Sine */
#define HID_USAGE_PID_ET_SINE (HID_USAGE(HID_USAGE_PID, 0x31))

/* PID Page: ET Triangle */
#define HID_USAGE_PID_ET_TRIANGLE (HID_USAGE(HID_USAGE_PID, 0x32))

/* PID Page: ET Sawtooth Up */
#define HID_USAGE_PID_ET_SAWTOOTH_UP (HID_USAGE(HID_USAGE_PID, 0x33))

/* PID Page: ET Sawtooth Down */
#define HID_USAGE_PID_ET_SAWTOOTH_DOWN (HID_USAGE(HID_USAGE_PID, 0x34))

/* PID Page: ET Spring */
#define HID_USAGE_PID_ET_SPRING (HID_USAGE(HID_USAGE_PID, 0x40))

/* PID Page: ET Damper */
#define HID_USAGE_PID_ET_DAMPER (HID_USAGE(HID_USAGE_PID, 0x41))

/* PID Page: ET Inertia */
#define HID_USAGE_PID_ET_INERTIA (HID_USAGE(HID_USAGE_PID, 0x42))

/* PID Page: ET Friction */
#define HID_USAGE_PID_ET_FRICTION (HID_USAGE(HID_USAGE_PID, 0x43))

/* PID Page: Duration */
#define HID_USAGE_PID_DURATION (HID_USAGE(HID_USAGE_PID, 0x50))

/* PID Page: Sample Period */
#define HID_USAGE_PID_SAMPLE_PERIOD (HID_USAGE(HID_USAGE_PID, 0x51))

/* PID Page: Gain */
#define HID_USAGE_PID_GAIN (HID_USAGE(HID_USAGE_PID, 0x52))

/* PID Page: Trigger Button */
#define HID_USAGE_PID_TRIGGER_BUTTON (HID_USAGE(HID_USAGE_PID, 0x53))

/* PID Page: Trigger Repeat Interval */
#define HID_USAGE_PID_TRIGGER_REPEAT_INTERVAL (HID_USAGE(HID_USAGE_PID, 0x54))

/* PID Page: Axes Enable */
#define HID_USAGE_PID_AXES_ENABLE (HID_USAGE(HID_USAGE_PID, 0x55))

/* PID Page: Direction Enable */
#define HID_USAGE_PID_DIRECTION_ENABLE (HID_USAGE(HID_USAGE_PID, 0x56))

/* PID Page: Direction */
#define HID_USAGE_PID_DIRECTION (HID_USAGE(HID_USAGE_PID, 0x57))

/* PID Page: Type Specific Block Offset */
#define HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_OFFSET (HID_USAGE(HID_USAGE_PID, 0x58))

/* PID Page: Block Type */
#define HID_USAGE_PID_BLOCK_TYPE (HID_USAGE(HID_USAGE_PID, 0x59))

/* PID Page: Set Envelope Report */
#define HID_USAGE_PID_SET_ENVELOPE_REPORT (HID_USAGE(HID_USAGE_PID, 0x5A))

/* PID Page: Attack Level */
#define HID_USAGE_PID_ATTACK_LEVEL (HID_USAGE(HID_USAGE_PID, 0x5B))

/* PID Page: Attack Time */
#define HID_USAGE_PID_ATTACK_TIME (HID_USAGE(HID_USAGE_PID, 0x5C))

/* PID Page: Fade Level */
#define HID_USAGE_PID_FADE_LEVEL (HID_USAGE(HID_USAGE_PID, 0x5D))

/* PID Page: Fade Time */
#define HID_USAGE_PID_FADE_TIME (HID_USAGE(HID_USAGE_PID, 0x5E))

/* PID Page: Set Condition Report */
#define HID_USAGE_PID_SET_CONDITION_REPORT (HID_USAGE(HID_USAGE_PID, 0x5F))

/* PID Page: CP Offset */
#define HID_USAGE_PID_CP_OFFSET (HID_USAGE(HID_USAGE_PID, 0x60))

/* PID Page: Positive Coefficient */
#define HID_USAGE_PID_POSITIVE_COEFFICIENT (HID_USAGE(HID_USAGE_PID, 0x61))

/* PID Page: Negative Coefficient */
#define HID_USAGE_PID_NEGATIVE_COEFFICIENT (HID_USAGE(HID_USAGE_PID, 0x62))

/* PID Page: Positive Saturation */
#define HID_USAGE_PID_POSITIVE_SATURATION (HID_USAGE(HID_USAGE_PID, 0x63))

/* PID Page: Negative Saturation */
#define HID_USAGE_PID_NEGATIVE_SATURATION (HID_USAGE(HID_USAGE_PID, 0x64))

/* PID Page: Dead Band */
#define HID_USAGE_PID_DEAD_BAND (HID_USAGE(HID_USAGE_PID, 0x65))

/* PID Page: Download Force Sample */
#define HID_USAGE_PID_DOWNLOAD_FORCE_SAMPLE (HID_USAGE(HID_USAGE_PID, 0x66))

/* PID Page: Isoch Custom Force Enable */
#define HID_USAGE_PID_ISOCH_CUSTOM_FORCE_ENABLE (HID_USAGE(HID_USAGE_PID, 0x67))

/* PID Page: Custom Force Data Report */
#define HID_USAGE_PID_CUSTOM_FORCE_DATA_REPORT (HID_USAGE(HID_USAGE_PID, 0x68))

/* PID Page: Custom Force Data */
#define HID_USAGE_PID_CUSTOM_FORCE_DATA (HID_USAGE(HID_USAGE_PID, 0x69))

/* PID Page: Custom Force Vendor Defined Data */
#define HID_USAGE_PID_CUSTOM_FORCE_VENDOR_DEFINED_DATA (HID_USAGE(HID_USAGE_PID, 0x6A))

/* PID Page: Set Custom Force Report */
#define HID_USAGE_PID_SET_CUSTOM_FORCE_REPORT (HID_USAGE(HID_USAGE_PID, 0x6B))

/* PID Page: Custom Force Data Offset */
#define HID_USAGE_PID_CUSTOM_FORCE_DATA_OFFSET (HID_USAGE(HID_USAGE_PID, 0x6C))

/* PID Page: Sample Count */
#define HID_USAGE_PID_SAMPLE_COUNT (HID_USAGE(HID_USAGE_PID, 0x6D))

/* PID Page: Set Periodic Report */
#define HID_USAGE_PID_SET_PERIODIC_REPORT (HID_USAGE(HID_USAGE_PID, 0x6E))

/* PID Page: Offset */
#define HID_USAGE_PID_OFFSET (HID_USAGE(HID_USAGE_PID, 0x6F))

/* PID Page: Magnitude */
#define HID_USAGE_PID_MAGNITUDE (HID_USAGE(HID_USAGE_PID, 0x70))

/* PID Page: Phase */
#define HID_USAGE_PID_PHASE (HID_USAGE(HID_USAGE_PID, 0x71))

/* PID Page: Period */
#define HID_USAGE_PID_PERIOD (HID_USAGE(HID_USAGE_PID, 0x72))

/* PID Page: Set Constant Force Report */
#define HID_USAGE_PID_SET_CONSTANT_FORCE_REPORT (HID_USAGE(HID_USAGE_PID, 0x73))

/* PID Page: Set Ramp Force Report */
#define HID_USAGE_PID_SET_RAMP_FORCE_REPORT (HID_USAGE(HID_USAGE_PID, 0x74))

/* PID Page: Ramp Start */
#define HID_USAGE_PID_RAMP_START (HID_USAGE(HID_USAGE_PID, 0x75))

/* PID Page: Ramp End */
#define HID_USAGE_PID_RAMP_END (HID_USAGE(HID_USAGE_PID, 0x76))

/* PID Page: Effect Operation Report */
#define HID_USAGE_PID_EFFECT_OPERATION_REPORT (HID_USAGE(HID_USAGE_PID, 0x77))

/* PID Page: Effect Operation */
#define HID_USAGE_PID_EFFECT_OPERATION (HID_USAGE(HID_USAGE_PID, 0x78))

/* PID Page: Op Effect Start */
#define HID_USAGE_PID_OP_EFFECT_START (HID_USAGE(HID_USAGE_PID, 0x79))

/* PID Page: Op Effect Start Solo */
#define HID_USAGE_PID_OP_EFFECT_START_SOLO (HID_USAGE(HID_USAGE_PID, 0x7A))

/* PID Page: Op Effect Stop */
#define HID_USAGE_PID_OP_EFFECT_STOP (HID_USAGE(HID_USAGE_PID, 0x7B))

/* PID Page: Loop Count */
#define HID_USAGE_PID_LOOP_COUNT (HID_USAGE(HID_USAGE_PID, 0x7C))

/* PID Page: Device Gain Report */
#define HID_USAGE_PID_DEVICE_GAIN_REPORT (HID_USAGE(HID_USAGE_PID, 0x7D))

/* PID Page: Device Gain */
#define HID_USAGE_PID_DEVICE_GAIN (HID_USAGE(HID_USAGE_PID, 0x7E))

/* PID Page: PID Pool Report */
#define HID_USAGE_PID_PID_POOL_REPORT (HID_USAGE(HID_USAGE_PID, 0x7F))

/* PID Page: RAM Pool Size */
#define HID_USAGE_PID_RAM_POOL_SIZE (HID_USAGE(HID_USAGE_PID, 0x80))

/* PID Page: ROM Pool Size */
#define HID_USAGE_PID_ROM_POOL_SIZE (HID_USAGE(HID_USAGE_PID, 0x81))

/* PID Page: ROM Effect Block Count */
#define HID_USAGE_PID_ROM_EFFECT_BLOCK_COUNT (HID_USAGE(HID_USAGE_PID, 0x82))

/* PID Page: Simultaneous Effects Max */
#define HID_USAGE_PID_SIMULTANEOUS_EFFECTS_MAX (HID_USAGE(HID_USAGE_PID, 0x83))

/* PID Page: Pool Alignment */
#define HID_USAGE_PID_POOL_ALIGNMENT (HID_USAGE(HID_USAGE_PID, 0x84))

/* PID Page: PID Pool Move Report */
#define HID_USAGE_PID_PID_POOL_MOVE_REPORT (HID_USAGE(HID_USAGE_PID, 0x85))

/* PID Page: Move Source */
#define HID_USAGE_PID_MOVE_SOURCE (HID_USAGE(HID_USAGE_PID, 0x86))

/* PID Page: Move Destination */
#define HID_USAGE_PID_MOVE_DESTINATION (HID_USAGE(HID_USAGE_PID, 0x87))

/* PID Page: Move Length */
#define HID_USAGE_PID_MOVE_LENGTH (HID_USAGE(HID_USAGE_PID, 0x88))

/* PID Page: PID Block Load Report */
#define HID_USAGE_PID_PID_BLOCK_LOAD_REPORT (HID_USAGE(HID_USAGE_PID, 0x89))

/* PID Page: Block Load Status */
#define HID_USAGE_PID_BLOCK_LOAD_STATUS (HID_USAGE(HID_USAGE_PID, 0x8B))

/* PID Page: Block Load Success */
#define HID_USAGE_PID_BLOCK_LOAD_SUCCESS (HID_USAGE(HID_USAGE_PID, 0x8C))

/* PID Page: Block Load Full */
#define HID_USAGE_PID_BLOCK_LOAD_FULL (HID_USAGE(HID_USAGE_PID, 0x8D))

/* PID Page: Block Load Error */
#define HID_USAGE_PID_BLOCK_LOAD_ERROR (HID_USAGE(HID_USAGE_PID, 0x8E))

/* PID Page: Block Handle */
#define HID_USAGE_PID_BLOCK_HANDLE (HID_USAGE(HID_USAGE_PID, 0x8F))

/* PID Page: PID Block Free Report */
#define HID_USAGE_PID_PID_BLOCK_FREE_REPORT (HID_USAGE(HID_USAGE_PID, 0x90))

/* PID Page: Type Specific Block Handle */
#define HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_HANDLE (HID_USAGE(HID_USAGE_PID, 0x91))

/* PID Page: PID State Report */
#define HID_USAGE_PID_PID_STATE_REPORT (HID_USAGE(HID_USAGE_PID, 0x92))

/* PID Page: Effect Playing */
#define HID_USAGE_PID_EFFECT_PLAYING (HID_USAGE(HID_USAGE_PID, 0x94))

/* PID Page: PID Device Control Report */
#define HID_USAGE_PID_PID_DEVICE_CONTROL_REPORT (HID_USAGE(HID_USAGE_PID, 0x95))

/* PID Page: PID Device Control */
#define HID_USAGE_PID_PID_DEVICE_CONTROL (HID_USAGE(HID_USAGE_PID, 0x96))

/* PID Page: DC Enable Actuators */
#define HID_USAGE_PID_DC_ENABLE_ACTUATORS (HID_USAGE(HID_USAGE_PID, 0x97))

/* PID Page: DC Disable Actuators */
#define HID_USAGE_PID_DC_DISABLE_ACTUATORS (HID_USAGE(HID_USAGE_PID, 0x98))

/* PID Page: DC Stop All Effects */
#define HID_USAGE_PID_DC_STOP_ALL_EFFECTS (HID_USAGE(HID_USAGE_PID, 0x99))

/* PID Page: DC Device Reset */
#define HID_USAGE_PID_DC_DEVICE_RESET (HID_USAGE(HID_USAGE_PID, 0x9A))

/* PID Page: DC Device Pause */
#define HID_USAGE_PID_DC_DEVICE_PAUSE (HID_USAGE(HID_USAGE_PID, 0x9B))

/* PID Page: DC Device Continue */
#define HID_USAGE_PID_DC_DEVICE_CONTINUE (HID_USAGE(HID_USAGE_PID, 0x9C))

/* PID Page: Device Paused */
#define HID_USAGE_PID_DEVICE_PAUSED (HID_USAGE(HID_USAGE_PID, 0x9F))

/* PID Page: Actuators Enabled */
#define HID_USAGE_PID_ACTUATORS_ENABLED (HID_USAGE(HID_USAGE_PID, 0xA0))

/* PID Page: Safety Switch */
#define HID_USAGE_PID_SAFETY_SWITCH (HID_USAGE(HID_USAGE_PID, 0xA4))

/* PID Page: Actuator Override Switch */
#define HID_USAGE_PID_ACTUATOR_OVERRIDE_SWITCH (HID_USAGE(HID_USAGE_PID, 0xA5))

/* PID Page: Actuator Power */
#define HID_USAGE_PID_ACTUATOR_POWER (HID_USAGE(HID_USAGE_PID, 0xA6))

/* PID Page: Start Delay */
#define HID_USAGE_PID_START_DELAY (HID_USAGE(HID_USAGE_PID, 0xA7))

/* PID Page: Parameter Block Size */
#define HID_USAGE_PID_PARAMETER_BLOCK_SIZE (HID_USAGE(HID_USAGE_PID, 0xA8))

/* PID Page: Device Managed Pool */
#define HID_USAGE_PID_DEVICE_MANAGED_POOL (HID_USAGE(HID_USAGE_PID, 0xA9))

/* PID Page: Shared Parameter Blocks */
#define HID_USAGE_PID_SHARED_PARAMETER_BLOCKS (HID_USAGE(HID_USAGE_PID, 0xAA))

/* PID Page: Create New Effect Report */
#define HID_USAGE_PID_CREATE_NEW_EFFECT_REPORT (HID_USAGE(HID_USAGE_PID, 0xAB))

/* PID Page: RAM Pool Available */
#define HID_USAGE_PID_RAM_POOL_AVAILABLE (HID_USAGE(HID_USAGE_PID, 0xAC))

/* Eye and Head Trackers Page */
#define HID_USAGE_EHT (0x12)

/* Eye and Head Trackers Page: Undefined */
#define HID_USAGE_EHT_UNDEFINED (HID_USAGE(HID_USAGE_EHT, 0x00))

/* Eye and Head Trackers Page: Eye Tracker [CA] */
#define HID_USAGE_EHT_EYE_TRACKER (HID_USAGE(HID_USAGE_EHT, 0x01))

/* Eye and Head Trackers Page: Head Tracker [CA] */
#define HID_USAGE_EHT_HEAD_TRACKER (HID_USAGE(HID_USAGE_EHT, 0x02))

/* Eye and Head Trackers Page: Tracking Data [CP] */
#define HID_USAGE_EHT_TRACKING_DATA (HID_USAGE(HID_USAGE_EHT, 0x10))

/* Eye and Head Trackers Page: Capabilities [CL] */
#define HID_USAGE_EHT_CAPABILITIES (HID_USAGE(HID_USAGE_EHT, 0x11))

/* Eye and Head Trackers Page: Configuration [CL] */
#define HID_USAGE_EHT_CONFIGURATION (HID_USAGE(HID_USAGE_EHT, 0x12))

/* Eye and Head Trackers Page: Status [CL] */
#define HID_USAGE_EHT_STATUS (HID_USAGE(HID_USAGE_EHT, 0x13))

/* Eye and Head Trackers Page: Control [CL] */
#define HID_USAGE_EHT_CONTROL (HID_USAGE(HID_USAGE_EHT, 0x14))

/* Eye and Head Trackers Page: Sensor Timestamp [DV] */
#define HID_USAGE_EHT_SENSOR_TIMESTAMP (HID_USAGE(HID_USAGE_EHT, 0x20))

/* Eye and Head Trackers Page: Position X [DV] */
#define HID_USAGE_EHT_POSITION_X (HID_USAGE(HID_USAGE_EHT, 0x21))

/* Eye and Head Trackers Page: Position Y [DV] */
#define HID_USAGE_EHT_POSITION_Y (HID_USAGE(HID_USAGE_EHT, 0x22))

/* Eye and Head Trackers Page: Position Z [DV] */
#define HID_USAGE_EHT_POSITION_Z (HID_USAGE(HID_USAGE_EHT, 0x23))

/* Eye and Head Trackers Page: Gaze Point [CP] */
#define HID_USAGE_EHT_GAZE_POINT (HID_USAGE(HID_USAGE_EHT, 0x24))

/* Eye and Head Trackers Page: Left Eye Position [CP] */
#define HID_USAGE_EHT_LEFT_EYE_POSITION (HID_USAGE(HID_USAGE_EHT, 0x25))

/* Eye and Head Trackers Page: Right Eye Position [CP] */
#define HID_USAGE_EHT_RIGHT_EYE_POSITION (HID_USAGE(HID_USAGE_EHT, 0x26))

/* Eye and Head Trackers Page: Head Position [CP] */
#define HID_USAGE_EHT_HEAD_POSITION (HID_USAGE(HID_USAGE_EHT, 0x27))

/* Eye and Head Trackers Page: Head Direction Point [CP] */
#define HID_USAGE_EHT_HEAD_DIRECTION_POINT (HID_USAGE(HID_USAGE_EHT, 0x28))

/* Eye and Head Trackers Page: Rotation about X axis [DV] */
#define HID_USAGE_EHT_ROTATION_ABOUT_X_AXIS (HID_USAGE(HID_USAGE_EHT, 0x29))

/* Eye and Head Trackers Page: Rotation about Y axis [DV] */
#define HID_USAGE_EHT_ROTATION_ABOUT_Y_AXIS (HID_USAGE(HID_USAGE_EHT, 0x2A))

/* Eye and Head Trackers Page: Rotation about Z axis [DV] */
#define HID_USAGE_EHT_ROTATION_ABOUT_Z_AXIS (HID_USAGE(HID_USAGE_EHT, 0x2B))

/* Eye and Head Trackers Page: Tracker Quality [SV] */
#define HID_USAGE_EHT_TRACKER_QUALITY (HID_USAGE(HID_USAGE_EHT, 0x100))

/* Eye and Head Trackers Page: Minimum Tracking Distance [SV] */
#define HID_USAGE_EHT_MINIMUM_TRACKING_DISTANCE (HID_USAGE(HID_USAGE_EHT, 0x101))

/* Eye and Head Trackers Page: Optimum Tracking Distance [SV] */
#define HID_USAGE_EHT_OPTIMUM_TRACKING_DISTANCE (HID_USAGE(HID_USAGE_EHT, 0x102))

/* Eye and Head Trackers Page: Maximum Tracking Distance [SV] */
#define HID_USAGE_EHT_MAXIMUM_TRACKING_DISTANCE (HID_USAGE(HID_USAGE_EHT, 0x103))

/* Eye and Head Trackers Page: Maximum Screen Plane Width [SV] */
#define HID_USAGE_EHT_MAXIMUM_SCREEN_PLANE_WIDTH (HID_USAGE(HID_USAGE_EHT, 0x104))

/* Eye and Head Trackers Page: Maximum Screen Plane Height [SV] */
#define HID_USAGE_EHT_MAXIMUM_SCREEN_PLANE_HEIGHT (HID_USAGE(HID_USAGE_EHT, 0x105))

/* Eye and Head Trackers Page: Display Manufacturer ID [SV] */
#define HID_USAGE_EHT_DISPLAY_MANUFACTURER_ID (HID_USAGE(HID_USAGE_EHT, 0x200))

/* Eye and Head Trackers Page: Display Product ID [SV] */
#define HID_USAGE_EHT_DISPLAY_PRODUCT_ID (HID_USAGE(HID_USAGE_EHT, 0x201))

/* Eye and Head Trackers Page: Display Serial Number [SV] */
#define HID_USAGE_EHT_DISPLAY_SERIAL_NUMBER (HID_USAGE(HID_USAGE_EHT, 0x202))

/* Eye and Head Trackers Page: Display Manufacturer Date [SV] */
#define HID_USAGE_EHT_DISPLAY_MANUFACTURER_DATE (HID_USAGE(HID_USAGE_EHT, 0x203))

/* Eye and Head Trackers Page: Calibrated Screen Width [SV] */
#define HID_USAGE_EHT_CALIBRATED_SCREEN_WIDTH (HID_USAGE(HID_USAGE_EHT, 0x204))

/* Eye and Head Trackers Page: Calibrated Screen Height [SV] */
#define HID_USAGE_EHT_CALIBRATED_SCREEN_HEIGHT (HID_USAGE(HID_USAGE_EHT, 0x205))

/* Eye and Head Trackers Page: Sampling Frequency [DV] */
#define HID_USAGE_EHT_SAMPLING_FREQUENCY (HID_USAGE(HID_USAGE_EHT, 0x300))

/* Eye and Head Trackers Page: Configuration Status [DV] */
#define HID_USAGE_EHT_CONFIGURATION_STATUS (HID_USAGE(HID_USAGE_EHT, 0x301))

/* Eye and Head Trackers Page: Device Mode Request [DV] */
#define HID_USAGE_EHT_DEVICE_MODE_REQUEST (HID_USAGE(HID_USAGE_EHT, 0x400))

/* Auxiliary Display Page */
#define HID_USAGE_AUXDISP (0x14)

/* Auxiliary Display Page: Undefined */
#define HID_USAGE_AUXDISP_UNDEFINED (HID_USAGE(HID_USAGE_AUXDISP, 0x00))

/* Auxiliary Display Page: Alphanumeric Display [CA] */
#define HID_USAGE_AUXDISP_ALPHANUMERIC_DISPLAY (HID_USAGE(HID_USAGE_AUXDISP, 0x01))

/* Auxiliary Display Page: Auxiliary Display [CA] */
#define HID_USAGE_AUXDISP_AUXILIARY_DISPLAY (HID_USAGE(HID_USAGE_AUXDISP, 0x02))

/* Auxiliary Display Page: Display Attributes Report [CL] */
#define HID_USAGE_AUXDISP_DISPLAY_ATTRIBUTES_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x20))

/* Auxiliary Display Page: ASCII Character Set [SF] */
#define HID_USAGE_AUXDISP_ASCII_CHARACTER_SET (HID_USAGE(HID_USAGE_AUXDISP, 0x21))

/* Auxiliary Display Page: Data Read Back [SF] */
#define HID_USAGE_AUXDISP_DATA_READ_BACK (HID_USAGE(HID_USAGE_AUXDISP, 0x22))

/* Auxiliary Display Page: Font Read Back [SF] */
#define HID_USAGE_AUXDISP_FONT_READ_BACK (HID_USAGE(HID_USAGE_AUXDISP, 0x23))

/* Auxiliary Display Page: Display Control Report [CL] */
#define HID_USAGE_AUXDISP_DISPLAY_CONTROL_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x24))

/* Auxiliary Display Page: Clear Display [DF] */
#define HID_USAGE_AUXDISP_CLEAR_DISPLAY (HID_USAGE(HID_USAGE_AUXDISP, 0x25))

/* Auxiliary Display Page: Display Enable [DF] */
#define HID_USAGE_AUXDISP_DISPLAY_ENABLE (HID_USAGE(HID_USAGE_AUXDISP, 0x26))

/* Auxiliary Display Page: Screen Saver Delay [SV, DV] */
#define HID_USAGE_AUXDISP_SCREEN_SAVER_DELAY (HID_USAGE(HID_USAGE_AUXDISP, 0x27))

/* Auxiliary Display Page: Screen Saver Enable [DF] */
#define HID_USAGE_AUXDISP_SCREEN_SAVER_ENABLE (HID_USAGE(HID_USAGE_AUXDISP, 0x28))

/* Auxiliary Display Page: Vertical Scroll [SF, DF] */
#define HID_USAGE_AUXDISP_VERTICAL_SCROLL (HID_USAGE(HID_USAGE_AUXDISP, 0x29))

/* Auxiliary Display Page: Horizontal Scroll [SF, DF] */
#define HID_USAGE_AUXDISP_HORIZONTAL_SCROLL (HID_USAGE(HID_USAGE_AUXDISP, 0x2A))

/* Auxiliary Display Page: Character Report [CL] */
#define HID_USAGE_AUXDISP_CHARACTER_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x2B))

/* Auxiliary Display Page: Display Data [DV] */
#define HID_USAGE_AUXDISP_DISPLAY_DATA (HID_USAGE(HID_USAGE_AUXDISP, 0x2C))

/* Auxiliary Display Page: Display Status [CL] */
#define HID_USAGE_AUXDISP_DISPLAY_STATUS (HID_USAGE(HID_USAGE_AUXDISP, 0x2D))

/* Auxiliary Display Page: Stat Not Ready [Sel] */
#define HID_USAGE_AUXDISP_STAT_NOT_READY (HID_USAGE(HID_USAGE_AUXDISP, 0x2E))

/* Auxiliary Display Page: Stat Ready [Sel] */
#define HID_USAGE_AUXDISP_STAT_READY (HID_USAGE(HID_USAGE_AUXDISP, 0x2F))

/* Auxiliary Display Page: Err Not a loadable character [Sel] */
#define HID_USAGE_AUXDISP_ERR_NOT_A_LOADABLE_CHARACTER (HID_USAGE(HID_USAGE_AUXDISP, 0x30))

/* Auxiliary Display Page: Err Font data cannot be read [Sel] */
#define HID_USAGE_AUXDISP_ERR_FONT_DATA_CANNOT_BE_READ (HID_USAGE(HID_USAGE_AUXDISP, 0x31))

/* Auxiliary Display Page: Cursor Position Report [Sel] */
#define HID_USAGE_AUXDISP_CURSOR_POSITION_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x32))

/* Auxiliary Display Page: Row [DV] */
#define HID_USAGE_AUXDISP_ROW (HID_USAGE(HID_USAGE_AUXDISP, 0x33))

/* Auxiliary Display Page: Column [DV] */
#define HID_USAGE_AUXDISP_COLUMN (HID_USAGE(HID_USAGE_AUXDISP, 0x34))

/* Auxiliary Display Page: Rows [SV] */
#define HID_USAGE_AUXDISP_ROWS (HID_USAGE(HID_USAGE_AUXDISP, 0x35))

/* Auxiliary Display Page: Columns [SV] */
#define HID_USAGE_AUXDISP_COLUMNS (HID_USAGE(HID_USAGE_AUXDISP, 0x36))

/* Auxiliary Display Page: Cursor Pixel Positioning [SF] */
#define HID_USAGE_AUXDISP_CURSOR_PIXEL_POSITIONING (HID_USAGE(HID_USAGE_AUXDISP, 0x37))

/* Auxiliary Display Page: Cursor Mode [DF] */
#define HID_USAGE_AUXDISP_CURSOR_MODE (HID_USAGE(HID_USAGE_AUXDISP, 0x38))

/* Auxiliary Display Page: Cursor Enable [DF] */
#define HID_USAGE_AUXDISP_CURSOR_ENABLE (HID_USAGE(HID_USAGE_AUXDISP, 0x39))

/* Auxiliary Display Page: Cursor Blink [DF] */
#define HID_USAGE_AUXDISP_CURSOR_BLINK (HID_USAGE(HID_USAGE_AUXDISP, 0x3A))

/* Auxiliary Display Page: Font Report [CL] */
#define HID_USAGE_AUXDISP_FONT_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x3B))

/* Auxiliary Display Page: Font Data [Buffered Bytes] */
#define HID_USAGE_AUXDISP_FONT_DATA (HID_USAGE(HID_USAGE_AUXDISP, 0x3C))

/* Auxiliary Display Page: Character Width [SV] */
#define HID_USAGE_AUXDISP_CHARACTER_WIDTH (HID_USAGE(HID_USAGE_AUXDISP, 0x3D))

/* Auxiliary Display Page: Character Height [SV] */
#define HID_USAGE_AUXDISP_CHARACTER_HEIGHT (HID_USAGE(HID_USAGE_AUXDISP, 0x3E))

/* Auxiliary Display Page: Character Spacing Horizontal [SV] */
#define HID_USAGE_AUXDISP_CHARACTER_SPACING_HORIZONTAL (HID_USAGE(HID_USAGE_AUXDISP, 0x3F))

/* Auxiliary Display Page: Character Spacing Vertical [SV] */
#define HID_USAGE_AUXDISP_CHARACTER_SPACING_VERTICAL (HID_USAGE(HID_USAGE_AUXDISP, 0x40))

/* Auxiliary Display Page: Unicode Character Set [SF] */
#define HID_USAGE_AUXDISP_UNICODE_CHARACTER_SET (HID_USAGE(HID_USAGE_AUXDISP, 0x41))

/* Auxiliary Display Page: Font 7-Segment [SF] */
#define HID_USAGE_AUXDISP_FONT_7_SEGMENT (HID_USAGE(HID_USAGE_AUXDISP, 0x42))

/* Auxiliary Display Page: 7-Segment Direct Map [SF] */
#define HID_USAGE_AUXDISP_7_SEGMENT_DIRECT_MAP (HID_USAGE(HID_USAGE_AUXDISP, 0x43))

/* Auxiliary Display Page: Font 14-Segment [SF] */
#define HID_USAGE_AUXDISP_FONT_14_SEGMENT (HID_USAGE(HID_USAGE_AUXDISP, 0x44))

/* Auxiliary Display Page: 14-Segment Direct Map [SF] */
#define HID_USAGE_AUXDISP_14_SEGMENT_DIRECT_MAP (HID_USAGE(HID_USAGE_AUXDISP, 0x45))

/* Auxiliary Display Page: Display Brightness [DV] */
#define HID_USAGE_AUXDISP_DISPLAY_BRIGHTNESS (HID_USAGE(HID_USAGE_AUXDISP, 0x46))

/* Auxiliary Display Page: Display Contrast [DV] */
#define HID_USAGE_AUXDISP_DISPLAY_CONTRAST (HID_USAGE(HID_USAGE_AUXDISP, 0x47))

/* Auxiliary Display Page: Character Attribute [CL] */
#define HID_USAGE_AUXDISP_CHARACTER_ATTRIBUTE (HID_USAGE(HID_USAGE_AUXDISP, 0x48))

/* Auxiliary Display Page: Attribute Readback [SF] */
#define HID_USAGE_AUXDISP_ATTRIBUTE_READBACK (HID_USAGE(HID_USAGE_AUXDISP, 0x49))

/* Auxiliary Display Page: Attribute Data [DV] */
#define HID_USAGE_AUXDISP_ATTRIBUTE_DATA (HID_USAGE(HID_USAGE_AUXDISP, 0x4A))

/* Auxiliary Display Page: Char Attr Enhance [OOC] */
#define HID_USAGE_AUXDISP_CHAR_ATTR_ENHANCE (HID_USAGE(HID_USAGE_AUXDISP, 0x4B))

/* Auxiliary Display Page: Char Attr Underline [OOC] */
#define HID_USAGE_AUXDISP_CHAR_ATTR_UNDERLINE (HID_USAGE(HID_USAGE_AUXDISP, 0x4C))

/* Auxiliary Display Page: Char Attr Blink [OOC] */
#define HID_USAGE_AUXDISP_CHAR_ATTR_BLINK (HID_USAGE(HID_USAGE_AUXDISP, 0x4D))

/* Auxiliary Display Page: Bitmap Size X [SV] */
#define HID_USAGE_AUXDISP_BITMAP_SIZE_X (HID_USAGE(HID_USAGE_AUXDISP, 0x80))

/* Auxiliary Display Page: Bitmap Size Y [SV] */
#define HID_USAGE_AUXDISP_BITMAP_SIZE_Y (HID_USAGE(HID_USAGE_AUXDISP, 0x81))

/* Auxiliary Display Page: Max Blit Size [SV] */
#define HID_USAGE_AUXDISP_MAX_BLIT_SIZE (HID_USAGE(HID_USAGE_AUXDISP, 0x82))

/* Auxiliary Display Page: Bit Depth Format [SV] */
#define HID_USAGE_AUXDISP_BIT_DEPTH_FORMAT (HID_USAGE(HID_USAGE_AUXDISP, 0x83))

/* Auxiliary Display Page: Display Orientation [DV] */
#define HID_USAGE_AUXDISP_DISPLAY_ORIENTATION (HID_USAGE(HID_USAGE_AUXDISP, 0x84))

/* Auxiliary Display Page: Palette Report [CL] */
#define HID_USAGE_AUXDISP_PALETTE_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x85))

/* Auxiliary Display Page: Palette Data Size [SV] */
#define HID_USAGE_AUXDISP_PALETTE_DATA_SIZE (HID_USAGE(HID_USAGE_AUXDISP, 0x86))

/* Auxiliary Display Page: Palette Data Offset [SV] */
#define HID_USAGE_AUXDISP_PALETTE_DATA_OFFSET (HID_USAGE(HID_USAGE_AUXDISP, 0x87))

/* Auxiliary Display Page: Palette Data [Buffered Bytes] */
#define HID_USAGE_AUXDISP_PALETTE_DATA (HID_USAGE(HID_USAGE_AUXDISP, 0x88))

/* Auxiliary Display Page: Blit Report [CL] */
#define HID_USAGE_AUXDISP_BLIT_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x8A))

/* Auxiliary Display Page: Blit Rectangle X1 [SV] */
#define HID_USAGE_AUXDISP_BLIT_RECTANGLE_X1 (HID_USAGE(HID_USAGE_AUXDISP, 0x8B))

/* Auxiliary Display Page: Blit Rectangle Y1 [SV] */
#define HID_USAGE_AUXDISP_BLIT_RECTANGLE_Y1 (HID_USAGE(HID_USAGE_AUXDISP, 0x8C))

/* Auxiliary Display Page: Blit Rectangle X2 [SV] */
#define HID_USAGE_AUXDISP_BLIT_RECTANGLE_X2 (HID_USAGE(HID_USAGE_AUXDISP, 0x8D))

/* Auxiliary Display Page: Blit Rectangle Y2 [SV] */
#define HID_USAGE_AUXDISP_BLIT_RECTANGLE_Y2 (HID_USAGE(HID_USAGE_AUXDISP, 0x8E))

/* Auxiliary Display Page: Blit Data [Buffered Bytes] */
#define HID_USAGE_AUXDISP_BLIT_DATA (HID_USAGE(HID_USAGE_AUXDISP, 0x8F))

/* Auxiliary Display Page: Soft Button [CL] */
#define HID_USAGE_AUXDISP_SOFT_BUTTON (HID_USAGE(HID_USAGE_AUXDISP, 0x90))

/* Auxiliary Display Page: Soft Button ID [SV] */
#define HID_USAGE_AUXDISP_SOFT_BUTTON_ID (HID_USAGE(HID_USAGE_AUXDISP, 0x91))

/* Auxiliary Display Page: Soft Button Side [SV] */
#define HID_USAGE_AUXDISP_SOFT_BUTTON_SIDE (HID_USAGE(HID_USAGE_AUXDISP, 0x92))

/* Auxiliary Display Page: Soft Button Offset 1 [SV] */
#define HID_USAGE_AUXDISP_SOFT_BUTTON_OFFSET_1 (HID_USAGE(HID_USAGE_AUXDISP, 0x93))

/* Auxiliary Display Page: Soft Button Offset 2 [SV] */
#define HID_USAGE_AUXDISP_SOFT_BUTTON_OFFSET_2 (HID_USAGE(HID_USAGE_AUXDISP, 0x94))

/* Auxiliary Display Page: Soft Button Report [SV] */
#define HID_USAGE_AUXDISP_SOFT_BUTTON_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0x95))

/* Auxiliary Display Page: Soft Keys [SV] */
#define HID_USAGE_AUXDISP_SOFT_KEYS (HID_USAGE(HID_USAGE_AUXDISP, 0xC2))

/* Auxiliary Display Page: Display Data Extensions [SF] */
#define HID_USAGE_AUXDISP_DISPLAY_DATA_EXTENSIONS (HID_USAGE(HID_USAGE_AUXDISP, 0xCC))

/* Auxiliary Display Page: Character Mapping [SV] */
#define HID_USAGE_AUXDISP_CHARACTER_MAPPING (HID_USAGE(HID_USAGE_AUXDISP, 0xCF))

/* Auxiliary Display Page: Unicode Equivalent [SV] */
#define HID_USAGE_AUXDISP_UNICODE_EQUIVALENT (HID_USAGE(HID_USAGE_AUXDISP, 0xDD))

/* Auxiliary Display Page: Character Page Mapping [SV] */
#define HID_USAGE_AUXDISP_CHARACTER_PAGE_MAPPING (HID_USAGE(HID_USAGE_AUXDISP, 0xDF))

/* Auxiliary Display Page: Request Report [DV] */
#define HID_USAGE_AUXDISP_REQUEST_REPORT (HID_USAGE(HID_USAGE_AUXDISP, 0xFF))

/* Sensors Page */
#define HID_USAGE_SENSORS (0x20)

/* Sensors Page: Undefined */
#define HID_USAGE_SENSORS_UNDEFINED (HID_USAGE(HID_USAGE_SENSORS, 0x00))

/* Sensors Page: Sensor [CA, CP] */
#define HID_USAGE_SENSORS_SENSOR (HID_USAGE(HID_USAGE_SENSORS, 0x01))

/* Sensors Page: Biometric [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC (HID_USAGE(HID_USAGE_SENSORS, 0x10))

/* Sensors Page: Biometric: Human Presence [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_HUMAN_PRESENCE (HID_USAGE(HID_USAGE_SENSORS, 0x11))

/* Sensors Page: Biometric: Human Proximity [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_HUMAN_PROXIMITY (HID_USAGE(HID_USAGE_SENSORS, 0x12))

/* Sensors Page: Biometric: Human Touch [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_HUMAN_TOUCH (HID_USAGE(HID_USAGE_SENSORS, 0x13))

/* Sensors Page: Biometric: Blood Pressure [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_BLOOD_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x14))

/* Sensors Page: Biometric: Body Temperature [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_BODY_TEMPERATURE (HID_USAGE(HID_USAGE_SENSORS, 0x15))

/* Sensors Page: Biometric: Heart Rate [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_HEART_RATE (HID_USAGE(HID_USAGE_SENSORS, 0x16))

/* Sensors Page: Biometric: Heart Rate Variability [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_HEART_RATE_VARIABILITY (HID_USAGE(HID_USAGE_SENSORS, 0x17))

/* Sensors Page: Biometric: Peripheral Oxygen Saturation [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_PERIPHERAL_OXYGEN_SATURATION                                   \
    (HID_USAGE(HID_USAGE_SENSORS, 0x18))

/* Sensors Page: Biometric: Respiratory Rate [CA, CP] */
#define HID_USAGE_SENSORS_BIOMETRIC_RESPIRATORY_RATE (HID_USAGE(HID_USAGE_SENSORS, 0x19))

/* Sensors Page: Electrical [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL (HID_USAGE(HID_USAGE_SENSORS, 0x20))

/* Sensors Page: Electrical: Capacitance [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_CAPACITANCE (HID_USAGE(HID_USAGE_SENSORS, 0x21))

/* Sensors Page: Electrical: Current [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_CURRENT (HID_USAGE(HID_USAGE_SENSORS, 0x22))

/* Sensors Page: Electrical: Power [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_POWER (HID_USAGE(HID_USAGE_SENSORS, 0x23))

/* Sensors Page: Electrical: Inductance [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_INDUCTANCE (HID_USAGE(HID_USAGE_SENSORS, 0x24))

/* Sensors Page: Electrical: Resistance [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_RESISTANCE (HID_USAGE(HID_USAGE_SENSORS, 0x25))

/* Sensors Page: Electrical: Voltage [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_VOLTAGE (HID_USAGE(HID_USAGE_SENSORS, 0x26))

/* Sensors Page: Electrical: Potentiometer [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_POTENTIOMETER (HID_USAGE(HID_USAGE_SENSORS, 0x27))

/* Sensors Page: Electrical: Frequency [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_FREQUENCY (HID_USAGE(HID_USAGE_SENSORS, 0x28))

/* Sensors Page: Electrical: Period [CA, CP] */
#define HID_USAGE_SENSORS_ELECTRICAL_PERIOD (HID_USAGE(HID_USAGE_SENSORS, 0x29))

/* Sensors Page: Environmental [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL (HID_USAGE(HID_USAGE_SENSORS, 0x30))

/* Sensors Page: Environmental: Atmospheric Pressure [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_ATMOSPHERIC_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x31))

/* Sensors Page: Environmental: Humidity [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_HUMIDITY (HID_USAGE(HID_USAGE_SENSORS, 0x32))

/* Sensors Page: Environmental: Temperature [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_TEMPERATURE (HID_USAGE(HID_USAGE_SENSORS, 0x33))

/* Sensors Page: Environmental: Wind Direction [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_WIND_DIRECTION (HID_USAGE(HID_USAGE_SENSORS, 0x34))

/* Sensors Page: Environmental: Wind Speed [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_WIND_SPEED (HID_USAGE(HID_USAGE_SENSORS, 0x35))

/* Sensors Page: Environmental: Air Quality [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_AIR_QUALITY (HID_USAGE(HID_USAGE_SENSORS, 0x36))

/* Sensors Page: Environmental: Heat Index [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_HEAT_INDEX (HID_USAGE(HID_USAGE_SENSORS, 0x37))

/* Sensors Page: Environmental: Surface Temperature [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_SURFACE_TEMPERATURE (HID_USAGE(HID_USAGE_SENSORS, 0x38))

/* Sensors Page: Environmental: Volatile Organic Compounds [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_VOLATILE_ORGANIC_COMPOUNDS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x39))

/* Sensors Page: Environmental: Object Presence [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_OBJECT_PRESENCE (HID_USAGE(HID_USAGE_SENSORS, 0x3A))

/* Sensors Page: Environmental: Object Proximity [CA, CP] */
#define HID_USAGE_SENSORS_ENVIRONMENTAL_OBJECT_PROXIMITY (HID_USAGE(HID_USAGE_SENSORS, 0x3B))

/* Sensors Page: Light [CA, CP] */
#define HID_USAGE_SENSORS_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x40))

/* Sensors Page: Light: Ambient Light [CA, CP] */
#define HID_USAGE_SENSORS_LIGHT_AMBIENT_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x41))

/* Sensors Page: Light: Consumer Infrared [CA, CP] */
#define HID_USAGE_SENSORS_LIGHT_CONSUMER_INFRARED (HID_USAGE(HID_USAGE_SENSORS, 0x42))

/* Sensors Page: Light: Infrared Light [CA, CP] */
#define HID_USAGE_SENSORS_LIGHT_INFRARED_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x43))

/* Sensors Page: Light: Visible Light [CA, CP] */
#define HID_USAGE_SENSORS_LIGHT_VISIBLE_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x44))

/* Sensors Page: Light: Ultraviolet Light [CA, CP] */
#define HID_USAGE_SENSORS_LIGHT_ULTRAVIOLET_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x45))

/* Sensors Page: Location [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION (HID_USAGE(HID_USAGE_SENSORS, 0x50))

/* Sensors Page: Location: Broadcast [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION_BROADCAST (HID_USAGE(HID_USAGE_SENSORS, 0x51))

/* Sensors Page: Location: Dead Reckoning [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION_DEAD_RECKONING (HID_USAGE(HID_USAGE_SENSORS, 0x52))

/* Sensors Page: Location: GPS (Global Positioning System) [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION_GPS_GLOBAL_POSITIONING_SYSTEM                                   \
    (HID_USAGE(HID_USAGE_SENSORS, 0x53))

/* Sensors Page: Location: Lookup [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION_LOOKUP (HID_USAGE(HID_USAGE_SENSORS, 0x54))

/* Sensors Page: Location: Other [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION_OTHER (HID_USAGE(HID_USAGE_SENSORS, 0x55))

/* Sensors Page: Location: Static [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION_STATIC (HID_USAGE(HID_USAGE_SENSORS, 0x56))

/* Sensors Page: Location: Triangulation [CA, CP] */
#define HID_USAGE_SENSORS_LOCATION_TRIANGULATION (HID_USAGE(HID_USAGE_SENSORS, 0x57))

/* Sensors Page: Mechanical [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL (HID_USAGE(HID_USAGE_SENSORS, 0x60))

/* Sensors Page: Mechanical: Boolean Switch [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_BOOLEAN_SWITCH (HID_USAGE(HID_USAGE_SENSORS, 0x61))

/* Sensors Page: Mechanical: Boolean Switch Array [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_BOOLEAN_SWITCH_ARRAY (HID_USAGE(HID_USAGE_SENSORS, 0x62))

/* Sensors Page: Mechanical: Multivalue Switch [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_MULTIVALUE_SWITCH (HID_USAGE(HID_USAGE_SENSORS, 0x63))

/* Sensors Page: Mechanical: Force [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_FORCE (HID_USAGE(HID_USAGE_SENSORS, 0x64))

/* Sensors Page: Mechanical: Pressure [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x65))

/* Sensors Page: Mechanical: Strain [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_STRAIN (HID_USAGE(HID_USAGE_SENSORS, 0x66))

/* Sensors Page: Mechanical: Weight [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_WEIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x67))

/* Sensors Page: Mechanical: Haptic Vibrator [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_HAPTIC_VIBRATOR (HID_USAGE(HID_USAGE_SENSORS, 0x68))

/* Sensors Page: Mechanical: Hall Effect Switch [CA, CP] */
#define HID_USAGE_SENSORS_MECHANICAL_HALL_EFFECT_SWITCH (HID_USAGE(HID_USAGE_SENSORS, 0x69))

/* Sensors Page: Motion [CA, CP] */
#define HID_USAGE_SENSORS_MOTION (HID_USAGE(HID_USAGE_SENSORS, 0x70))

/* Sensors Page: Motion: Accelerometer 1D [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_ACCELEROMETER_1D (HID_USAGE(HID_USAGE_SENSORS, 0x71))

/* Sensors Page: Motion: Accelerometer 2D [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_ACCELEROMETER_2D (HID_USAGE(HID_USAGE_SENSORS, 0x72))

/* Sensors Page: Motion: Accelerometer 3D [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_ACCELEROMETER_3D (HID_USAGE(HID_USAGE_SENSORS, 0x73))

/* Sensors Page: Motion: Gyrometer 1D [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_GYROMETER_1D (HID_USAGE(HID_USAGE_SENSORS, 0x74))

/* Sensors Page: Motion: Gyrometer 2D [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_GYROMETER_2D (HID_USAGE(HID_USAGE_SENSORS, 0x75))

/* Sensors Page: Motion: Gyrometer 3D [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_GYROMETER_3D (HID_USAGE(HID_USAGE_SENSORS, 0x76))

/* Sensors Page: Motion: Motion Detector [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_MOTION_DETECTOR (HID_USAGE(HID_USAGE_SENSORS, 0x77))

/* Sensors Page: Motion: Speedometer [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_SPEEDOMETER (HID_USAGE(HID_USAGE_SENSORS, 0x78))

/* Sensors Page: Motion: Accelerometer [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_ACCELEROMETER (HID_USAGE(HID_USAGE_SENSORS, 0x79))

/* Sensors Page: Motion: Gyrometer [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_GYROMETER (HID_USAGE(HID_USAGE_SENSORS, 0x7A))

/* Sensors Page: Motion: Gravity Vector [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_GRAVITY_VECTOR (HID_USAGE(HID_USAGE_SENSORS, 0x7B))

/* Sensors Page: Motion: Linear Accelerometer [CA, CP] */
#define HID_USAGE_SENSORS_MOTION_LINEAR_ACCELEROMETER (HID_USAGE(HID_USAGE_SENSORS, 0x7C))

/* Sensors Page: Orientation [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION (HID_USAGE(HID_USAGE_SENSORS, 0x80))

/* Sensors Page: Orientation: Compass 1D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_COMPASS_1D (HID_USAGE(HID_USAGE_SENSORS, 0x81))

/* Sensors Page: Orientation: Compass 2D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_COMPASS_2D (HID_USAGE(HID_USAGE_SENSORS, 0x82))

/* Sensors Page: Orientation: Compass 3D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_COMPASS_3D (HID_USAGE(HID_USAGE_SENSORS, 0x83))

/* Sensors Page: Orientation: Inclinometer 1D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_INCLINOMETER_1D (HID_USAGE(HID_USAGE_SENSORS, 0x84))

/* Sensors Page: Orientation: Inclinometer 2D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_INCLINOMETER_2D (HID_USAGE(HID_USAGE_SENSORS, 0x85))

/* Sensors Page: Orientation: Inclinometer 3D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_INCLINOMETER_3D (HID_USAGE(HID_USAGE_SENSORS, 0x86))

/* Sensors Page: Orientation: Distance 1D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_DISTANCE_1D (HID_USAGE(HID_USAGE_SENSORS, 0x87))

/* Sensors Page: Orientation: Distance 2D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_DISTANCE_2D (HID_USAGE(HID_USAGE_SENSORS, 0x88))

/* Sensors Page: Orientation: Distance 3D [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_DISTANCE_3D (HID_USAGE(HID_USAGE_SENSORS, 0x89))

/* Sensors Page: Orientation: Device Orientation [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_DEVICE_ORIENTATION (HID_USAGE(HID_USAGE_SENSORS, 0x8A))

/* Sensors Page: Orientation: Compass [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_COMPASS (HID_USAGE(HID_USAGE_SENSORS, 0x8B))

/* Sensors Page: Orientation: Inclinometer [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_INCLINOMETER (HID_USAGE(HID_USAGE_SENSORS, 0x8C))

/* Sensors Page: Orientation: Distance [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_DISTANCE (HID_USAGE(HID_USAGE_SENSORS, 0x8D))

/* Sensors Page: Orientation: Relative Orientation [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_RELATIVE_ORIENTATION (HID_USAGE(HID_USAGE_SENSORS, 0x8E))

/* Sensors Page: Orientation: Simple Orientation [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_SIMPLE_ORIENTATION (HID_USAGE(HID_USAGE_SENSORS, 0x8F))

/* Sensors Page: Scanner [CA, CP] */
#define HID_USAGE_SENSORS_SCANNER (HID_USAGE(HID_USAGE_SENSORS, 0x90))

/* Sensors Page: Scanner: Barcode [CA, CP] */
#define HID_USAGE_SENSORS_SCANNER_BARCODE (HID_USAGE(HID_USAGE_SENSORS, 0x91))

/* Sensors Page: Scanner: RFID [CA, CP] */
#define HID_USAGE_SENSORS_SCANNER_RFID (HID_USAGE(HID_USAGE_SENSORS, 0x92))

/* Sensors Page: Scanner: NFC [CA, CP] */
#define HID_USAGE_SENSORS_SCANNER_NFC (HID_USAGE(HID_USAGE_SENSORS, 0x93))

/* Sensors Page: Time [CA, CP] */
#define HID_USAGE_SENSORS_TIME (HID_USAGE(HID_USAGE_SENSORS, 0xA0))

/* Sensors Page: Time: Alarm Timer [CA, CP] */
#define HID_USAGE_SENSORS_TIME_ALARM_TIMER (HID_USAGE(HID_USAGE_SENSORS, 0xA1))

/* Sensors Page: Time: Real Time Clock [CA, CP] */
#define HID_USAGE_SENSORS_TIME_REAL_TIME_CLOCK (HID_USAGE(HID_USAGE_SENSORS, 0xA2))

/* Sensors Page: Personal Activity [CA, CP] */
#define HID_USAGE_SENSORS_PERSONAL_ACTIVITY (HID_USAGE(HID_USAGE_SENSORS, 0xB0))

/* Sensors Page: Personal Activity: Activity Detection [CA, CP] */
#define HID_USAGE_SENSORS_PERSONAL_ACTIVITY_ACTIVITY_DETECTION (HID_USAGE(HID_USAGE_SENSORS, 0xB1))

/* Sensors Page: Personal Activity: Device Position [CA, CP] */
#define HID_USAGE_SENSORS_PERSONAL_ACTIVITY_DEVICE_POSITION (HID_USAGE(HID_USAGE_SENSORS, 0xB2))

/* Sensors Page: Personal Activity: Pedometer [CA, CP] */
#define HID_USAGE_SENSORS_PERSONAL_ACTIVITY_PEDOMETER (HID_USAGE(HID_USAGE_SENSORS, 0xB3))

/* Sensors Page: Personal Activity: Step Detection [CA, CP] */
#define HID_USAGE_SENSORS_PERSONAL_ACTIVITY_STEP_DETECTION (HID_USAGE(HID_USAGE_SENSORS, 0xB4))

/* Sensors Page: Orientation Extended [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_EXTENDED (HID_USAGE(HID_USAGE_SENSORS, 0xC0))

/* Sensors Page: Orientation Extended: Geomagnetic Orientation [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_EXTENDED_GEOMAGNETIC_ORIENTATION                             \
    (HID_USAGE(HID_USAGE_SENSORS, 0xC1))

/* Sensors Page: Orientation Extended: Magnetometer [CA, CP] */
#define HID_USAGE_SENSORS_ORIENTATION_EXTENDED_MAGNETOMETER (HID_USAGE(HID_USAGE_SENSORS, 0xC2))

/* Sensors Page: Gesture [CA, CP] */
#define HID_USAGE_SENSORS_GESTURE (HID_USAGE(HID_USAGE_SENSORS, 0xD0))

/* Sensors Page: Gesture: Chassis Flip Gesture [CA, CP] */
#define HID_USAGE_SENSORS_GESTURE_CHASSIS_FLIP_GESTURE (HID_USAGE(HID_USAGE_SENSORS, 0xD1))

/* Sensors Page: Gesture: Hinge Fold Gesture [CA, CP] */
#define HID_USAGE_SENSORS_GESTURE_HINGE_FOLD_GESTURE (HID_USAGE(HID_USAGE_SENSORS, 0xD2))

/* Sensors Page: Other [CA, CP] */
#define HID_USAGE_SENSORS_OTHER (HID_USAGE(HID_USAGE_SENSORS, 0xE0))

/* Sensors Page: Other: Custom [CA, CP] */
#define HID_USAGE_SENSORS_OTHER_CUSTOM (HID_USAGE(HID_USAGE_SENSORS, 0xE1))

/* Sensors Page: Other: Generic [CA, CP] */
#define HID_USAGE_SENSORS_OTHER_GENERIC (HID_USAGE(HID_USAGE_SENSORS, 0xE2))

/* Sensors Page: Other: Generic Enumerator [CA, CP] */
#define HID_USAGE_SENSORS_OTHER_GENERIC_ENUMERATOR (HID_USAGE(HID_USAGE_SENSORS, 0xE3))

/* Sensors Page: Other: Hinge Angle [CA, CP] */
#define HID_USAGE_SENSORS_OTHER_HINGE_ANGLE (HID_USAGE(HID_USAGE_SENSORS, 0xE4))

/* Sensors Page: Event [DV] */
#define HID_USAGE_SENSORS_EVENT (HID_USAGE(HID_USAGE_SENSORS, 0x200))

/* Sensors Page: Event: Sensor State [NAry] */
#define HID_USAGE_SENSORS_EVENT_SENSOR_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x201))

/* Sensors Page: Event: Sensor Event [NAry] */
#define HID_USAGE_SENSORS_EVENT_SENSOR_EVENT (HID_USAGE(HID_USAGE_SENSORS, 0x202))

/* Sensors Page: Property [DV] */
#define HID_USAGE_SENSORS_PROPERTY (HID_USAGE(HID_USAGE_SENSORS, 0x300))

/* Sensors Page: Property: Friendly Name [SV] */
#define HID_USAGE_SENSORS_PROPERTY_FRIENDLY_NAME (HID_USAGE(HID_USAGE_SENSORS, 0x301))

/* Sensors Page: Property: Persistent Unique ID [DV] */
#define HID_USAGE_SENSORS_PROPERTY_PERSISTENT_UNIQUE_ID (HID_USAGE(HID_USAGE_SENSORS, 0x302))

/* Sensors Page: Property: Sensor Status [DV] */
#define HID_USAGE_SENSORS_PROPERTY_SENSOR_STATUS (HID_USAGE(HID_USAGE_SENSORS, 0x303))

/* Sensors Page: Property: Minimum Report Interval [SV] */
#define HID_USAGE_SENSORS_PROPERTY_MINIMUM_REPORT_INTERVAL (HID_USAGE(HID_USAGE_SENSORS, 0x304))

/* Sensors Page: Property: Sensor Manufacturer [SV] */
#define HID_USAGE_SENSORS_PROPERTY_SENSOR_MANUFACTURER (HID_USAGE(HID_USAGE_SENSORS, 0x305))

/* Sensors Page: Property: Sensor Model [SV] */
#define HID_USAGE_SENSORS_PROPERTY_SENSOR_MODEL (HID_USAGE(HID_USAGE_SENSORS, 0x306))

/* Sensors Page: Property: Sensor Serial Number [SV] */
#define HID_USAGE_SENSORS_PROPERTY_SENSOR_SERIAL_NUMBER (HID_USAGE(HID_USAGE_SENSORS, 0x307))

/* Sensors Page: Property: Sensor Description [SV] */
#define HID_USAGE_SENSORS_PROPERTY_SENSOR_DESCRIPTION (HID_USAGE(HID_USAGE_SENSORS, 0x308))

/* Sensors Page: Property: Sensor Connection Type [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_SENSOR_CONNECTION_TYPE (HID_USAGE(HID_USAGE_SENSORS, 0x309))

/* Sensors Page: Property: Sensor Device Path [DV] */
#define HID_USAGE_SENSORS_PROPERTY_SENSOR_DEVICE_PATH (HID_USAGE(HID_USAGE_SENSORS, 0x30A))

/* Sensors Page: Property: Hardware Revision [SV] */
#define HID_USAGE_SENSORS_PROPERTY_HARDWARE_REVISION (HID_USAGE(HID_USAGE_SENSORS, 0x30B))

/* Sensors Page: Property: Firmware Version [SV] */
#define HID_USAGE_SENSORS_PROPERTY_FIRMWARE_VERSION (HID_USAGE(HID_USAGE_SENSORS, 0x30C))

/* Sensors Page: Property: Release Date [SV] */
#define HID_USAGE_SENSORS_PROPERTY_RELEASE_DATE (HID_USAGE(HID_USAGE_SENSORS, 0x30D))

/* Sensors Page: Property: Report Interval [DV] */
#define HID_USAGE_SENSORS_PROPERTY_REPORT_INTERVAL (HID_USAGE(HID_USAGE_SENSORS, 0x30E))

/* Sensors Page: Property: Change Sensitivity Absolute [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CHANGE_SENSITIVITY_ABSOLUTE (HID_USAGE(HID_USAGE_SENSORS, 0x30F))

/* Sensors Page: Property: Change Sensitivity Percent of Range [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CHANGE_SENSITIVITY_PERCENT_OF_RANGE                             \
    (HID_USAGE(HID_USAGE_SENSORS, 0x310))

/* Sensors Page: Property: Change Sensitivity Percent Relative [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CHANGE_SENSITIVITY_PERCENT_RELATIVE                             \
    (HID_USAGE(HID_USAGE_SENSORS, 0x311))

/* Sensors Page: Property: Accuracy [DV] */
#define HID_USAGE_SENSORS_PROPERTY_ACCURACY (HID_USAGE(HID_USAGE_SENSORS, 0x312))

/* Sensors Page: Property: Resolution [DV] */
#define HID_USAGE_SENSORS_PROPERTY_RESOLUTION (HID_USAGE(HID_USAGE_SENSORS, 0x313))

/* Sensors Page: Property: Maximum [DV] */
#define HID_USAGE_SENSORS_PROPERTY_MAXIMUM (HID_USAGE(HID_USAGE_SENSORS, 0x314))

/* Sensors Page: Property: Minimum [DV] */
#define HID_USAGE_SENSORS_PROPERTY_MINIMUM (HID_USAGE(HID_USAGE_SENSORS, 0x315))

/* Sensors Page: Property: Reporting State [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_REPORTING_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x316))

/* Sensors Page: Property: Sampling Rate [DV] */
#define HID_USAGE_SENSORS_PROPERTY_SAMPLING_RATE (HID_USAGE(HID_USAGE_SENSORS, 0x317))

/* Sensors Page: Property: Response Curve [DV] */
#define HID_USAGE_SENSORS_PROPERTY_RESPONSE_CURVE (HID_USAGE(HID_USAGE_SENSORS, 0x318))

/* Sensors Page: Property: Power State [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_POWER_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x319))

/* Sensors Page: Property: Maximum FIFO Events [SV] */
#define HID_USAGE_SENSORS_PROPERTY_MAXIMUM_FIFO_EVENTS (HID_USAGE(HID_USAGE_SENSORS, 0x31A))

/* Sensors Page: Property: Report Latency [DV] */
#define HID_USAGE_SENSORS_PROPERTY_REPORT_LATENCY (HID_USAGE(HID_USAGE_SENSORS, 0x31B))

/* Sensors Page: Property: Flush FIFO Events [DF] */
#define HID_USAGE_SENSORS_PROPERTY_FLUSH_FIFO_EVENTS (HID_USAGE(HID_USAGE_SENSORS, 0x31C))

/* Sensors Page: Property: Maximum Power Consumption [DV] */
#define HID_USAGE_SENSORS_PROPERTY_MAXIMUM_POWER_CONSUMPTION (HID_USAGE(HID_USAGE_SENSORS, 0x31D))

/* Sensors Page: Property: Is Primary [DF] */
#define HID_USAGE_SENSORS_PROPERTY_IS_PRIMARY (HID_USAGE(HID_USAGE_SENSORS, 0x31E))

/* Sensors Page: Data Field: Location [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_LOCATION (HID_USAGE(HID_USAGE_SENSORS, 0x400))

/* Sensors Page: Data Field: Altitude Antenna Sea Level [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ALTITUDE_ANTENNA_SEA_LEVEL                                    \
    (HID_USAGE(HID_USAGE_SENSORS, 0x402))

/* Sensors Page: Data Field: Differential Reference Station ID [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_DIFFERENTIAL_REFERENCE_STATION_ID                             \
    (HID_USAGE(HID_USAGE_SENSORS, 0x403))

/* Sensors Page: Data Field: Altitude Ellipsoid Error [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ALTITUDE_ELLIPSOID_ERROR (HID_USAGE(HID_USAGE_SENSORS, 0x404))

/* Sensors Page: Data Field: Altitude Ellipsoid [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ALTITUDE_ELLIPSOID (HID_USAGE(HID_USAGE_SENSORS, 0x405))

/* Sensors Page: Data Field: Altitude Sea Level Error [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ALTITUDE_SEA_LEVEL_ERROR (HID_USAGE(HID_USAGE_SENSORS, 0x406))

/* Sensors Page: Data Field: Altitude Sea Level [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ALTITUDE_SEA_LEVEL (HID_USAGE(HID_USAGE_SENSORS, 0x407))

/* Sensors Page: Data Field: Differential GPS Data Age [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_DIFFERENTIAL_GPS_DATA_AGE (HID_USAGE(HID_USAGE_SENSORS, 0x408))

/* Sensors Page: Data Field: Error Radius [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ERROR_RADIUS (HID_USAGE(HID_USAGE_SENSORS, 0x409))

/* Sensors Page: Data Field: Fix Quality [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_FIX_QUALITY (HID_USAGE(HID_USAGE_SENSORS, 0x40A))

/* Sensors Page: Data Field: Fix Type [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_FIX_TYPE (HID_USAGE(HID_USAGE_SENSORS, 0x40B))

/* Sensors Page: Data Field: Geoidal Separation [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GEOIDAL_SEPARATION (HID_USAGE(HID_USAGE_SENSORS, 0x40C))

/* Sensors Page: Data Field: GPS Operation Mode [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GPS_OPERATION_MODE (HID_USAGE(HID_USAGE_SENSORS, 0x40D))

/* Sensors Page: Data Field: GPS Selection Mode [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GPS_SELECTION_MODE (HID_USAGE(HID_USAGE_SENSORS, 0x40E))

/* Sensors Page: Data Field: GPS Status [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GPS_STATUS (HID_USAGE(HID_USAGE_SENSORS, 0x40F))

/* Sensors Page: Data Field: Position Dilution of Precision [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_POSITION_DILUTION_OF_PRECISION                                \
    (HID_USAGE(HID_USAGE_SENSORS, 0x410))

/* Sensors Page: Data Field: Horizontal Dilution of Precision [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HORIZONTAL_DILUTION_OF_PRECISION                              \
    (HID_USAGE(HID_USAGE_SENSORS, 0x411))

/* Sensors Page: Data Field: Vertical Dilution of Precision [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_VERTICAL_DILUTION_OF_PRECISION                                \
    (HID_USAGE(HID_USAGE_SENSORS, 0x412))

/* Sensors Page: Data Field: Latitude [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_LATITUDE (HID_USAGE(HID_USAGE_SENSORS, 0x413))

/* Sensors Page: Data Field: Longitude [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_LONGITUDE (HID_USAGE(HID_USAGE_SENSORS, 0x414))

/* Sensors Page: Data Field: True Heading [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TRUE_HEADING (HID_USAGE(HID_USAGE_SENSORS, 0x415))

/* Sensors Page: Data Field: Magnetic Heading [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MAGNETIC_HEADING (HID_USAGE(HID_USAGE_SENSORS, 0x416))

/* Sensors Page: Data Field: Magnetic Variation [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MAGNETIC_VARIATION (HID_USAGE(HID_USAGE_SENSORS, 0x417))

/* Sensors Page: Data Field: Speed [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SPEED (HID_USAGE(HID_USAGE_SENSORS, 0x418))

/* Sensors Page: Data Field: Satellites in View [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_IN_VIEW (HID_USAGE(HID_USAGE_SENSORS, 0x419))

/* Sensors Page: Data Field: Satellites in View Azimuth [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_IN_VIEW_AZIMUTH                                    \
    (HID_USAGE(HID_USAGE_SENSORS, 0x41A))

/* Sensors Page: Data Field: Satellites in View Elevation [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_IN_VIEW_ELEVATION                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x41B))

/* Sensors Page: Data Field: Satellites in View IDs [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_IN_VIEW_IDS (HID_USAGE(HID_USAGE_SENSORS, 0x41C))

/* Sensors Page: Data Field: Satellites in View PRNs [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_IN_VIEW_PRNS (HID_USAGE(HID_USAGE_SENSORS, 0x41D))

/* Sensors Page: Data Field: Satellites in View S/N Ratios [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_IN_VIEW_S_N_RATIOS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x41E))

/* Sensors Page: Data Field: Satellites Used Count [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_USED_COUNT (HID_USAGE(HID_USAGE_SENSORS, 0x41F))

/* Sensors Page: Data Field: Satellites Used PRNs [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SATELLITES_USED_PRNS (HID_USAGE(HID_USAGE_SENSORS, 0x420))

/* Sensors Page: Data Field: NMEA Sentence [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_NMEA_SENTENCE (HID_USAGE(HID_USAGE_SENSORS, 0x421))

/* Sensors Page: Data Field: Address Line 1 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ADDRESS_LINE_1 (HID_USAGE(HID_USAGE_SENSORS, 0x422))

/* Sensors Page: Data Field: Address Line 2 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ADDRESS_LINE_2 (HID_USAGE(HID_USAGE_SENSORS, 0x423))

/* Sensors Page: Data Field: City [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CITY (HID_USAGE(HID_USAGE_SENSORS, 0x424))

/* Sensors Page: Data Field: State or Province [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_STATE_OR_PROVINCE (HID_USAGE(HID_USAGE_SENSORS, 0x425))

/* Sensors Page: Data Field: Country or Region [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_COUNTRY_OR_REGION (HID_USAGE(HID_USAGE_SENSORS, 0x426))

/* Sensors Page: Data Field: Postal Code [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_POSTAL_CODE (HID_USAGE(HID_USAGE_SENSORS, 0x427))

/* Sensors Page: Property: Location [DV] */
#define HID_USAGE_SENSORS_PROPERTY_LOCATION (HID_USAGE(HID_USAGE_SENSORS, 0x42A))

/* Sensors Page: Property: Location Desired Accuracy [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_LOCATION_DESIRED_ACCURACY (HID_USAGE(HID_USAGE_SENSORS, 0x42B))

/* Sensors Page: Data Field: Environmental [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ENVIRONMENTAL (HID_USAGE(HID_USAGE_SENSORS, 0x430))

/* Sensors Page: Data Field: Atmospheric Pressure [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ATMOSPHERIC_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x431))

/* Sensors Page: Data Field: Relative Humidity [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_RELATIVE_HUMIDITY (HID_USAGE(HID_USAGE_SENSORS, 0x433))

/* Sensors Page: Data Field: Temperature [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TEMPERATURE (HID_USAGE(HID_USAGE_SENSORS, 0x434))

/* Sensors Page: Data Field: Wind Direction [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_WIND_DIRECTION (HID_USAGE(HID_USAGE_SENSORS, 0x435))

/* Sensors Page: Data Field: Wind Speed [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_WIND_SPEED (HID_USAGE(HID_USAGE_SENSORS, 0x436))

/* Sensors Page: Data Field: Air Quality Index [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_AIR_QUALITY_INDEX (HID_USAGE(HID_USAGE_SENSORS, 0x437))

/* Sensors Page: Data Field: Equivalent CO2 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_EQUIVALENT_CO2 (HID_USAGE(HID_USAGE_SENSORS, 0x438))

/* Sensors Page: Data Field: Volatile Organic Compound Concentration [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_VOLATILE_ORGANIC_COMPOUND_CONCENTRATION                       \
    (HID_USAGE(HID_USAGE_SENSORS, 0x439))

/* Sensors Page: Data Field: Object Presence [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_OBJECT_PRESENCE (HID_USAGE(HID_USAGE_SENSORS, 0x43A))

/* Sensors Page: Data Field: Object Proximity Range [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_OBJECT_PROXIMITY_RANGE (HID_USAGE(HID_USAGE_SENSORS, 0x43B))

/* Sensors Page: Data Field: Object Proximity Out of Range [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_OBJECT_PROXIMITY_OUT_OF_RANGE                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x43C))

/* Sensors Page: Property: Environmental [SV] */
#define HID_USAGE_SENSORS_PROPERTY_ENVIRONMENTAL (HID_USAGE(HID_USAGE_SENSORS, 0x440))

/* Sensors Page: Property: Reference Pressure [SV] */
#define HID_USAGE_SENSORS_PROPERTY_REFERENCE_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x441))

/* Sensors Page: Data Field: Motion [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MOTION (HID_USAGE(HID_USAGE_SENSORS, 0x450))

/* Sensors Page: Data Field: Motion State [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_MOTION_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x451))

/* Sensors Page: Data Field: Acceleration [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ACCELERATION (HID_USAGE(HID_USAGE_SENSORS, 0x452))

/* Sensors Page: Data Field: Acceleration Axis X [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ACCELERATION_AXIS_X (HID_USAGE(HID_USAGE_SENSORS, 0x453))

/* Sensors Page: Data Field: Acceleration Axis Y [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ACCELERATION_AXIS_Y (HID_USAGE(HID_USAGE_SENSORS, 0x454))

/* Sensors Page: Data Field: Acceleration Axis Z [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ACCELERATION_AXIS_Z (HID_USAGE(HID_USAGE_SENSORS, 0x455))

/* Sensors Page: Data Field: Angular Velocity [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_VELOCITY (HID_USAGE(HID_USAGE_SENSORS, 0x456))

/* Sensors Page: Data Field: Angular Velocity about X Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_VELOCITY_ABOUT_X_AXIS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x457))

/* Sensors Page: Data Field: Angular Velocity about Y Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_VELOCITY_ABOUT_Y_AXIS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x458))

/* Sensors Page: Data Field: Angular Velocity about Z Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_VELOCITY_ABOUT_Z_AXIS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x459))

/* Sensors Page: Data Field: Angular Position [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_POSITION (HID_USAGE(HID_USAGE_SENSORS, 0x45A))

/* Sensors Page: Data Field: Angular Position about X Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_POSITION_ABOUT_X_AXIS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x45B))

/* Sensors Page: Data Field: Angular Position about Y Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_POSITION_ABOUT_Y_AXIS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x45C))

/* Sensors Page: Data Field: Angular Position about Z Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ANGULAR_POSITION_ABOUT_Z_AXIS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x45D))

/* Sensors Page: Data Field: Motion Speed [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MOTION_SPEED (HID_USAGE(HID_USAGE_SENSORS, 0x45E))

/* Sensors Page: Data Field: Motion Intensity [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MOTION_INTENSITY (HID_USAGE(HID_USAGE_SENSORS, 0x45F))

/* Sensors Page: Data Field: Orientation [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ORIENTATION (HID_USAGE(HID_USAGE_SENSORS, 0x470))

/* Sensors Page: Data Field: Heading [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING (HID_USAGE(HID_USAGE_SENSORS, 0x471))

/* Sensors Page: Data Field: Heading X Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING_X_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x472))

/* Sensors Page: Data Field: Heading Y Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING_Y_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x473))

/* Sensors Page: Data Field: Heading Z Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING_Z_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x474))

/* Sensors Page: Data Field: Heading Compensated Magnetic North [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING_COMPENSATED_MAGNETIC_NORTH                            \
    (HID_USAGE(HID_USAGE_SENSORS, 0x475))

/* Sensors Page: Data Field: Heading Compensated True North [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING_COMPENSATED_TRUE_NORTH                                \
    (HID_USAGE(HID_USAGE_SENSORS, 0x476))

/* Sensors Page: Data Field: Heading Magnetic North [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING_MAGNETIC_NORTH (HID_USAGE(HID_USAGE_SENSORS, 0x477))

/* Sensors Page: Data Field: Heading True North [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEADING_TRUE_NORTH (HID_USAGE(HID_USAGE_SENSORS, 0x478))

/* Sensors Page: Data Field: Distance [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_DISTANCE (HID_USAGE(HID_USAGE_SENSORS, 0x479))

/* Sensors Page: Data Field: Distance X Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_DISTANCE_X_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x47A))

/* Sensors Page: Data Field: Distance Y Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_DISTANCE_Y_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x47B))

/* Sensors Page: Data Field: Distance Z Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_DISTANCE_Z_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x47C))

/* Sensors Page: Data Field: Distance Out-of-Range [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_DISTANCE_OUT_OF_RANGE (HID_USAGE(HID_USAGE_SENSORS, 0x47D))

/* Sensors Page: Data Field: Tilt [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TILT (HID_USAGE(HID_USAGE_SENSORS, 0x47E))

/* Sensors Page: Data Field: Tilt X Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TILT_X_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x47F))

/* Sensors Page: Data Field: Tilt Y Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TILT_Y_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x480))

/* Sensors Page: Data Field: Tilt Z Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TILT_Z_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x481))

/* Sensors Page: Data Field: Rotation Matrix [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ROTATION_MATRIX (HID_USAGE(HID_USAGE_SENSORS, 0x482))

/* Sensors Page: Data Field: Quaternion [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_QUATERNION (HID_USAGE(HID_USAGE_SENSORS, 0x483))

/* Sensors Page: Data Field: Magnetic Flux [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MAGNETIC_FLUX (HID_USAGE(HID_USAGE_SENSORS, 0x484))

/* Sensors Page: Data Field: Magnetic Flux X Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MAGNETIC_FLUX_X_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x485))

/* Sensors Page: Data Field: Magnetic Flux Y Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MAGNETIC_FLUX_Y_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x486))

/* Sensors Page: Data Field: Magnetic Flux Z Axis [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MAGNETIC_FLUX_Z_AXIS (HID_USAGE(HID_USAGE_SENSORS, 0x487))

/* Sensors Page: Data Field: Magnetometer Accuracy [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_MAGNETOMETER_ACCURACY (HID_USAGE(HID_USAGE_SENSORS, 0x488))

/* Sensors Page: Data Field: Simple Orientation Direction [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_SIMPLE_ORIENTATION_DIRECTION                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x489))

/* Sensors Page: Data Field: Mechanical [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MECHANICAL (HID_USAGE(HID_USAGE_SENSORS, 0x490))

/* Sensors Page: Data Field: Boolean Switch State [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_BOOLEAN_SWITCH_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x491))

/* Sensors Page: Data Field: Boolean Switch Array States [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_BOOLEAN_SWITCH_ARRAY_STATES                                   \
    (HID_USAGE(HID_USAGE_SENSORS, 0x492))

/* Sensors Page: Data Field: Multivalue Switch Value [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MULTIVALUE_SWITCH_VALUE (HID_USAGE(HID_USAGE_SENSORS, 0x493))

/* Sensors Page: Data Field: Force [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_FORCE (HID_USAGE(HID_USAGE_SENSORS, 0x494))

/* Sensors Page: Data Field: Absolute Pressure [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ABSOLUTE_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x495))

/* Sensors Page: Data Field: Gauge Pressure [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GAUGE_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x496))

/* Sensors Page: Data Field: Strain [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_STRAIN (HID_USAGE(HID_USAGE_SENSORS, 0x497))

/* Sensors Page: Data Field: Weight [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_WEIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x498))

/* Sensors Page: Property: Mechanical [DV] */
#define HID_USAGE_SENSORS_PROPERTY_MECHANICAL (HID_USAGE(HID_USAGE_SENSORS, 0x4A0))

/* Sensors Page: Property: Vibration State [DF] */
#define HID_USAGE_SENSORS_PROPERTY_VIBRATION_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x4A1))

/* Sensors Page: Property: Forward Vibration Speed [DV] */
#define HID_USAGE_SENSORS_PROPERTY_FORWARD_VIBRATION_SPEED (HID_USAGE(HID_USAGE_SENSORS, 0x4A2))

/* Sensors Page: Property: Backward Vibration Speed [DV] */
#define HID_USAGE_SENSORS_PROPERTY_BACKWARD_VIBRATION_SPEED (HID_USAGE(HID_USAGE_SENSORS, 0x4A3))

/* Sensors Page: Data Field: Biometric [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_BIOMETRIC (HID_USAGE(HID_USAGE_SENSORS, 0x4B0))

/* Sensors Page: Data Field: Human Presence [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_HUMAN_PRESENCE (HID_USAGE(HID_USAGE_SENSORS, 0x4B1))

/* Sensors Page: Data Field: Human Proximity Range [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HUMAN_PROXIMITY_RANGE (HID_USAGE(HID_USAGE_SENSORS, 0x4B2))

/* Sensors Page: Data Field: Human Proximity Out of Range [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_HUMAN_PROXIMITY_OUT_OF_RANGE                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x4B3))

/* Sensors Page: Data Field: Human Touch State [SF] */
#define HID_USAGE_SENSORS_DATA_FIELD_HUMAN_TOUCH_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x4B4))

/* Sensors Page: Data Field: Blood Pressure [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_BLOOD_PRESSURE (HID_USAGE(HID_USAGE_SENSORS, 0x4B5))

/* Sensors Page: Data Field: Blood Pressure Diastolic [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_BLOOD_PRESSURE_DIASTOLIC (HID_USAGE(HID_USAGE_SENSORS, 0x4B6))

/* Sensors Page: Data Field: Blood Pressure Systolic [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_BLOOD_PRESSURE_SYSTOLIC (HID_USAGE(HID_USAGE_SENSORS, 0x4B7))

/* Sensors Page: Data Field: Heart Rate [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEART_RATE (HID_USAGE(HID_USAGE_SENSORS, 0x4B8))

/* Sensors Page: Data Field: Resting Heart Rate [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_RESTING_HEART_RATE (HID_USAGE(HID_USAGE_SENSORS, 0x4B9))

/* Sensors Page: Data Field: Heartbeat Interval [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HEARTBEAT_INTERVAL (HID_USAGE(HID_USAGE_SENSORS, 0x4BA))

/* Sensors Page: Data Field: Respiratory Rate [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_RESPIRATORY_RATE (HID_USAGE(HID_USAGE_SENSORS, 0x4BB))

/* Sensors Page: Data Field: SpO2 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SPO2 (HID_USAGE(HID_USAGE_SENSORS, 0x4BC))

/* Sensors Page: Data Field: Light [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4D0))

/* Sensors Page: Data Field: Illuminance [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ILLUMINANCE (HID_USAGE(HID_USAGE_SENSORS, 0x4D1))

/* Sensors Page: Data Field: Color Temperature [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_COLOR_TEMPERATURE (HID_USAGE(HID_USAGE_SENSORS, 0x4D2))

/* Sensors Page: Data Field: Chromaticity [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CHROMATICITY (HID_USAGE(HID_USAGE_SENSORS, 0x4D3))

/* Sensors Page: Data Field: Chromaticity X [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CHROMATICITY_X (HID_USAGE(HID_USAGE_SENSORS, 0x4D4))

/* Sensors Page: Data Field: Chromaticity Y [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CHROMATICITY_Y (HID_USAGE(HID_USAGE_SENSORS, 0x4D5))

/* Sensors Page: Data Field: Consumer IR Sentence Receive [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CONSUMER_IR_SENTENCE_RECEIVE                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x4D6))

/* Sensors Page: Data Field: Infrared Light [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_INFRARED_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4D7))

/* Sensors Page: Data Field: Red Light [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_RED_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4D8))

/* Sensors Page: Data Field: Green Light [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GREEN_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4D9))

/* Sensors Page: Data Field: Blue Light [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_BLUE_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4DA))

/* Sensors Page: Data Field: Ultraviolet A Light [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ULTRAVIOLET_A_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4DB))

/* Sensors Page: Data Field: Ultraviolet B Light [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ULTRAVIOLET_B_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4DC))

/* Sensors Page: Data Field: Ultraviolet Index [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ULTRAVIOLET_INDEX (HID_USAGE(HID_USAGE_SENSORS, 0x4DD))

/* Sensors Page: Data Field: Near Infrared Light [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_NEAR_INFRARED_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4DE))

/* Sensors Page: Property: Light [DV] */
#define HID_USAGE_SENSORS_PROPERTY_LIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x4DF))

/* Sensors Page: Property: Consumer IR Sentence Send [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CONSUMER_IR_SENTENCE_SEND (HID_USAGE(HID_USAGE_SENSORS, 0x4E0))

/* Sensors Page: Property: Auto Brightness Preferred [DF] */
#define HID_USAGE_SENSORS_PROPERTY_AUTO_BRIGHTNESS_PREFERRED (HID_USAGE(HID_USAGE_SENSORS, 0x4E2))

/* Sensors Page: Property: Auto Color Preferred [DF] */
#define HID_USAGE_SENSORS_PROPERTY_AUTO_COLOR_PREFERRED (HID_USAGE(HID_USAGE_SENSORS, 0x4E3))

/* Sensors Page: Data Field: Scanner [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SCANNER (HID_USAGE(HID_USAGE_SENSORS, 0x4F0))

/* Sensors Page: Data Field: RFID Tag 40 Bit [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_RFID_TAG_40_BIT (HID_USAGE(HID_USAGE_SENSORS, 0x4F1))

/* Sensors Page: Data Field: NFC Sentence Receive [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_NFC_SENTENCE_RECEIVE (HID_USAGE(HID_USAGE_SENSORS, 0x4F2))

/* Sensors Page: Property: Scanner [DV] */
#define HID_USAGE_SENSORS_PROPERTY_SCANNER (HID_USAGE(HID_USAGE_SENSORS, 0x4F8))

/* Sensors Page: Property: NFC Sentence Send [SV] */
#define HID_USAGE_SENSORS_PROPERTY_NFC_SENTENCE_SEND (HID_USAGE(HID_USAGE_SENSORS, 0x4F9))

/* Sensors Page: Data Field: Electrical [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ELECTRICAL (HID_USAGE(HID_USAGE_SENSORS, 0x500))

/* Sensors Page: Data Field: Capacitance [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CAPACITANCE (HID_USAGE(HID_USAGE_SENSORS, 0x501))

/* Sensors Page: Data Field: Current [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CURRENT (HID_USAGE(HID_USAGE_SENSORS, 0x502))

/* Sensors Page: Data Field: Electrical Power [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ELECTRICAL_POWER (HID_USAGE(HID_USAGE_SENSORS, 0x503))

/* Sensors Page: Data Field: Inductance [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_INDUCTANCE (HID_USAGE(HID_USAGE_SENSORS, 0x504))

/* Sensors Page: Data Field: Resistance [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_RESISTANCE (HID_USAGE(HID_USAGE_SENSORS, 0x505))

/* Sensors Page: Data Field: Voltage [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_VOLTAGE (HID_USAGE(HID_USAGE_SENSORS, 0x506))

/* Sensors Page: Data Field: Frequency [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_FREQUENCY (HID_USAGE(HID_USAGE_SENSORS, 0x507))

/* Sensors Page: Data Field: Period [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_PERIOD (HID_USAGE(HID_USAGE_SENSORS, 0x508))

/* Sensors Page: Data Field: Percent of Range [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_PERCENT_OF_RANGE (HID_USAGE(HID_USAGE_SENSORS, 0x509))

/* Sensors Page: Data Field: Time [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TIME (HID_USAGE(HID_USAGE_SENSORS, 0x520))

/* Sensors Page: Data Field: Year [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_YEAR (HID_USAGE(HID_USAGE_SENSORS, 0x521))

/* Sensors Page: Data Field: Month [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MONTH (HID_USAGE(HID_USAGE_SENSORS, 0x522))

/* Sensors Page: Data Field: Day [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_DAY (HID_USAGE(HID_USAGE_SENSORS, 0x523))

/* Sensors Page: Data Field: Day of Week [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_DAY_OF_WEEK (HID_USAGE(HID_USAGE_SENSORS, 0x524))

/* Sensors Page: Data Field: Hour [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HOUR (HID_USAGE(HID_USAGE_SENSORS, 0x525))

/* Sensors Page: Data Field: Minute [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MINUTE (HID_USAGE(HID_USAGE_SENSORS, 0x526))

/* Sensors Page: Data Field: Second [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_SECOND (HID_USAGE(HID_USAGE_SENSORS, 0x527))

/* Sensors Page: Data Field: Millisecond [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_MILLISECOND (HID_USAGE(HID_USAGE_SENSORS, 0x528))

/* Sensors Page: Data Field: Timestamp [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TIMESTAMP (HID_USAGE(HID_USAGE_SENSORS, 0x529))

/* Sensors Page: Data Field: Julian Day of Year [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_JULIAN_DAY_OF_YEAR (HID_USAGE(HID_USAGE_SENSORS, 0x52A))

/* Sensors Page: Data Field: Time Since System Boot [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_TIME_SINCE_SYSTEM_BOOT (HID_USAGE(HID_USAGE_SENSORS, 0x52B))

/* Sensors Page: Property: Time [DV] */
#define HID_USAGE_SENSORS_PROPERTY_TIME (HID_USAGE(HID_USAGE_SENSORS, 0x530))

/* Sensors Page: Property: Time Zone Offset from UTC [DV] */
#define HID_USAGE_SENSORS_PROPERTY_TIME_ZONE_OFFSET_FROM_UTC (HID_USAGE(HID_USAGE_SENSORS, 0x531))

/* Sensors Page: Property: Time Zone Name [DV] */
#define HID_USAGE_SENSORS_PROPERTY_TIME_ZONE_NAME (HID_USAGE(HID_USAGE_SENSORS, 0x532))

/* Sensors Page: Property: Daylight Savings Time Observed [DF] */
#define HID_USAGE_SENSORS_PROPERTY_DAYLIGHT_SAVINGS_TIME_OBSERVED                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x533))

/* Sensors Page: Property: Time Trim Adjustment [DV] */
#define HID_USAGE_SENSORS_PROPERTY_TIME_TRIM_ADJUSTMENT (HID_USAGE(HID_USAGE_SENSORS, 0x534))

/* Sensors Page: Property: Arm Alarm [DF] */
#define HID_USAGE_SENSORS_PROPERTY_ARM_ALARM (HID_USAGE(HID_USAGE_SENSORS, 0x535))

/* Sensors Page: Data Field: Custom [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM (HID_USAGE(HID_USAGE_SENSORS, 0x540))

/* Sensors Page: Data Field: Custom Usage [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_USAGE (HID_USAGE(HID_USAGE_SENSORS, 0x541))

/* Sensors Page: Data Field: Custom Boolean Array [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_BOOLEAN_ARRAY (HID_USAGE(HID_USAGE_SENSORS, 0x542))

/* Sensors Page: Data Field: Custom Value [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE (HID_USAGE(HID_USAGE_SENSORS, 0x543))

/* Sensors Page: Data Field: Custom Value 1 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_1 (HID_USAGE(HID_USAGE_SENSORS, 0x544))

/* Sensors Page: Data Field: Custom Value 2 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_2 (HID_USAGE(HID_USAGE_SENSORS, 0x545))

/* Sensors Page: Data Field: Custom Value 3 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_3 (HID_USAGE(HID_USAGE_SENSORS, 0x546))

/* Sensors Page: Data Field: Custom Value 4 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_4 (HID_USAGE(HID_USAGE_SENSORS, 0x547))

/* Sensors Page: Data Field: Custom Value 5 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_5 (HID_USAGE(HID_USAGE_SENSORS, 0x548))

/* Sensors Page: Data Field: Custom Value 6 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_6 (HID_USAGE(HID_USAGE_SENSORS, 0x549))

/* Sensors Page: Data Field: Custom Value 7 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_7 (HID_USAGE(HID_USAGE_SENSORS, 0x54A))

/* Sensors Page: Data Field: Custom Value 8 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_8 (HID_USAGE(HID_USAGE_SENSORS, 0x54B))

/* Sensors Page: Data Field: Custom Value 9 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_9 (HID_USAGE(HID_USAGE_SENSORS, 0x54C))

/* Sensors Page: Data Field: Custom Value 10 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_10 (HID_USAGE(HID_USAGE_SENSORS, 0x54D))

/* Sensors Page: Data Field: Custom Value 11 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_11 (HID_USAGE(HID_USAGE_SENSORS, 0x54E))

/* Sensors Page: Data Field: Custom Value 12 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_12 (HID_USAGE(HID_USAGE_SENSORS, 0x54F))

/* Sensors Page: Data Field: Custom Value 13 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_13 (HID_USAGE(HID_USAGE_SENSORS, 0x550))

/* Sensors Page: Data Field: Custom Value 14 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_14 (HID_USAGE(HID_USAGE_SENSORS, 0x551))

/* Sensors Page: Data Field: Custom Value 15 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_15 (HID_USAGE(HID_USAGE_SENSORS, 0x552))

/* Sensors Page: Data Field: Custom Value 16 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_16 (HID_USAGE(HID_USAGE_SENSORS, 0x553))

/* Sensors Page: Data Field: Custom Value 17 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_17 (HID_USAGE(HID_USAGE_SENSORS, 0x554))

/* Sensors Page: Data Field: Custom Value 18 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_18 (HID_USAGE(HID_USAGE_SENSORS, 0x555))

/* Sensors Page: Data Field: Custom Value 19 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_19 (HID_USAGE(HID_USAGE_SENSORS, 0x556))

/* Sensors Page: Data Field: Custom Value 20 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_20 (HID_USAGE(HID_USAGE_SENSORS, 0x557))

/* Sensors Page: Data Field: Custom Value 21 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_21 (HID_USAGE(HID_USAGE_SENSORS, 0x558))

/* Sensors Page: Data Field: Custom Value 22 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_22 (HID_USAGE(HID_USAGE_SENSORS, 0x559))

/* Sensors Page: Data Field: Custom Value 23 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_23 (HID_USAGE(HID_USAGE_SENSORS, 0x55A))

/* Sensors Page: Data Field: Custom Value 24 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_24 (HID_USAGE(HID_USAGE_SENSORS, 0x55B))

/* Sensors Page: Data Field: Custom Value 25 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_25 (HID_USAGE(HID_USAGE_SENSORS, 0x55C))

/* Sensors Page: Data Field: Custom Value 26 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_26 (HID_USAGE(HID_USAGE_SENSORS, 0x55D))

/* Sensors Page: Data Field: Custom Value 27 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_27 (HID_USAGE(HID_USAGE_SENSORS, 0x55E))

/* Sensors Page: Data Field: Custom Value 28 [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_VALUE_28 (HID_USAGE(HID_USAGE_SENSORS, 0x55F))

/* Sensors Page: Data Field: Generic [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC (HID_USAGE(HID_USAGE_SENSORS, 0x560))

/* Sensors Page: Data Field: Generic GUID or PROPERTYKEY [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_GUID_OR_PROPERTYKEY                                   \
    (HID_USAGE(HID_USAGE_SENSORS, 0x561))

/* Sensors Page: Data Field: Generic Category GUID [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_CATEGORY_GUID (HID_USAGE(HID_USAGE_SENSORS, 0x562))

/* Sensors Page: Data Field: Generic Type GUID [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_TYPE_GUID (HID_USAGE(HID_USAGE_SENSORS, 0x563))

/* Sensors Page: Data Field: Generic Event PROPERTYKEY [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_EVENT_PROPERTYKEY (HID_USAGE(HID_USAGE_SENSORS, 0x564))

/* Sensors Page: Data Field: Generic Property PROPERTYKEY [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_PROPERTY_PROPERTYKEY                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x565))

/* Sensors Page: Data Field: Generic Data Field PROPERTYKEY [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_DATA_FIELD_PROPERTYKEY                                \
    (HID_USAGE(HID_USAGE_SENSORS, 0x566))

/* Sensors Page: Data Field: Generic Event [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_EVENT (HID_USAGE(HID_USAGE_SENSORS, 0x567))

/* Sensors Page: Data Field: Generic Property [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_PROPERTY (HID_USAGE(HID_USAGE_SENSORS, 0x568))

/* Sensors Page: Data Field: Generic Data Field [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_DATA_FIELD (HID_USAGE(HID_USAGE_SENSORS, 0x569))

/* Sensors Page: Data Field: Enumerator Table Row Index [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ENUMERATOR_TABLE_ROW_INDEX                                    \
    (HID_USAGE(HID_USAGE_SENSORS, 0x56A))

/* Sensors Page: Data Field: Enumerator Table Row Count [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_ENUMERATOR_TABLE_ROW_COUNT                                    \
    (HID_USAGE(HID_USAGE_SENSORS, 0x56B))

/* Sensors Page: Data Field: Generic GUID or PROPERTYKEY kind [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_GUID_OR_PROPERTYKEY_KIND                              \
    (HID_USAGE(HID_USAGE_SENSORS, 0x56C))

/* Sensors Page: Data Field: Generic GUID [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_GUID (HID_USAGE(HID_USAGE_SENSORS, 0x56D))

/* Sensors Page: Data Field: Generic PROPERTYKEY [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_PROPERTYKEY (HID_USAGE(HID_USAGE_SENSORS, 0x56E))

/* Sensors Page: Data Field: Generic Top Level Collection ID [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_TOP_LEVEL_COLLECTION_ID                               \
    (HID_USAGE(HID_USAGE_SENSORS, 0x56F))

/* Sensors Page: Data Field: Generic Report ID [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_REPORT_ID (HID_USAGE(HID_USAGE_SENSORS, 0x570))

/* Sensors Page: Data Field: Generic Report Item Position Index [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_REPORT_ITEM_POSITION_INDEX                            \
    (HID_USAGE(HID_USAGE_SENSORS, 0x571))

/* Sensors Page: Data Field: Generic Firmware VARTYPE [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_FIRMWARE_VARTYPE (HID_USAGE(HID_USAGE_SENSORS, 0x572))

/* Sensors Page: Data Field: Generic Unit of Measure [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_UNIT_OF_MEASURE (HID_USAGE(HID_USAGE_SENSORS, 0x573))

/* Sensors Page: Data Field: Generic Unit Exponent [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_UNIT_EXPONENT (HID_USAGE(HID_USAGE_SENSORS, 0x574))

/* Sensors Page: Data Field: Generic Report Size [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_REPORT_SIZE (HID_USAGE(HID_USAGE_SENSORS, 0x575))

/* Sensors Page: Data Field: Generic Report Count [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GENERIC_REPORT_COUNT (HID_USAGE(HID_USAGE_SENSORS, 0x576))

/* Sensors Page: Property: Generic [DV] */
#define HID_USAGE_SENSORS_PROPERTY_GENERIC (HID_USAGE(HID_USAGE_SENSORS, 0x580))

/* Sensors Page: Property: Enumerator Table Row Index [DV] */
#define HID_USAGE_SENSORS_PROPERTY_ENUMERATOR_TABLE_ROW_INDEX (HID_USAGE(HID_USAGE_SENSORS, 0x581))

/* Sensors Page: Property: Enumerator Table Row Count [SV] */
#define HID_USAGE_SENSORS_PROPERTY_ENUMERATOR_TABLE_ROW_COUNT (HID_USAGE(HID_USAGE_SENSORS, 0x582))

/* Sensors Page: Data Field: Personal Activity [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_PERSONAL_ACTIVITY (HID_USAGE(HID_USAGE_SENSORS, 0x590))

/* Sensors Page: Data Field: Activity Type [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_ACTIVITY_TYPE (HID_USAGE(HID_USAGE_SENSORS, 0x591))

/* Sensors Page: Data Field: Activity State [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_ACTIVITY_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x592))

/* Sensors Page: Data Field: Device Position [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_DEVICE_POSITION (HID_USAGE(HID_USAGE_SENSORS, 0x593))

/* Sensors Page: Data Field: Step Count [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_STEP_COUNT (HID_USAGE(HID_USAGE_SENSORS, 0x594))

/* Sensors Page: Data Field: Step Count Reset [DF] */
#define HID_USAGE_SENSORS_DATA_FIELD_STEP_COUNT_RESET (HID_USAGE(HID_USAGE_SENSORS, 0x595))

/* Sensors Page: Data Field: Step Duration [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_STEP_DURATION (HID_USAGE(HID_USAGE_SENSORS, 0x596))

/* Sensors Page: Data Field: Step Type [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_STEP_TYPE (HID_USAGE(HID_USAGE_SENSORS, 0x597))

/* Sensors Page: Property: Minimum Activity Detection Interval [DV] */
#define HID_USAGE_SENSORS_PROPERTY_MINIMUM_ACTIVITY_DETECTION_INTERVAL                             \
    (HID_USAGE(HID_USAGE_SENSORS, 0x5A0))

/* Sensors Page: Property: Supported Activity Types [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_SUPPORTED_ACTIVITY_TYPES (HID_USAGE(HID_USAGE_SENSORS, 0x5A1))

/* Sensors Page: Property: Subscribed Activity Types [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_SUBSCRIBED_ACTIVITY_TYPES (HID_USAGE(HID_USAGE_SENSORS, 0x5A2))

/* Sensors Page: Property: Supported Step Types [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_SUPPORTED_STEP_TYPES (HID_USAGE(HID_USAGE_SENSORS, 0x5A3))

/* Sensors Page: Property: Subscribed Step Types [NAry] */
#define HID_USAGE_SENSORS_PROPERTY_SUBSCRIBED_STEP_TYPES (HID_USAGE(HID_USAGE_SENSORS, 0x5A4))

/* Sensors Page: Property: Floor Height [DV] */
#define HID_USAGE_SENSORS_PROPERTY_FLOOR_HEIGHT (HID_USAGE(HID_USAGE_SENSORS, 0x5A5))

/* Sensors Page: Data Field: Custom Type ID [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_CUSTOM_TYPE_ID (HID_USAGE(HID_USAGE_SENSORS, 0x5B0))

/* Sensors Page: Property: Custom [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM (HID_USAGE(HID_USAGE_SENSORS, 0x5C0))

/* Sensors Page: Property: Custom Value 1 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_1 (HID_USAGE(HID_USAGE_SENSORS, 0x5C1))

/* Sensors Page: Property: Custom Value 2 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_2 (HID_USAGE(HID_USAGE_SENSORS, 0x5C2))

/* Sensors Page: Property: Custom Value 3 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_3 (HID_USAGE(HID_USAGE_SENSORS, 0x5C3))

/* Sensors Page: Property: Custom Value 4 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_4 (HID_USAGE(HID_USAGE_SENSORS, 0x5C4))

/* Sensors Page: Property: Custom Value 5 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_5 (HID_USAGE(HID_USAGE_SENSORS, 0x5C5))

/* Sensors Page: Property: Custom Value 6 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_6 (HID_USAGE(HID_USAGE_SENSORS, 0x5C6))

/* Sensors Page: Property: Custom Value 7 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_7 (HID_USAGE(HID_USAGE_SENSORS, 0x5C7))

/* Sensors Page: Property: Custom Value 8 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_8 (HID_USAGE(HID_USAGE_SENSORS, 0x5C8))

/* Sensors Page: Property: Custom Value 9 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_9 (HID_USAGE(HID_USAGE_SENSORS, 0x5C9))

/* Sensors Page: Property: Custom Value 10 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_10 (HID_USAGE(HID_USAGE_SENSORS, 0x5CA))

/* Sensors Page: Property: Custom Value 11 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_11 (HID_USAGE(HID_USAGE_SENSORS, 0x5CB))

/* Sensors Page: Property: Custom Value 12 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_12 (HID_USAGE(HID_USAGE_SENSORS, 0x5CC))

/* Sensors Page: Property: Custom Value 13 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_13 (HID_USAGE(HID_USAGE_SENSORS, 0x5CD))

/* Sensors Page: Property: Custom Value 14 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_14 (HID_USAGE(HID_USAGE_SENSORS, 0x5CE))

/* Sensors Page: Property: Custom Value 15 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_15 (HID_USAGE(HID_USAGE_SENSORS, 0x5CF))

/* Sensors Page: Property: Custom Value 16 [DV] */
#define HID_USAGE_SENSORS_PROPERTY_CUSTOM_VALUE_16 (HID_USAGE(HID_USAGE_SENSORS, 0x5D0))

/* Sensors Page: Data Field: Hinge [SV, DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HINGE (HID_USAGE(HID_USAGE_SENSORS, 0x5E0))

/* Sensors Page: Data Field: Hinge Angle [SV, DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HINGE_ANGLE (HID_USAGE(HID_USAGE_SENSORS, 0x5E1))

/* Sensors Page: Data Field: Gesture Sensor [DV] */
#define HID_USAGE_SENSORS_DATA_FIELD_GESTURE_SENSOR (HID_USAGE(HID_USAGE_SENSORS, 0x5F0))

/* Sensors Page: Data Field: Gesture State [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_GESTURE_STATE (HID_USAGE(HID_USAGE_SENSORS, 0x5F1))

/* Sensors Page: Data Field: Hinge Fold Initial Angle [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HINGE_FOLD_INITIAL_ANGLE (HID_USAGE(HID_USAGE_SENSORS, 0x5F2))

/* Sensors Page: Data Field: Hinge Fold Final Angle [SV] */
#define HID_USAGE_SENSORS_DATA_FIELD_HINGE_FOLD_FINAL_ANGLE (HID_USAGE(HID_USAGE_SENSORS, 0x5F3))

/* Sensors Page: Data Field: Hinge Fold Contributing Panel [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_HINGE_FOLD_CONTRIBUTING_PANEL                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x5F4))

/* Sensors Page: Data Field: Hinge Fold Type [NAry] */
#define HID_USAGE_SENSORS_DATA_FIELD_HINGE_FOLD_TYPE (HID_USAGE(HID_USAGE_SENSORS, 0x5F5))

/* Sensors Page: Sensor State: Undefined [Sel] */
#define HID_USAGE_SENSORS_SENSOR_STATE_UNDEFINED (HID_USAGE(HID_USAGE_SENSORS, 0x800))

/* Sensors Page: Sensor State: Ready [Sel] */
#define HID_USAGE_SENSORS_SENSOR_STATE_READY (HID_USAGE(HID_USAGE_SENSORS, 0x801))

/* Sensors Page: Sensor State: Not Available [Sel] */
#define HID_USAGE_SENSORS_SENSOR_STATE_NOT_AVAILABLE (HID_USAGE(HID_USAGE_SENSORS, 0x802))

/* Sensors Page: Sensor State: No Data [Sel] */
#define HID_USAGE_SENSORS_SENSOR_STATE_NO_DATA (HID_USAGE(HID_USAGE_SENSORS, 0x803))

/* Sensors Page: Sensor State: Initializing [Sel] */
#define HID_USAGE_SENSORS_SENSOR_STATE_INITIALIZING (HID_USAGE(HID_USAGE_SENSORS, 0x804))

/* Sensors Page: Sensor State: Access Denied [Sel] */
#define HID_USAGE_SENSORS_SENSOR_STATE_ACCESS_DENIED (HID_USAGE(HID_USAGE_SENSORS, 0x805))

/* Sensors Page: Sensor State: Error [Sel] */
#define HID_USAGE_SENSORS_SENSOR_STATE_ERROR (HID_USAGE(HID_USAGE_SENSORS, 0x806))

/* Sensors Page: Sensor Event: Unknown [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_UNKNOWN (HID_USAGE(HID_USAGE_SENSORS, 0x810))

/* Sensors Page: Sensor Event: State Changed [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_STATE_CHANGED (HID_USAGE(HID_USAGE_SENSORS, 0x811))

/* Sensors Page: Sensor Event: Property Changed [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_PROPERTY_CHANGED (HID_USAGE(HID_USAGE_SENSORS, 0x812))

/* Sensors Page: Sensor Event: Data Updated [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_DATA_UPDATED (HID_USAGE(HID_USAGE_SENSORS, 0x813))

/* Sensors Page: Sensor Event: Poll Response [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_POLL_RESPONSE (HID_USAGE(HID_USAGE_SENSORS, 0x814))

/* Sensors Page: Sensor Event: Change Sensitivity [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_CHANGE_SENSITIVITY (HID_USAGE(HID_USAGE_SENSORS, 0x815))

/* Sensors Page: Sensor Event: Range Maximum Reached [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_RANGE_MAXIMUM_REACHED (HID_USAGE(HID_USAGE_SENSORS, 0x816))

/* Sensors Page: Sensor Event: Range Minimum Reached [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_RANGE_MINIMUM_REACHED (HID_USAGE(HID_USAGE_SENSORS, 0x817))

/* Sensors Page: Sensor Event: High Threshold Cross Upward [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_HIGH_THRESHOLD_CROSS_UPWARD                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x818))

/* Sensors Page: Sensor Event: High Threshold Cross Downward [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_HIGH_THRESHOLD_CROSS_DOWNWARD                               \
    (HID_USAGE(HID_USAGE_SENSORS, 0x819))

/* Sensors Page: Sensor Event: Low Threshold Cross Upward [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_LOW_THRESHOLD_CROSS_UPWARD                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x81A))

/* Sensors Page: Sensor Event: Low Threshold Cross Downward [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_LOW_THRESHOLD_CROSS_DOWNWARD                                \
    (HID_USAGE(HID_USAGE_SENSORS, 0x81B))

/* Sensors Page: Sensor Event: Zero Threshold Cross Upward [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_ZERO_THRESHOLD_CROSS_UPWARD                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x81C))

/* Sensors Page: Sensor Event: Zero Threshold Cross Downward [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_ZERO_THRESHOLD_CROSS_DOWNWARD                               \
    (HID_USAGE(HID_USAGE_SENSORS, 0x81D))

/* Sensors Page: Sensor Event: Period Exceeded [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_PERIOD_EXCEEDED (HID_USAGE(HID_USAGE_SENSORS, 0x81E))

/* Sensors Page: Sensor Event: Frequency Exceeded [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_FREQUENCY_EXCEEDED (HID_USAGE(HID_USAGE_SENSORS, 0x81F))

/* Sensors Page: Sensor Event: Complex Trigger [Sel] */
#define HID_USAGE_SENSORS_SENSOR_EVENT_COMPLEX_TRIGGER (HID_USAGE(HID_USAGE_SENSORS, 0x820))

/* Sensors Page: Connection Type: PC Integrated [Sel] */
#define HID_USAGE_SENSORS_CONNECTION_TYPE_PC_INTEGRATED (HID_USAGE(HID_USAGE_SENSORS, 0x830))

/* Sensors Page: Connection Type: PC Attached [Sel] */
#define HID_USAGE_SENSORS_CONNECTION_TYPE_PC_ATTACHED (HID_USAGE(HID_USAGE_SENSORS, 0x831))

/* Sensors Page: Connection Type: PC External [Sel] */
#define HID_USAGE_SENSORS_CONNECTION_TYPE_PC_EXTERNAL (HID_USAGE(HID_USAGE_SENSORS, 0x832))

/* Sensors Page: Reporting State: Report No Events [Sel] */
#define HID_USAGE_SENSORS_REPORTING_STATE_REPORT_NO_EVENTS (HID_USAGE(HID_USAGE_SENSORS, 0x840))

/* Sensors Page: Reporting State: Report All Events [Sel] */
#define HID_USAGE_SENSORS_REPORTING_STATE_REPORT_ALL_EVENTS (HID_USAGE(HID_USAGE_SENSORS, 0x841))

/* Sensors Page: Reporting State: Report Threshold Events [Sel] */
#define HID_USAGE_SENSORS_REPORTING_STATE_REPORT_THRESHOLD_EVENTS                                  \
    (HID_USAGE(HID_USAGE_SENSORS, 0x842))

/* Sensors Page: Reporting State: Wake On No Events [Sel] */
#define HID_USAGE_SENSORS_REPORTING_STATE_WAKE_ON_NO_EVENTS (HID_USAGE(HID_USAGE_SENSORS, 0x843))

/* Sensors Page: Reporting State: Wake On All Events [Sel] */
#define HID_USAGE_SENSORS_REPORTING_STATE_WAKE_ON_ALL_EVENTS (HID_USAGE(HID_USAGE_SENSORS, 0x844))

/* Sensors Page: Reporting State: Wake On Threshold Events [Sel] */
#define HID_USAGE_SENSORS_REPORTING_STATE_WAKE_ON_THRESHOLD_EVENTS                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x845))

/* Sensors Page: Power State: Undefined [Sel] */
#define HID_USAGE_SENSORS_POWER_STATE_UNDEFINED (HID_USAGE(HID_USAGE_SENSORS, 0x850))

/* Sensors Page: Power State: D0 Full Power [Sel] */
#define HID_USAGE_SENSORS_POWER_STATE_D0_FULL_POWER (HID_USAGE(HID_USAGE_SENSORS, 0x851))

/* Sensors Page: Power State: D1 Low Power [Sel] */
#define HID_USAGE_SENSORS_POWER_STATE_D1_LOW_POWER (HID_USAGE(HID_USAGE_SENSORS, 0x852))

/* Sensors Page: Power State: D2 Standby Power with Wakeup [Sel] */
#define HID_USAGE_SENSORS_POWER_STATE_D2_STANDBY_POWER_WITH_WAKEUP                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x853))

/* Sensors Page: Power State: D3 Sleep with Wakeup [Sel] */
#define HID_USAGE_SENSORS_POWER_STATE_D3_SLEEP_WITH_WAKEUP (HID_USAGE(HID_USAGE_SENSORS, 0x854))

/* Sensors Page: Power State: D4 Power Off [Sel] */
#define HID_USAGE_SENSORS_POWER_STATE_D4_POWER_OFF (HID_USAGE(HID_USAGE_SENSORS, 0x855))

/* Sensors Page: Fix Quality: No Fix [Sel] */
#define HID_USAGE_SENSORS_FIX_QUALITY_NO_FIX (HID_USAGE(HID_USAGE_SENSORS, 0x870))

/* Sensors Page: Fix Quality: GPS [Sel] */
#define HID_USAGE_SENSORS_FIX_QUALITY_GPS (HID_USAGE(HID_USAGE_SENSORS, 0x871))

/* Sensors Page: Fix Quality: DGPS [Sel] */
#define HID_USAGE_SENSORS_FIX_QUALITY_DGPS (HID_USAGE(HID_USAGE_SENSORS, 0x872))

/* Sensors Page: Fix Type: No Fix [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_NO_FIX (HID_USAGE(HID_USAGE_SENSORS, 0x880))

/* Sensors Page: Fix Type: GPS SPS Mode, Fix Valid [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_GPS_SPS_MODE_FIX_VALID (HID_USAGE(HID_USAGE_SENSORS, 0x881))

/* Sensors Page: Fix Type: DGPS SPS Mode, Fix Valid [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_DGPS_SPS_MODE_FIX_VALID (HID_USAGE(HID_USAGE_SENSORS, 0x882))

/* Sensors Page: Fix Type: GPS PPS Mode, Fix Valid [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_GPS_PPS_MODE_FIX_VALID (HID_USAGE(HID_USAGE_SENSORS, 0x883))

/* Sensors Page: Fix Type: Real Time Kinematic [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_REAL_TIME_KINEMATIC (HID_USAGE(HID_USAGE_SENSORS, 0x884))

/* Sensors Page: Fix Type: Float RTK [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_FLOAT_RTK (HID_USAGE(HID_USAGE_SENSORS, 0x885))

/* Sensors Page: Fix Type: Estimated (dead reckoned) [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_ESTIMATED_DEAD_RECKONED (HID_USAGE(HID_USAGE_SENSORS, 0x886))

/* Sensors Page: Fix Type: Manual Input Mode [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_MANUAL_INPUT_MODE (HID_USAGE(HID_USAGE_SENSORS, 0x887))

/* Sensors Page: Fix Type: Simulator Mode [Sel] */
#define HID_USAGE_SENSORS_FIX_TYPE_SIMULATOR_MODE (HID_USAGE(HID_USAGE_SENSORS, 0x888))

/* Sensors Page: GPS Operation Mode: Manual [Sel] */
#define HID_USAGE_SENSORS_GPS_OPERATION_MODE_MANUAL (HID_USAGE(HID_USAGE_SENSORS, 0x890))

/* Sensors Page: GPS Operation Mode: Automatic [Sel] */
#define HID_USAGE_SENSORS_GPS_OPERATION_MODE_AUTOMATIC (HID_USAGE(HID_USAGE_SENSORS, 0x891))

/* Sensors Page: GPS Selection Mode: Autonomous [Sel] */
#define HID_USAGE_SENSORS_GPS_SELECTION_MODE_AUTONOMOUS (HID_USAGE(HID_USAGE_SENSORS, 0x8A0))

/* Sensors Page: GPS Selection Mode: DGPS [Sel] */
#define HID_USAGE_SENSORS_GPS_SELECTION_MODE_DGPS (HID_USAGE(HID_USAGE_SENSORS, 0x8A1))

/* Sensors Page: GPS Selection Mode: Estimated (dead reckoned) [Sel] */
#define HID_USAGE_SENSORS_GPS_SELECTION_MODE_ESTIMATED_DEAD_RECKONED                               \
    (HID_USAGE(HID_USAGE_SENSORS, 0x8A2))

/* Sensors Page: GPS Selection Mode: Manual Input [Sel] */
#define HID_USAGE_SENSORS_GPS_SELECTION_MODE_MANUAL_INPUT (HID_USAGE(HID_USAGE_SENSORS, 0x8A3))

/* Sensors Page: GPS Selection Mode: Simulator [Sel] */
#define HID_USAGE_SENSORS_GPS_SELECTION_MODE_SIMULATOR (HID_USAGE(HID_USAGE_SENSORS, 0x8A4))

/* Sensors Page: GPS Selection Mode: Data Not Valid [Sel] */
#define HID_USAGE_SENSORS_GPS_SELECTION_MODE_DATA_NOT_VALID (HID_USAGE(HID_USAGE_SENSORS, 0x8A5))

/* Sensors Page: GPS Status Data: Valid [Sel] */
#define HID_USAGE_SENSORS_GPS_STATUS_DATA_VALID (HID_USAGE(HID_USAGE_SENSORS, 0x8B0))

/* Sensors Page: GPS Status Data: Not Valid [Sel] */
#define HID_USAGE_SENSORS_GPS_STATUS_DATA_NOT_VALID (HID_USAGE(HID_USAGE_SENSORS, 0x8B1))

/* Sensors Page: Accuracy: Default [Sel] */
#define HID_USAGE_SENSORS_ACCURACY_DEFAULT (HID_USAGE(HID_USAGE_SENSORS, 0x860))

/* Sensors Page: Accuracy: High [Sel] */
#define HID_USAGE_SENSORS_ACCURACY_HIGH (HID_USAGE(HID_USAGE_SENSORS, 0x861))

/* Sensors Page: Accuracy: Medium [Sel] */
#define HID_USAGE_SENSORS_ACCURACY_MEDIUM (HID_USAGE(HID_USAGE_SENSORS, 0x862))

/* Sensors Page: Accuracy: Low [Sel] */
#define HID_USAGE_SENSORS_ACCURACY_LOW (HID_USAGE(HID_USAGE_SENSORS, 0x863))

/* Sensors Page: Day of Week: Sunday [Sel] */
#define HID_USAGE_SENSORS_DAY_OF_WEEK_SUNDAY (HID_USAGE(HID_USAGE_SENSORS, 0x8C0))

/* Sensors Page: Day of Week: Monday [Sel] */
#define HID_USAGE_SENSORS_DAY_OF_WEEK_MONDAY (HID_USAGE(HID_USAGE_SENSORS, 0x8C1))

/* Sensors Page: Day of Week: Tuesday [Sel] */
#define HID_USAGE_SENSORS_DAY_OF_WEEK_TUESDAY (HID_USAGE(HID_USAGE_SENSORS, 0x8C2))

/* Sensors Page: Day of Week: Wednesday [Sel] */
#define HID_USAGE_SENSORS_DAY_OF_WEEK_WEDNESDAY (HID_USAGE(HID_USAGE_SENSORS, 0x8C3))

/* Sensors Page: Day of Week: Thursday [Sel] */
#define HID_USAGE_SENSORS_DAY_OF_WEEK_THURSDAY (HID_USAGE(HID_USAGE_SENSORS, 0x8C4))

/* Sensors Page: Day of Week: Friday [Sel] */
#define HID_USAGE_SENSORS_DAY_OF_WEEK_FRIDAY (HID_USAGE(HID_USAGE_SENSORS, 0x8C5))

/* Sensors Page: Day of Week: Saturday [Sel] */
#define HID_USAGE_SENSORS_DAY_OF_WEEK_SATURDAY (HID_USAGE(HID_USAGE_SENSORS, 0x8C6))

/* Sensors Page: Kind: Category [Sel] */
#define HID_USAGE_SENSORS_KIND_CATEGORY (HID_USAGE(HID_USAGE_SENSORS, 0x8D0))

/* Sensors Page: Kind: Type [Sel] */
#define HID_USAGE_SENSORS_KIND_TYPE (HID_USAGE(HID_USAGE_SENSORS, 0x8D1))

/* Sensors Page: Kind: Event [Sel] */
#define HID_USAGE_SENSORS_KIND_EVENT (HID_USAGE(HID_USAGE_SENSORS, 0x8D2))

/* Sensors Page: Kind: Property [Sel] */
#define HID_USAGE_SENSORS_KIND_PROPERTY (HID_USAGE(HID_USAGE_SENSORS, 0x8D3))

/* Sensors Page: Kind: Data Field [Sel] */
#define HID_USAGE_SENSORS_KIND_DATA_FIELD (HID_USAGE(HID_USAGE_SENSORS, 0x8D4))

/* Sensors Page: Magnetometer Accuracy: Low [Sel] */
#define HID_USAGE_SENSORS_MAGNETOMETER_ACCURACY_LOW (HID_USAGE(HID_USAGE_SENSORS, 0x8E0))

/* Sensors Page: Magnetometer Accuracy: Medium [Sel] */
#define HID_USAGE_SENSORS_MAGNETOMETER_ACCURACY_MEDIUM (HID_USAGE(HID_USAGE_SENSORS, 0x8E1))

/* Sensors Page: Magnetometer Accuracy: High [Sel] */
#define HID_USAGE_SENSORS_MAGNETOMETER_ACCURACY_HIGH (HID_USAGE(HID_USAGE_SENSORS, 0x8E2))

/* Sensors Page: Simple Orientation Direction: Not Rotated [Sel] */
#define HID_USAGE_SENSORS_SIMPLE_ORIENTATION_DIRECTION_NOT_ROTATED                                 \
    (HID_USAGE(HID_USAGE_SENSORS, 0x8F0))

/* Sensors Page: Simple Orientation Direction: Rotated 90 Degrees CCW [Sel] */
#define HID_USAGE_SENSORS_SIMPLE_ORIENTATION_DIRECTION_ROTATED_90_DEGREES_CCW                      \
    (HID_USAGE(HID_USAGE_SENSORS, 0x8F1))

/* Sensors Page: Simple Orientation Direction: Rotated 180 Degrees CCW [Sel] */
#define HID_USAGE_SENSORS_SIMPLE_ORIENTATION_DIRECTION_ROTATED_180_DEGREES_CCW                     \
    (HID_USAGE(HID_USAGE_SENSORS, 0x8F2))

/* Sensors Page: Simple Orientation Direction: Rotated 270 Degrees CCW [Sel] */
#define HID_USAGE_SENSORS_SIMPLE_ORIENTATION_DIRECTION_ROTATED_270_DEGREES_CCW                     \
    (HID_USAGE(HID_USAGE_SENSORS, 0x8F3))

/* Sensors Page: Simple Orientation Direction: Face Up [Sel] */
#define HID_USAGE_SENSORS_SIMPLE_ORIENTATION_DIRECTION_FACE_UP (HID_USAGE(HID_USAGE_SENSORS, 0x8F4))

/* Sensors Page: Simple Orientation Direction: Face Down [Sel] */
#define HID_USAGE_SENSORS_SIMPLE_ORIENTATION_DIRECTION_FACE_DOWN                                   \
    (HID_USAGE(HID_USAGE_SENSORS, 0x8F5))

/* Sensors Page: VT_NULL [Sel] */
#define HID_USAGE_SENSORS_VT_NULL (HID_USAGE(HID_USAGE_SENSORS, 0x900))

/* Sensors Page: VT_BOOL [Sel] */
#define HID_USAGE_SENSORS_VT_BOOL (HID_USAGE(HID_USAGE_SENSORS, 0x901))

/* Sensors Page: VT_UI1 [Sel] */
#define HID_USAGE_SENSORS_VT_UI1 (HID_USAGE(HID_USAGE_SENSORS, 0x902))

/* Sensors Page: VT_I1 [Sel] */
#define HID_USAGE_SENSORS_VT_I1 (HID_USAGE(HID_USAGE_SENSORS, 0x903))

/* Sensors Page: VT_UI2 [Sel] */
#define HID_USAGE_SENSORS_VT_UI2 (HID_USAGE(HID_USAGE_SENSORS, 0x904))

/* Sensors Page: VT_I2 [Sel] */
#define HID_USAGE_SENSORS_VT_I2 (HID_USAGE(HID_USAGE_SENSORS, 0x905))

/* Sensors Page: VT_UI4 [Sel] */
#define HID_USAGE_SENSORS_VT_UI4 (HID_USAGE(HID_USAGE_SENSORS, 0x906))

/* Sensors Page: VT_I4 [Sel] */
#define HID_USAGE_SENSORS_VT_I4 (HID_USAGE(HID_USAGE_SENSORS, 0x907))

/* Sensors Page: VT_UI8 [Sel] */
#define HID_USAGE_SENSORS_VT_UI8 (HID_USAGE(HID_USAGE_SENSORS, 0x908))

/* Sensors Page: VT_I8 [Sel] */
#define HID_USAGE_SENSORS_VT_I8 (HID_USAGE(HID_USAGE_SENSORS, 0x909))

/* Sensors Page: VT_R4 [Sel] */
#define HID_USAGE_SENSORS_VT_R4 (HID_USAGE(HID_USAGE_SENSORS, 0x90A))

/* Sensors Page: VT_R8 [Sel] */
#define HID_USAGE_SENSORS_VT_R8 (HID_USAGE(HID_USAGE_SENSORS, 0x90B))

/* Sensors Page: VT_WSTR [Sel] */
#define HID_USAGE_SENSORS_VT_WSTR (HID_USAGE(HID_USAGE_SENSORS, 0x90C))

/* Sensors Page: VT_STR [Sel] */
#define HID_USAGE_SENSORS_VT_STR (HID_USAGE(HID_USAGE_SENSORS, 0x90D))

/* Sensors Page: VT_CLSID [Sel] */
#define HID_USAGE_SENSORS_VT_CLSID (HID_USAGE(HID_USAGE_SENSORS, 0x90E))

/* Sensors Page: VT_VECTOR VT_UI1 [Sel] */
#define HID_USAGE_SENSORS_VT_VECTOR_VT_UI1 (HID_USAGE(HID_USAGE_SENSORS, 0x90F))

/* Sensors Page: VT_F16E0 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E0 (HID_USAGE(HID_USAGE_SENSORS, 0x910))

/* Sensors Page: VT_F16E1 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E1 (HID_USAGE(HID_USAGE_SENSORS, 0x911))

/* Sensors Page: VT_F16E2 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E2 (HID_USAGE(HID_USAGE_SENSORS, 0x912))

/* Sensors Page: VT_F16E3 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E3 (HID_USAGE(HID_USAGE_SENSORS, 0x913))

/* Sensors Page: VT_F16E4 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E4 (HID_USAGE(HID_USAGE_SENSORS, 0x914))

/* Sensors Page: VT_F16E5 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E5 (HID_USAGE(HID_USAGE_SENSORS, 0x915))

/* Sensors Page: VT_F16E6 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E6 (HID_USAGE(HID_USAGE_SENSORS, 0x916))

/* Sensors Page: VT_F16E7 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E7 (HID_USAGE(HID_USAGE_SENSORS, 0x917))

/* Sensors Page: VT_F16E8 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E8 (HID_USAGE(HID_USAGE_SENSORS, 0x918))

/* Sensors Page: VT_F16E9 [Sel] */
#define HID_USAGE_SENSORS_VT_F16E9 (HID_USAGE(HID_USAGE_SENSORS, 0x919))

/* Sensors Page: VT_F16EA [Sel] */
#define HID_USAGE_SENSORS_VT_F16EA (HID_USAGE(HID_USAGE_SENSORS, 0x91A))

/* Sensors Page: VT_F16EB [Sel] */
#define HID_USAGE_SENSORS_VT_F16EB (HID_USAGE(HID_USAGE_SENSORS, 0x91B))

/* Sensors Page: VT_F16EC [Sel] */
#define HID_USAGE_SENSORS_VT_F16EC (HID_USAGE(HID_USAGE_SENSORS, 0x91C))

/* Sensors Page: VT_F16ED [Sel] */
#define HID_USAGE_SENSORS_VT_F16ED (HID_USAGE(HID_USAGE_SENSORS, 0x91D))

/* Sensors Page: VT_F16EE [Sel] */
#define HID_USAGE_SENSORS_VT_F16EE (HID_USAGE(HID_USAGE_SENSORS, 0x91E))

/* Sensors Page: VT_F16EF [Sel] */
#define HID_USAGE_SENSORS_VT_F16EF (HID_USAGE(HID_USAGE_SENSORS, 0x91F))

/* Sensors Page: VT_F32E0 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E0 (HID_USAGE(HID_USAGE_SENSORS, 0x920))

/* Sensors Page: VT_F32E1 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E1 (HID_USAGE(HID_USAGE_SENSORS, 0x921))

/* Sensors Page: VT_F32E2 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E2 (HID_USAGE(HID_USAGE_SENSORS, 0x922))

/* Sensors Page: VT_F32E3 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E3 (HID_USAGE(HID_USAGE_SENSORS, 0x923))

/* Sensors Page: VT_F32E4 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E4 (HID_USAGE(HID_USAGE_SENSORS, 0x924))

/* Sensors Page: VT_F32E5 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E5 (HID_USAGE(HID_USAGE_SENSORS, 0x925))

/* Sensors Page: VT_F32E6 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E6 (HID_USAGE(HID_USAGE_SENSORS, 0x926))

/* Sensors Page: VT_F32E7 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E7 (HID_USAGE(HID_USAGE_SENSORS, 0x927))

/* Sensors Page: VT_F32E8 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E8 (HID_USAGE(HID_USAGE_SENSORS, 0x928))

/* Sensors Page: VT_F32E9 [Sel] */
#define HID_USAGE_SENSORS_VT_F32E9 (HID_USAGE(HID_USAGE_SENSORS, 0x929))

/* Sensors Page: VT_F32EA [Sel] */
#define HID_USAGE_SENSORS_VT_F32EA (HID_USAGE(HID_USAGE_SENSORS, 0x92A))

/* Sensors Page: VT_F32EB [Sel] */
#define HID_USAGE_SENSORS_VT_F32EB (HID_USAGE(HID_USAGE_SENSORS, 0x92B))

/* Sensors Page: VT_F32EC [Sel] */
#define HID_USAGE_SENSORS_VT_F32EC (HID_USAGE(HID_USAGE_SENSORS, 0x92C))

/* Sensors Page: VT_F32ED [Sel] */
#define HID_USAGE_SENSORS_VT_F32ED (HID_USAGE(HID_USAGE_SENSORS, 0x92D))

/* Sensors Page: VT_F32EE [Sel] */
#define HID_USAGE_SENSORS_VT_F32EE (HID_USAGE(HID_USAGE_SENSORS, 0x92E))

/* Sensors Page: VT_F32EF [Sel] */
#define HID_USAGE_SENSORS_VT_F32EF (HID_USAGE(HID_USAGE_SENSORS, 0x92F))

/* Sensors Page: Activity Type: Unknown [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_UNKNOWN (HID_USAGE(HID_USAGE_SENSORS, 0x930))

/* Sensors Page: Activity Type: Stationary [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_STATIONARY (HID_USAGE(HID_USAGE_SENSORS, 0x931))

/* Sensors Page: Activity Type: Fidgeting [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_FIDGETING (HID_USAGE(HID_USAGE_SENSORS, 0x932))

/* Sensors Page: Activity Type: Walking [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_WALKING (HID_USAGE(HID_USAGE_SENSORS, 0x933))

/* Sensors Page: Activity Type: Running [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_RUNNING (HID_USAGE(HID_USAGE_SENSORS, 0x934))

/* Sensors Page: Activity Type: In Vehicle [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_IN_VEHICLE (HID_USAGE(HID_USAGE_SENSORS, 0x935))

/* Sensors Page: Activity Type: Biking [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_BIKING (HID_USAGE(HID_USAGE_SENSORS, 0x936))

/* Sensors Page: Activity Type: Idle [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_TYPE_IDLE (HID_USAGE(HID_USAGE_SENSORS, 0x937))

/* Sensors Page: Unit: Not Specified [Sel] */
#define HID_USAGE_SENSORS_UNIT_NOT_SPECIFIED (HID_USAGE(HID_USAGE_SENSORS, 0x940))

/* Sensors Page: Unit: Lux [Sel] */
#define HID_USAGE_SENSORS_UNIT_LUX (HID_USAGE(HID_USAGE_SENSORS, 0x941))

/* Sensors Page: Unit: Degrees Kelvin [Sel] */
#define HID_USAGE_SENSORS_UNIT_DEGREES_KELVIN (HID_USAGE(HID_USAGE_SENSORS, 0x942))

/* Sensors Page: Unit: Degrees Celsius [Sel] */
#define HID_USAGE_SENSORS_UNIT_DEGREES_CELSIUS (HID_USAGE(HID_USAGE_SENSORS, 0x943))

/* Sensors Page: Unit: Pascal [Sel] */
#define HID_USAGE_SENSORS_UNIT_PASCAL (HID_USAGE(HID_USAGE_SENSORS, 0x944))

/* Sensors Page: Unit: Newton [Sel] */
#define HID_USAGE_SENSORS_UNIT_NEWTON (HID_USAGE(HID_USAGE_SENSORS, 0x945))

/* Sensors Page: Unit: Meters/Second [Sel] */
#define HID_USAGE_SENSORS_UNIT_METERS_SECOND (HID_USAGE(HID_USAGE_SENSORS, 0x946))

/* Sensors Page: Unit: Kilogram [Sel] */
#define HID_USAGE_SENSORS_UNIT_KILOGRAM (HID_USAGE(HID_USAGE_SENSORS, 0x947))

/* Sensors Page: Unit: Meter [Sel] */
#define HID_USAGE_SENSORS_UNIT_METER (HID_USAGE(HID_USAGE_SENSORS, 0x948))

/* Sensors Page: Unit: Meters/Second/Second [Sel] */
#define HID_USAGE_SENSORS_UNIT_METERS_SECOND_SECOND (HID_USAGE(HID_USAGE_SENSORS, 0x949))

/* Sensors Page: Unit: Farad [Sel] */
#define HID_USAGE_SENSORS_UNIT_FARAD (HID_USAGE(HID_USAGE_SENSORS, 0x94A))

/* Sensors Page: Unit: Ampere [Sel] */
#define HID_USAGE_SENSORS_UNIT_AMPERE (HID_USAGE(HID_USAGE_SENSORS, 0x94B))

/* Sensors Page: Unit: Watt [Sel] */
#define HID_USAGE_SENSORS_UNIT_WATT (HID_USAGE(HID_USAGE_SENSORS, 0x94C))

/* Sensors Page: Unit: Henry [Sel] */
#define HID_USAGE_SENSORS_UNIT_HENRY (HID_USAGE(HID_USAGE_SENSORS, 0x94D))

/* Sensors Page: Unit: Ohm [Sel] */
#define HID_USAGE_SENSORS_UNIT_OHM (HID_USAGE(HID_USAGE_SENSORS, 0x94E))

/* Sensors Page: Unit: Volt [Sel] */
#define HID_USAGE_SENSORS_UNIT_VOLT (HID_USAGE(HID_USAGE_SENSORS, 0x94F))

/* Sensors Page: Unit: Hertz [Sel] */
#define HID_USAGE_SENSORS_UNIT_HERTZ (HID_USAGE(HID_USAGE_SENSORS, 0x950))

/* Sensors Page: Unit: Bar [Sel] */
#define HID_USAGE_SENSORS_UNIT_BAR (HID_USAGE(HID_USAGE_SENSORS, 0x951))

/* Sensors Page: Unit: Degrees Anti-clockwise [Sel] */
#define HID_USAGE_SENSORS_UNIT_DEGREES_ANTI_CLOCKWISE (HID_USAGE(HID_USAGE_SENSORS, 0x952))

/* Sensors Page: Unit: Degrees Clockwise [Sel] */
#define HID_USAGE_SENSORS_UNIT_DEGREES_CLOCKWISE (HID_USAGE(HID_USAGE_SENSORS, 0x953))

/* Sensors Page: Unit: Degrees [Sel] */
#define HID_USAGE_SENSORS_UNIT_DEGREES (HID_USAGE(HID_USAGE_SENSORS, 0x954))

/* Sensors Page: Unit: Degrees/Second [Sel] */
#define HID_USAGE_SENSORS_UNIT_DEGREES_SECOND (HID_USAGE(HID_USAGE_SENSORS, 0x955))

/* Sensors Page: Unit: Degrees/Second/Second [Sel] */
#define HID_USAGE_SENSORS_UNIT_DEGREES_SECOND_SECOND (HID_USAGE(HID_USAGE_SENSORS, 0x956))

/* Sensors Page: Unit: Knot [Sel] */
#define HID_USAGE_SENSORS_UNIT_KNOT (HID_USAGE(HID_USAGE_SENSORS, 0x957))

/* Sensors Page: Unit: Percent [Sel] */
#define HID_USAGE_SENSORS_UNIT_PERCENT (HID_USAGE(HID_USAGE_SENSORS, 0x958))

/* Sensors Page: Unit: Second [Sel] */
#define HID_USAGE_SENSORS_UNIT_SECOND (HID_USAGE(HID_USAGE_SENSORS, 0x959))

/* Sensors Page: Unit: Millisecond [Sel] */
#define HID_USAGE_SENSORS_UNIT_MILLISECOND (HID_USAGE(HID_USAGE_SENSORS, 0x95A))

/* Sensors Page: Unit: G [Sel] */
#define HID_USAGE_SENSORS_UNIT_G (HID_USAGE(HID_USAGE_SENSORS, 0x95B))

/* Sensors Page: Unit: Bytes [Sel] */
#define HID_USAGE_SENSORS_UNIT_BYTES (HID_USAGE(HID_USAGE_SENSORS, 0x95C))

/* Sensors Page: Unit: Milligauss [Sel] */
#define HID_USAGE_SENSORS_UNIT_MILLIGAUSS (HID_USAGE(HID_USAGE_SENSORS, 0x95D))

/* Sensors Page: Unit: Bits [Sel] */
#define HID_USAGE_SENSORS_UNIT_BITS (HID_USAGE(HID_USAGE_SENSORS, 0x95E))

/* Sensors Page: Activity State: No State Change [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_STATE_NO_STATE_CHANGE (HID_USAGE(HID_USAGE_SENSORS, 0x960))

/* Sensors Page: Activity State: Start Activity [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_STATE_START_ACTIVITY (HID_USAGE(HID_USAGE_SENSORS, 0x961))

/* Sensors Page: Activity State: End Activity [Sel] */
#define HID_USAGE_SENSORS_ACTIVITY_STATE_END_ACTIVITY (HID_USAGE(HID_USAGE_SENSORS, 0x962))

/* Sensors Page: Exponent 0 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_0 (HID_USAGE(HID_USAGE_SENSORS, 0x970))

/* Sensors Page: Exponent 1 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_1 (HID_USAGE(HID_USAGE_SENSORS, 0x971))

/* Sensors Page: Exponent 2 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_2 (HID_USAGE(HID_USAGE_SENSORS, 0x972))

/* Sensors Page: Exponent 3 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_3 (HID_USAGE(HID_USAGE_SENSORS, 0x973))

/* Sensors Page: Exponent 4 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_4 (HID_USAGE(HID_USAGE_SENSORS, 0x974))

/* Sensors Page: Exponent 5 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_5 (HID_USAGE(HID_USAGE_SENSORS, 0x975))

/* Sensors Page: Exponent 6 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_6 (HID_USAGE(HID_USAGE_SENSORS, 0x976))

/* Sensors Page: Exponent 7 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_7 (HID_USAGE(HID_USAGE_SENSORS, 0x977))

/* Sensors Page: Exponent 8 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_8 (HID_USAGE(HID_USAGE_SENSORS, 0x978))

/* Sensors Page: Exponent 9 [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_9 (HID_USAGE(HID_USAGE_SENSORS, 0x979))

/* Sensors Page: Exponent A [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_A (HID_USAGE(HID_USAGE_SENSORS, 0x97A))

/* Sensors Page: Exponent B [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_B (HID_USAGE(HID_USAGE_SENSORS, 0x97B))

/* Sensors Page: Exponent C [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_C (HID_USAGE(HID_USAGE_SENSORS, 0x97C))

/* Sensors Page: Exponent D [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_D (HID_USAGE(HID_USAGE_SENSORS, 0x97D))

/* Sensors Page: Exponent E [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_E (HID_USAGE(HID_USAGE_SENSORS, 0x97E))

/* Sensors Page: Exponent F [Sel] */
#define HID_USAGE_SENSORS_EXPONENT_F (HID_USAGE(HID_USAGE_SENSORS, 0x97F))

/* Sensors Page: Device Position: Unknown [Sel] */
#define HID_USAGE_SENSORS_DEVICE_POSITION_UNKNOWN (HID_USAGE(HID_USAGE_SENSORS, 0x980))

/* Sensors Page: Device Position: Unchanged [Sel] */
#define HID_USAGE_SENSORS_DEVICE_POSITION_UNCHANGED (HID_USAGE(HID_USAGE_SENSORS, 0x981))

/* Sensors Page: Device Position: On Desk [Sel] */
#define HID_USAGE_SENSORS_DEVICE_POSITION_ON_DESK (HID_USAGE(HID_USAGE_SENSORS, 0x982))

/* Sensors Page: Device Position: In Hand [Sel] */
#define HID_USAGE_SENSORS_DEVICE_POSITION_IN_HAND (HID_USAGE(HID_USAGE_SENSORS, 0x983))

/* Sensors Page: Device Position: Moving in Bag [Sel] */
#define HID_USAGE_SENSORS_DEVICE_POSITION_MOVING_IN_BAG (HID_USAGE(HID_USAGE_SENSORS, 0x984))

/* Sensors Page: Device Position: Stationary in Bag [Sel] */
#define HID_USAGE_SENSORS_DEVICE_POSITION_STATIONARY_IN_BAG (HID_USAGE(HID_USAGE_SENSORS, 0x985))

/* Sensors Page: Step Type: Unknown [Sel] */
#define HID_USAGE_SENSORS_STEP_TYPE_UNKNOWN (HID_USAGE(HID_USAGE_SENSORS, 0x990))

/* Sensors Page: Step Type: Running [Sel] */
#define HID_USAGE_SENSORS_STEP_TYPE_RUNNING (HID_USAGE(HID_USAGE_SENSORS, 0x991))

/* Sensors Page: Step Type: Walking [Sel] */
#define HID_USAGE_SENSORS_STEP_TYPE_WALKING (HID_USAGE(HID_USAGE_SENSORS, 0x992))

/* Sensors Page: Gesture State: Unknown [Sel] */
#define HID_USAGE_SENSORS_GESTURE_STATE_UNKNOWN (HID_USAGE(HID_USAGE_SENSORS, 0x9A0))

/* Sensors Page: Gesture State: Started [Sel] */
#define HID_USAGE_SENSORS_GESTURE_STATE_STARTED (HID_USAGE(HID_USAGE_SENSORS, 0x9A1))

/* Sensors Page: Gesture State: Completed [Sel] */
#define HID_USAGE_SENSORS_GESTURE_STATE_COMPLETED (HID_USAGE(HID_USAGE_SENSORS, 0x9A2))

/* Sensors Page: Gesture State: Cancelled [Sel] */
#define HID_USAGE_SENSORS_GESTURE_STATE_CANCELLED (HID_USAGE(HID_USAGE_SENSORS, 0x9A3))

/* Sensors Page: Hinge Fold Contributing Panel: Unknown [Sel] */
#define HID_USAGE_SENSORS_HINGE_FOLD_CONTRIBUTING_PANEL_UNKNOWN                                    \
    (HID_USAGE(HID_USAGE_SENSORS, 0x9B0))

/* Sensors Page: Hinge Fold Contributing Panel: Panel 1 [Sel] */
#define HID_USAGE_SENSORS_HINGE_FOLD_CONTRIBUTING_PANEL_PANEL_1                                    \
    (HID_USAGE(HID_USAGE_SENSORS, 0x9B1))

/* Sensors Page: Hinge Fold Contributing Panel: Panel 2 [Sel] */
#define HID_USAGE_SENSORS_HINGE_FOLD_CONTRIBUTING_PANEL_PANEL_2                                    \
    (HID_USAGE(HID_USAGE_SENSORS, 0x9B2))

/* Sensors Page: Hinge Fold Contributing Panel: Both [Sel] */
#define HID_USAGE_SENSORS_HINGE_FOLD_CONTRIBUTING_PANEL_BOTH (HID_USAGE(HID_USAGE_SENSORS, 0x9B3))

/* Sensors Page: Hinge Fold Type: Unknown [Sel] */
#define HID_USAGE_SENSORS_HINGE_FOLD_TYPE_UNKNOWN (HID_USAGE(HID_USAGE_SENSORS, 0x9B4))

/* Sensors Page: Hinge Fold Type: Increasing [Sel] */
#define HID_USAGE_SENSORS_HINGE_FOLD_TYPE_INCREASING (HID_USAGE(HID_USAGE_SENSORS, 0x9B5))

/* Sensors Page: Hinge Fold Type: Decreasing [Sel] */
#define HID_USAGE_SENSORS_HINGE_FOLD_TYPE_DECREASING (HID_USAGE(HID_USAGE_SENSORS, 0x9B6))

/* Sensors Page: Modifier: Change Sensitivity Absolute [US] */
#define HID_USAGE_SENSORS_MODIFIER_CHANGE_SENSITIVITY_ABSOLUTE                                     \
    (HID_USAGE(HID_USAGE_SENSORS, 0x1000))

/* Sensors Page: Modifier: Maximum [US] */
#define HID_USAGE_SENSORS_MODIFIER_MAXIMUM (HID_USAGE(HID_USAGE_SENSORS, 0x2000))

/* Sensors Page: Modifier: Minimum [US] */
#define HID_USAGE_SENSORS_MODIFIER_MINIMUM (HID_USAGE(HID_USAGE_SENSORS, 0x3000))

/* Sensors Page: Modifier: Accuracy [US] */
#define HID_USAGE_SENSORS_MODIFIER_ACCURACY (HID_USAGE(HID_USAGE_SENSORS, 0x4000))

/* Sensors Page: Modifier: Resolution [US] */
#define HID_USAGE_SENSORS_MODIFIER_RESOLUTION (HID_USAGE(HID_USAGE_SENSORS, 0x5000))

/* Sensors Page: Modifier: Threshold High [US] */
#define HID_USAGE_SENSORS_MODIFIER_THRESHOLD_HIGH (HID_USAGE(HID_USAGE_SENSORS, 0x6000))

/* Sensors Page: Modifier: Threshold Low [US] */
#define HID_USAGE_SENSORS_MODIFIER_THRESHOLD_LOW (HID_USAGE(HID_USAGE_SENSORS, 0x7000))

/* Sensors Page: Modifier: Calibration Offset [US] */
#define HID_USAGE_SENSORS_MODIFIER_CALIBRATION_OFFSET (HID_USAGE(HID_USAGE_SENSORS, 0x8000))

/* Sensors Page: Modifier: Calibration Multiplier [US] */
#define HID_USAGE_SENSORS_MODIFIER_CALIBRATION_MULTIPLIER (HID_USAGE(HID_USAGE_SENSORS, 0x9000))

/* Sensors Page: Modifier: Report Interval [US] */
#define HID_USAGE_SENSORS_MODIFIER_REPORT_INTERVAL (HID_USAGE(HID_USAGE_SENSORS, 0xA000))

/* Sensors Page: Modifier: Frequency Max [US] */
#define HID_USAGE_SENSORS_MODIFIER_FREQUENCY_MAX (HID_USAGE(HID_USAGE_SENSORS, 0xB000))

/* Sensors Page: Modifier: Period Max [US] */
#define HID_USAGE_SENSORS_MODIFIER_PERIOD_MAX (HID_USAGE(HID_USAGE_SENSORS, 0xC000))

/* Sensors Page: Modifier: Change Sensitivity Percent of Range [US] */
#define HID_USAGE_SENSORS_MODIFIER_CHANGE_SENSITIVITY_PERCENT_OF_RANGE                             \
    (HID_USAGE(HID_USAGE_SENSORS, 0xD000))

/* Sensors Page: Modifier: Change Sensitivity Percent Relative [US] */
#define HID_USAGE_SENSORS_MODIFIER_CHANGE_SENSITIVITY_PERCENT_RELATIVE                             \
    (HID_USAGE(HID_USAGE_SENSORS, 0xE000))

/* Medical Instrument Page */
#define HID_USAGE_MEDICAL (0x40)

/* Medical Instrument Page: Undefined */
#define HID_USAGE_MEDICAL_UNDEFINED (HID_USAGE(HID_USAGE_MEDICAL, 0x00))

/* Medical Instrument Page: Medical Ultrasound [CA] */
#define HID_USAGE_MEDICAL_MEDICAL_ULTRASOUND (HID_USAGE(HID_USAGE_MEDICAL, 0x01))

/* Medical Instrument Page: VCR/Acquisition [OOC] */
#define HID_USAGE_MEDICAL_VCR_ACQUISITION (HID_USAGE(HID_USAGE_MEDICAL, 0x20))

/* Medical Instrument Page: Freeze/Thaw [OOC] */
#define HID_USAGE_MEDICAL_FREEZE_THAW (HID_USAGE(HID_USAGE_MEDICAL, 0x21))

/* Medical Instrument Page: Clip Store [OSC] */
#define HID_USAGE_MEDICAL_CLIP_STORE (HID_USAGE(HID_USAGE_MEDICAL, 0x22))

/* Medical Instrument Page: Update [OSC] */
#define HID_USAGE_MEDICAL_UPDATE (HID_USAGE(HID_USAGE_MEDICAL, 0x23))

/* Medical Instrument Page: Next [OSC] */
#define HID_USAGE_MEDICAL_NEXT (HID_USAGE(HID_USAGE_MEDICAL, 0x24))

/* Medical Instrument Page: Save [OSC] */
#define HID_USAGE_MEDICAL_SAVE (HID_USAGE(HID_USAGE_MEDICAL, 0x25))

/* Medical Instrument Page: Print [OSC] */
#define HID_USAGE_MEDICAL_PRINT (HID_USAGE(HID_USAGE_MEDICAL, 0x26))

/* Medical Instrument Page: Microphone Enable [OSC] */
#define HID_USAGE_MEDICAL_MICROPHONE_ENABLE (HID_USAGE(HID_USAGE_MEDICAL, 0x27))

/* Medical Instrument Page: Cine [LC] */
#define HID_USAGE_MEDICAL_CINE (HID_USAGE(HID_USAGE_MEDICAL, 0x40))

/* Medical Instrument Page: Transmit Power [LC] */
#define HID_USAGE_MEDICAL_TRANSMIT_POWER (HID_USAGE(HID_USAGE_MEDICAL, 0x41))

/* Medical Instrument Page: Volume [LC] */
#define HID_USAGE_MEDICAL_VOLUME (HID_USAGE(HID_USAGE_MEDICAL, 0x42))

/* Medical Instrument Page: Focus [LC] */
#define HID_USAGE_MEDICAL_FOCUS (HID_USAGE(HID_USAGE_MEDICAL, 0x43))

/* Medical Instrument Page: Depth [LC] */
#define HID_USAGE_MEDICAL_DEPTH (HID_USAGE(HID_USAGE_MEDICAL, 0x44))

/* Medical Instrument Page: Soft Step - Primary [LC] */
#define HID_USAGE_MEDICAL_SOFT_STEP_MINUS_PRIMARY (HID_USAGE(HID_USAGE_MEDICAL, 0x60))

/* Medical Instrument Page: Soft Step - Secondary [LC] */
#define HID_USAGE_MEDICAL_SOFT_STEP_MINUS_SECONDARY (HID_USAGE(HID_USAGE_MEDICAL, 0x61))

/* Medical Instrument Page: Depth Gain Compensation [LC] */
#define HID_USAGE_MEDICAL_DEPTH_GAIN_COMPENSATION (HID_USAGE(HID_USAGE_MEDICAL, 0x70))

/* Medical Instrument Page: Zoom Select [OSC] */
#define HID_USAGE_MEDICAL_ZOOM_SELECT (HID_USAGE(HID_USAGE_MEDICAL, 0x80))

/* Medical Instrument Page: Zoom Adjust [LC] */
#define HID_USAGE_MEDICAL_ZOOM_ADJUST (HID_USAGE(HID_USAGE_MEDICAL, 0x81))

/* Medical Instrument Page: Spectral Doppler Mode Select [OSC] */
#define HID_USAGE_MEDICAL_SPECTRAL_DOPPLER_MODE_SELECT (HID_USAGE(HID_USAGE_MEDICAL, 0x82))

/* Medical Instrument Page: Spectral Doppler Adjust [LC] */
#define HID_USAGE_MEDICAL_SPECTRAL_DOPPLER_ADJUST (HID_USAGE(HID_USAGE_MEDICAL, 0x83))

/* Medical Instrument Page: Color Doppler Mode Select [OSC] */
#define HID_USAGE_MEDICAL_COLOR_DOPPLER_MODE_SELECT (HID_USAGE(HID_USAGE_MEDICAL, 0x84))

/* Medical Instrument Page: Color Doppler Adjust [LC] */
#define HID_USAGE_MEDICAL_COLOR_DOPPLER_ADJUST (HID_USAGE(HID_USAGE_MEDICAL, 0x85))

/* Medical Instrument Page: Motion Mode Select [OSC] */
#define HID_USAGE_MEDICAL_MOTION_MODE_SELECT (HID_USAGE(HID_USAGE_MEDICAL, 0x86))

/* Medical Instrument Page: Motion Mode Adjust [LC] */
#define HID_USAGE_MEDICAL_MOTION_MODE_ADJUST (HID_USAGE(HID_USAGE_MEDICAL, 0x87))

/* Medical Instrument Page: 2-D Mode Select [OSC] */
#define HID_USAGE_MEDICAL_2_D_MODE_SELECT (HID_USAGE(HID_USAGE_MEDICAL, 0x88))

/* Medical Instrument Page: 2-D Mode Adjust [LC] */
#define HID_USAGE_MEDICAL_2_D_MODE_ADJUST (HID_USAGE(HID_USAGE_MEDICAL, 0x89))

/* Medical Instrument Page: Soft Control Select [OSC] */
#define HID_USAGE_MEDICAL_SOFT_CONTROL_SELECT (HID_USAGE(HID_USAGE_MEDICAL, 0xA0))

/* Medical Instrument Page: Soft Control Adjust [LC] */
#define HID_USAGE_MEDICAL_SOFT_CONTROL_ADJUST (HID_USAGE(HID_USAGE_MEDICAL, 0xA1))

/* Braille Display Page */
#define HID_USAGE_BRAILLE (0x41)

/* Braille Display Page: Undefined */
#define HID_USAGE_BRAILLE_UNDEFINED (HID_USAGE(HID_USAGE_BRAILLE, 0x00))

/* Braille Display Page: Braille Display [CA] */
#define HID_USAGE_BRAILLE_BRAILLE_DISPLAY (HID_USAGE(HID_USAGE_BRAILLE, 0x01))

/* Braille Display Page: Braille Row [NAry] */
#define HID_USAGE_BRAILLE_BRAILLE_ROW (HID_USAGE(HID_USAGE_BRAILLE, 0x02))

/* Braille Display Page: 8 Dot Braille Cell [DV] */
#define HID_USAGE_BRAILLE_8_DOT_BRAILLE_CELL (HID_USAGE(HID_USAGE_BRAILLE, 0x03))

/* Braille Display Page: 6 Dot Braille Cell [DV] */
#define HID_USAGE_BRAILLE_6_DOT_BRAILLE_CELL (HID_USAGE(HID_USAGE_BRAILLE, 0x04))

/* Braille Display Page: Number of Braille Cells [DV] */
#define HID_USAGE_BRAILLE_NUMBER_OF_BRAILLE_CELLS (HID_USAGE(HID_USAGE_BRAILLE, 0x05))

/* Braille Display Page: Screen Reader Control [NAry] */
#define HID_USAGE_BRAILLE_SCREEN_READER_CONTROL (HID_USAGE(HID_USAGE_BRAILLE, 0x06))

/* Braille Display Page: Screen Reader Identifier [DV] */
#define HID_USAGE_BRAILLE_SCREEN_READER_IDENTIFIER (HID_USAGE(HID_USAGE_BRAILLE, 0x07))

/* Braille Display Page: Router Set 1 [NAry] */
#define HID_USAGE_BRAILLE_ROUTER_SET_1 (HID_USAGE(HID_USAGE_BRAILLE, 0xFA))

/* Braille Display Page: Router Set 2 [NAry] */
#define HID_USAGE_BRAILLE_ROUTER_SET_2 (HID_USAGE(HID_USAGE_BRAILLE, 0xFB))

/* Braille Display Page: Router Set 3 [Nary] */
#define HID_USAGE_BRAILLE_ROUTER_SET_3 (HID_USAGE(HID_USAGE_BRAILLE, 0xFC))

/* Braille Display Page: Router Key [Sel] */
#define HID_USAGE_BRAILLE_ROUTER_KEY (HID_USAGE(HID_USAGE_BRAILLE, 0x100))

/* Braille Display Page: Row Router Key [Sel] */
#define HID_USAGE_BRAILLE_ROW_ROUTER_KEY (HID_USAGE(HID_USAGE_BRAILLE, 0x101))

/* Braille Display Page: Braille Buttons [NAry] */
#define HID_USAGE_BRAILLE_BRAILLE_BUTTONS (HID_USAGE(HID_USAGE_BRAILLE, 0x200))

/* Braille Display Page: Braille Keyboard Dot 1 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_1 (HID_USAGE(HID_USAGE_BRAILLE, 0x201))

/* Braille Display Page: Braille Keyboard Dot 2 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_2 (HID_USAGE(HID_USAGE_BRAILLE, 0x202))

/* Braille Display Page: Braille Keyboard Dot 3 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_3 (HID_USAGE(HID_USAGE_BRAILLE, 0x203))

/* Braille Display Page: Braille Keyboard Dot 4 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_4 (HID_USAGE(HID_USAGE_BRAILLE, 0x204))

/* Braille Display Page: Braille Keyboard Dot 5 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_5 (HID_USAGE(HID_USAGE_BRAILLE, 0x205))

/* Braille Display Page: Braille Keyboard Dot 6 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_6 (HID_USAGE(HID_USAGE_BRAILLE, 0x206))

/* Braille Display Page: Braille Keyboard Dot 7 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_7 (HID_USAGE(HID_USAGE_BRAILLE, 0x207))

/* Braille Display Page: Braille Keyboard Dot 8 [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_DOT_8 (HID_USAGE(HID_USAGE_BRAILLE, 0x208))

/* Braille Display Page: Braille Keyboard Space [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_SPACE (HID_USAGE(HID_USAGE_BRAILLE, 0x209))

/* Braille Display Page: Braille Keyboard Left Space [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_LEFT_SPACE (HID_USAGE(HID_USAGE_BRAILLE, 0x20A))

/* Braille Display Page: Braille Keyboard Right Space [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_KEYBOARD_RIGHT_SPACE (HID_USAGE(HID_USAGE_BRAILLE, 0x20B))

/* Braille Display Page: Braille Face Controls [NAry] */
#define HID_USAGE_BRAILLE_BRAILLE_FACE_CONTROLS (HID_USAGE(HID_USAGE_BRAILLE, 0x20C))

/* Braille Display Page: Braille Left Controls [NAry] */
#define HID_USAGE_BRAILLE_BRAILLE_LEFT_CONTROLS (HID_USAGE(HID_USAGE_BRAILLE, 0x20D))

/* Braille Display Page: Braille Right Controls [NAry] */
#define HID_USAGE_BRAILLE_BRAILLE_RIGHT_CONTROLS (HID_USAGE(HID_USAGE_BRAILLE, 0x20E))

/* Braille Display Page: Braille Top Controls [NAry] */
#define HID_USAGE_BRAILLE_BRAILLE_TOP_CONTROLS (HID_USAGE(HID_USAGE_BRAILLE, 0x20F))

/* Braille Display Page: Braille Joystick Center [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_JOYSTICK_CENTER (HID_USAGE(HID_USAGE_BRAILLE, 0x210))

/* Braille Display Page: Braille Joystick Up [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_JOYSTICK_UP (HID_USAGE(HID_USAGE_BRAILLE, 0x211))

/* Braille Display Page: Braille Joystick Down [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_JOYSTICK_DOWN (HID_USAGE(HID_USAGE_BRAILLE, 0x212))

/* Braille Display Page: Braille Joystick Left [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_JOYSTICK_LEFT (HID_USAGE(HID_USAGE_BRAILLE, 0x213))

/* Braille Display Page: Braille Joystick Right [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_JOYSTICK_RIGHT (HID_USAGE(HID_USAGE_BRAILLE, 0x214))

/* Braille Display Page: Braille D-Pad Center [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_D_PAD_CENTER (HID_USAGE(HID_USAGE_BRAILLE, 0x215))

/* Braille Display Page: Braille D-Pad Up [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_D_PAD_UP (HID_USAGE(HID_USAGE_BRAILLE, 0x216))

/* Braille Display Page: Braille D-Pad Down [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_D_PAD_DOWN (HID_USAGE(HID_USAGE_BRAILLE, 0x217))

/* Braille Display Page: Braille D-Pad Left [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_D_PAD_LEFT (HID_USAGE(HID_USAGE_BRAILLE, 0x218))

/* Braille Display Page: Braille D-Pad Right [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_D_PAD_RIGHT (HID_USAGE(HID_USAGE_BRAILLE, 0x219))

/* Braille Display Page: Braille Pan Left [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_PAN_LEFT (HID_USAGE(HID_USAGE_BRAILLE, 0x21A))

/* Braille Display Page: Braille Pan Right [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_PAN_RIGHT (HID_USAGE(HID_USAGE_BRAILLE, 0x21B))

/* Braille Display Page: Braille Rocker Up [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_ROCKER_UP (HID_USAGE(HID_USAGE_BRAILLE, 0x21C))

/* Braille Display Page: Braille Rocker Down [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_ROCKER_DOWN (HID_USAGE(HID_USAGE_BRAILLE, 0x21D))

/* Braille Display Page: Braille Rocker Press [Sel] */
#define HID_USAGE_BRAILLE_BRAILLE_ROCKER_PRESS (HID_USAGE(HID_USAGE_BRAILLE, 0x21E))

/* Lighting And Illumination Page */
#define HID_USAGE_LIGHT (0x59)

/* Lighting And Illumination Page: Undefined */
#define HID_USAGE_LIGHT_UNDEFINED (HID_USAGE(HID_USAGE_LIGHT, 0x00))

/* Lighting And Illumination Page: Lamp Array [CA] */
#define HID_USAGE_LIGHT_LAMP_ARRAY (HID_USAGE(HID_USAGE_LIGHT, 0x01))

/* Lighting And Illumination Page: Lamp Array Attributes Report [CL] */
#define HID_USAGE_LIGHT_LAMP_ARRAY_ATTRIBUTES_REPORT (HID_USAGE(HID_USAGE_LIGHT, 0x02))

/* Lighting And Illumination Page: Lamp Count [SV, DV] */
#define HID_USAGE_LIGHT_LAMP_COUNT (HID_USAGE(HID_USAGE_LIGHT, 0x03))

/* Lighting And Illumination Page: Bounding Box Width In Micrometers [SV] */
#define HID_USAGE_LIGHT_BOUNDING_BOX_WIDTH_IN_MICROMETERS (HID_USAGE(HID_USAGE_LIGHT, 0x04))

/* Lighting And Illumination Page: Bounding Box Height In Micrometers [SV] */
#define HID_USAGE_LIGHT_BOUNDING_BOX_HEIGHT_IN_MICROMETERS (HID_USAGE(HID_USAGE_LIGHT, 0x05))

/* Lighting And Illumination Page: Bounding Box Depth In Micrometers [SV] */
#define HID_USAGE_LIGHT_BOUNDING_BOX_DEPTH_IN_MICROMETERS (HID_USAGE(HID_USAGE_LIGHT, 0x06))

/* Lighting And Illumination Page: Lamp Array Kind [SV] */
#define HID_USAGE_LIGHT_LAMP_ARRAY_KIND (HID_USAGE(HID_USAGE_LIGHT, 0x07))

/* Lighting And Illumination Page: Min Update Interval In Microseconds [SV] */
#define HID_USAGE_LIGHT_MIN_UPDATE_INTERVAL_IN_MICROSECONDS (HID_USAGE(HID_USAGE_LIGHT, 0x08))

/* Lighting And Illumination Page: Lamp Attributes Request Report [CL] */
#define HID_USAGE_LIGHT_LAMP_ATTRIBUTES_REQUEST_REPORT (HID_USAGE(HID_USAGE_LIGHT, 0x20))

/* Lighting And Illumination Page: Lamp Id [SV, DV] */
#define HID_USAGE_LIGHT_LAMP_ID (HID_USAGE(HID_USAGE_LIGHT, 0x21))

/* Lighting And Illumination Page: Lamp Attributes Response Report [CL] */
#define HID_USAGE_LIGHT_LAMP_ATTRIBUTES_RESPONSE_REPORT (HID_USAGE(HID_USAGE_LIGHT, 0x22))

/* Lighting And Illumination Page: Position X In Micrometers [DV] */
#define HID_USAGE_LIGHT_POSITION_X_IN_MICROMETERS (HID_USAGE(HID_USAGE_LIGHT, 0x23))

/* Lighting And Illumination Page: Position Y In Micrometers [DV] */
#define HID_USAGE_LIGHT_POSITION_Y_IN_MICROMETERS (HID_USAGE(HID_USAGE_LIGHT, 0x24))

/* Lighting And Illumination Page: Position Z In Micrometers [DV] */
#define HID_USAGE_LIGHT_POSITION_Z_IN_MICROMETERS (HID_USAGE(HID_USAGE_LIGHT, 0x25))

/* Lighting And Illumination Page: Lamp Purposes [DV] */
#define HID_USAGE_LIGHT_LAMP_PURPOSES (HID_USAGE(HID_USAGE_LIGHT, 0x26))

/* Lighting And Illumination Page: Update Latency In Microseconds [DV] */
#define HID_USAGE_LIGHT_UPDATE_LATENCY_IN_MICROSECONDS (HID_USAGE(HID_USAGE_LIGHT, 0x27))

/* Lighting And Illumination Page: Red Level Count [DV] */
#define HID_USAGE_LIGHT_RED_LEVEL_COUNT (HID_USAGE(HID_USAGE_LIGHT, 0x28))

/* Lighting And Illumination Page: Green Level Count [DV] */
#define HID_USAGE_LIGHT_GREEN_LEVEL_COUNT (HID_USAGE(HID_USAGE_LIGHT, 0x29))

/* Lighting And Illumination Page: Blue Level Count [DV] */
#define HID_USAGE_LIGHT_BLUE_LEVEL_COUNT (HID_USAGE(HID_USAGE_LIGHT, 0x2A))

/* Lighting And Illumination Page: Intensity Level Count [DV] */
#define HID_USAGE_LIGHT_INTENSITY_LEVEL_COUNT (HID_USAGE(HID_USAGE_LIGHT, 0x2B))

/* Lighting And Illumination Page: Is Programmable [DV] */
#define HID_USAGE_LIGHT_IS_PROGRAMMABLE (HID_USAGE(HID_USAGE_LIGHT, 0x2C))

/* Lighting And Illumination Page: Input Binding [DV] */
#define HID_USAGE_LIGHT_INPUT_BINDING (HID_USAGE(HID_USAGE_LIGHT, 0x2D))

/* Lighting And Illumination Page: Lamp Multi Update Report [CL] */
#define HID_USAGE_LIGHT_LAMP_MULTI_UPDATE_REPORT (HID_USAGE(HID_USAGE_LIGHT, 0x50))

/* Lighting And Illumination Page: Red Update Channel [DV] */
#define HID_USAGE_LIGHT_RED_UPDATE_CHANNEL (HID_USAGE(HID_USAGE_LIGHT, 0x51))

/* Lighting And Illumination Page: Green Update Channel [DV] */
#define HID_USAGE_LIGHT_GREEN_UPDATE_CHANNEL (HID_USAGE(HID_USAGE_LIGHT, 0x52))

/* Lighting And Illumination Page: Blue Update Channel [DV] */
#define HID_USAGE_LIGHT_BLUE_UPDATE_CHANNEL (HID_USAGE(HID_USAGE_LIGHT, 0x53))

/* Lighting And Illumination Page: Intensity Update Channel [DV] */
#define HID_USAGE_LIGHT_INTENSITY_UPDATE_CHANNEL (HID_USAGE(HID_USAGE_LIGHT, 0x54))

/* Lighting And Illumination Page: Lamp Update Flags [DV] */
#define HID_USAGE_LIGHT_LAMP_UPDATE_FLAGS (HID_USAGE(HID_USAGE_LIGHT, 0x55))

/* Lighting And Illumination Page: Lamp Range Update Report [CL] */
#define HID_USAGE_LIGHT_LAMP_RANGE_UPDATE_REPORT (HID_USAGE(HID_USAGE_LIGHT, 0x60))

/* Lighting And Illumination Page: Lamp Id Start [DV] */
#define HID_USAGE_LIGHT_LAMP_ID_START (HID_USAGE(HID_USAGE_LIGHT, 0x61))

/* Lighting And Illumination Page: Lamp Id End [DV] */
#define HID_USAGE_LIGHT_LAMP_ID_END (HID_USAGE(HID_USAGE_LIGHT, 0x62))

/* Lighting And Illumination Page: Lamp Array Control Report [CL] */
#define HID_USAGE_LIGHT_LAMP_ARRAY_CONTROL_REPORT (HID_USAGE(HID_USAGE_LIGHT, 0x70))

/* Lighting And Illumination Page: Autonomous Mode [DV] */
#define HID_USAGE_LIGHT_AUTONOMOUS_MODE (HID_USAGE(HID_USAGE_LIGHT, 0x71))

/* USB Monitor Page */
#define HID_USAGE_MONITOR (0x80)

/* USB Monitor Page: Monitor Control */
#define HID_USAGE_MONITOR_MONITOR_CONTROL (HID_USAGE(HID_USAGE_MONITOR, 0x01))

/* USB Monitor Page: EDID Information */
#define HID_USAGE_MONITOR_EDID_INFORMATION (HID_USAGE(HID_USAGE_MONITOR, 0x02))

/* USB Monitor Page: VDIF Information */
#define HID_USAGE_MONITOR_VDIF_INFORMATION (HID_USAGE(HID_USAGE_MONITOR, 0x03))

/* USB Monitor Page: VESA Version */
#define HID_USAGE_MONITOR_VESA_VERSION (HID_USAGE(HID_USAGE_MONITOR, 0x04))

/* VESA Virtual Control Page */
#define HID_USAGE_MONITOR_VESA (0x82)

/* VESA Virtual Control Page: Brightness */
#define HID_USAGE_MONITOR_VESA_BRIGHTNESS (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x10))

/* VESA Virtual Control Page: Contrast */
#define HID_USAGE_MONITOR_VESA_CONTRAST (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x12))

/* VESA Virtual Control Page: Red Video Gain */
#define HID_USAGE_MONITOR_VESA_RED_VIDEO_GAIN (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x16))

/* VESA Virtual Control Page: Green Video Gain */
#define HID_USAGE_MONITOR_VESA_GREEN_VIDEO_GAIN (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x18))

/* VESA Virtual Control Page: Blue Video Gain */
#define HID_USAGE_MONITOR_VESA_BLUE_VIDEO_GAIN (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x1A))

/* VESA Virtual Control Page: Focus */
#define HID_USAGE_MONITOR_VESA_FOCUS (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x1C))

/* VESA Virtual Control Page: Horizontal Position */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_POSITION (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x20))

/* VESA Virtual Control Page: Horizontal Size */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_SIZE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x22))

/* VESA Virtual Control Page: Horizontal Pincushion */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_PINCUSHION (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x24))

/* VESA Virtual Control Page: Horizontal Pincushion Balance */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_PINCUSHION_BALANCE                                       \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x26))

/* VESA Virtual Control Page: Horizontal Misconvergence */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_MISCONVERGENCE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x28))

/* VESA Virtual Control Page: Horizontal Linearity */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_LINEARITY (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x2A))

/* VESA Virtual Control Page: Horizontal Linearity Balance */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_LINEARITY_BALANCE                                        \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x2C))

/* VESA Virtual Control Page: Vertical Position */
#define HID_USAGE_MONITOR_VESA_VERTICAL_POSITION (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x30))

/* VESA Virtual Control Page: Vertical Size */
#define HID_USAGE_MONITOR_VESA_VERTICAL_SIZE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x32))

/* VESA Virtual Control Page: Vertical Pincushion */
#define HID_USAGE_MONITOR_VESA_VERTICAL_PINCUSHION (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x34))

/* VESA Virtual Control Page: Vertical Pincushion Balance */
#define HID_USAGE_MONITOR_VESA_VERTICAL_PINCUSHION_BALANCE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x36))

/* VESA Virtual Control Page: Vertical Misconvergence */
#define HID_USAGE_MONITOR_VESA_VERTICAL_MISCONVERGENCE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x38))

/* VESA Virtual Control Page: Vertical Linearity */
#define HID_USAGE_MONITOR_VESA_VERTICAL_LINEARITY (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x3A))

/* VESA Virtual Control Page: Vertical Linearity Balance */
#define HID_USAGE_MONITOR_VESA_VERTICAL_LINEARITY_BALANCE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x3C))

/* VESA Virtual Control Page: Parallelogram Distortion (Key Balance) */
#define HID_USAGE_MONITOR_VESA_PARALLELOGRAM_DISTORTION_KEY_BALANCE                                \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x40))

/* VESA Virtual Control Page: Trapezoidal Distortion (Key) */
#define HID_USAGE_MONITOR_VESA_TRAPEZOIDAL_DISTORTION_KEY (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x42))

/* VESA Virtual Control Page: Tilt (Rotation) */
#define HID_USAGE_MONITOR_VESA_TILT_ROTATION (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x44))

/* VESA Virtual Control Page: Top Corner Distortion Control */
#define HID_USAGE_MONITOR_VESA_TOP_CORNER_DISTORTION_CONTROL                                       \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x46))

/* VESA Virtual Control Page: Top Corner Distortion Balance */
#define HID_USAGE_MONITOR_VESA_TOP_CORNER_DISTORTION_BALANCE                                       \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x48))

/* VESA Virtual Control Page: Bottom Corner Distortion Control */
#define HID_USAGE_MONITOR_VESA_BOTTOM_CORNER_DISTORTION_CONTROL                                    \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x4A))

/* VESA Virtual Control Page: Bottom Corner Distortion Balance */
#define HID_USAGE_MONITOR_VESA_BOTTOM_CORNER_DISTORTION_BALANCE                                    \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x4C))

/* VESA Virtual Control Page: Horizontal Moiré */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_MOIR (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x56))

/* VESA Virtual Control Page: Vertical Moiré */
#define HID_USAGE_MONITOR_VESA_VERTICAL_MOIR (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x58))

/* VESA Virtual Control Page: Red Video Black Level */
#define HID_USAGE_MONITOR_VESA_RED_VIDEO_BLACK_LEVEL (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x6C))

/* VESA Virtual Control Page: Green Video Black Level */
#define HID_USAGE_MONITOR_VESA_GREEN_VIDEO_BLACK_LEVEL (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x6E))

/* VESA Virtual Control Page: Blue Video Black Level */
#define HID_USAGE_MONITOR_VESA_BLUE_VIDEO_BLACK_LEVEL (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x70))

/* VESA Virtual Control Page: Input Level Select */
#define HID_USAGE_MONITOR_VESA_INPUT_LEVEL_SELECT (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x5E))

/* VESA Virtual Control Page: Input Source Select */
#define HID_USAGE_MONITOR_VESA_INPUT_SOURCE_SELECT (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x60))

/* VESA Virtual Control Page: On Screen Display */
#define HID_USAGE_MONITOR_VESA_ON_SCREEN_DISPLAY (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xCA))

/* VESA Virtual Control Page: StereoMode */
#define HID_USAGE_MONITOR_VESA_STEREOMODE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xD4))

/* VESA Virtual Control Page: Auto Size Center */
#define HID_USAGE_MONITOR_VESA_AUTO_SIZE_CENTER (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xA2))

/* VESA Virtual Control Page: Polarity Horizontal Synchronization */
#define HID_USAGE_MONITOR_VESA_POLARITY_HORIZONTAL_SYNCHRONIZATION                                 \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xA4))

/* VESA Virtual Control Page: Polarity Vertical Synchronization */
#define HID_USAGE_MONITOR_VESA_POLARITY_VERTICAL_SYNCHRONIZATION                                   \
    (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xA6))

/* VESA Virtual Control Page: Synchronization Type */
#define HID_USAGE_MONITOR_VESA_SYNCHRONIZATION_TYPE (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xA8))

/* VESA Virtual Control Page: Screen Orientation */
#define HID_USAGE_MONITOR_VESA_SCREEN_ORIENTATION (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xAA))

/* VESA Virtual Control Page: Horizontal Frequency */
#define HID_USAGE_MONITOR_VESA_HORIZONTAL_FREQUENCY (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xAC))

/* VESA Virtual Control Page: Vertical Frequency */
#define HID_USAGE_MONITOR_VESA_VERTICAL_FREQUENCY (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xAE))

/* VESA Virtual Control Page: Degauss */
#define HID_USAGE_MONITOR_VESA_DEGAUSS (HID_USAGE(HID_USAGE_MONITOR_VESA, 0x01))

/* VESA Virtual Control Page: Settings */
#define HID_USAGE_MONITOR_VESA_SETTINGS (HID_USAGE(HID_USAGE_MONITOR_VESA, 0xB0))

/* Bar Code Scanner Page */
#define HID_USAGE_POS_BARCODE (0x8C)

/* Bar Code Scanner Page: Undefined */
#define HID_USAGE_POS_BARCODE_UNDEFINED (HID_USAGE(HID_USAGE_POS_BARCODE, 0x00))

/* Bar Code Scanner Page: Bar Code Badge Reader [CA] */
#define HID_USAGE_POS_BARCODE_BAR_CODE_BADGE_READER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x01))

/* Bar Code Scanner Page: Bar Code Scanner [CA] */
#define HID_USAGE_POS_BARCODE_BAR_CODE_SCANNER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x02))

/* Bar Code Scanner Page: Dumb Bar Code Scanner [CA] */
#define HID_USAGE_POS_BARCODE_DUMB_BAR_CODE_SCANNER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x03))

/* Bar Code Scanner Page: Cordless Scanner Base [CA] */
#define HID_USAGE_POS_BARCODE_CORDLESS_SCANNER_BASE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x04))

/* Bar Code Scanner Page: Bar Code Scanner Cradle [CA] */
#define HID_USAGE_POS_BARCODE_BAR_CODE_SCANNER_CRADLE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x05))

/* Bar Code Scanner Page: Attribute Report [CL] */
#define HID_USAGE_POS_BARCODE_ATTRIBUTE_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x10))

/* Bar Code Scanner Page: Settings Report [CL] */
#define HID_USAGE_POS_BARCODE_SETTINGS_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x11))

/* Bar Code Scanner Page: Scanned Data Report [CL] */
#define HID_USAGE_POS_BARCODE_SCANNED_DATA_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x12))

/* Bar Code Scanner Page: Raw Scanned Data Report [CL] */
#define HID_USAGE_POS_BARCODE_RAW_SCANNED_DATA_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x13))

/* Bar Code Scanner Page: Trigger Report [CL] */
#define HID_USAGE_POS_BARCODE_TRIGGER_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x14))

/* Bar Code Scanner Page: Status Report [CL] */
#define HID_USAGE_POS_BARCODE_STATUS_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x15))

/* Bar Code Scanner Page: UPC/EAN Control Report [CL] */
#define HID_USAGE_POS_BARCODE_UPC_EAN_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x16))

/* Bar Code Scanner Page: EAN 2/3 Label Control Report [CL] */
#define HID_USAGE_POS_BARCODE_EAN_2_3_LABEL_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x17))

/* Bar Code Scanner Page: Code 39 Control Report [CL] */
#define HID_USAGE_POS_BARCODE_CODE_39_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x18))

/* Bar Code Scanner Page: Interleaved 2 of 5 Control Report [CL] */
#define HID_USAGE_POS_BARCODE_INTERLEAVED_2_OF_5_CONTROL_REPORT                                    \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x19))

/* Bar Code Scanner Page: Standard 2 of 5 Control Report [CL] */
#define HID_USAGE_POS_BARCODE_STANDARD_2_OF_5_CONTROL_REPORT                                       \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x1A))

/* Bar Code Scanner Page: MSI Plessey Control Report [CL] */
#define HID_USAGE_POS_BARCODE_MSI_PLESSEY_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x1B))

/* Bar Code Scanner Page: Codabar Control Report [CL] */
#define HID_USAGE_POS_BARCODE_CODABAR_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x1C))

/* Bar Code Scanner Page: Code 128 Control Report [CL] */
#define HID_USAGE_POS_BARCODE_CODE_128_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x1D))

/* Bar Code Scanner Page: Misc 1D Control Report [CL] */
#define HID_USAGE_POS_BARCODE_MISC_1D_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x1E))

/* Bar Code Scanner Page: 2D Control Report [CL] */
#define HID_USAGE_POS_BARCODE_2D_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x1F))

/* Bar Code Scanner Page: Aiming/Pointer Mode [SF] */
#define HID_USAGE_POS_BARCODE_AIMING_POINTER_MODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x30))

/* Bar Code Scanner Page: Bar Code Present Sensor [SF] */
#define HID_USAGE_POS_BARCODE_BAR_CODE_PRESENT_SENSOR (HID_USAGE(HID_USAGE_POS_BARCODE, 0x31))

/* Bar Code Scanner Page: Class 1A Laser [SF] */
#define HID_USAGE_POS_BARCODE_CLASS_1A_LASER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x32))

/* Bar Code Scanner Page: Class 2 Laser [SF] */
#define HID_USAGE_POS_BARCODE_CLASS_2_LASER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x33))

/* Bar Code Scanner Page: Heater Present [SF] */
#define HID_USAGE_POS_BARCODE_HEATER_PRESENT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x34))

/* Bar Code Scanner Page: Contact Scanner [SF] */
#define HID_USAGE_POS_BARCODE_CONTACT_SCANNER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x35))

/* Bar Code Scanner Page: Electronic Article Surveillance Notification [SF] */
#define HID_USAGE_POS_BARCODE_ELECTRONIC_ARTICLE_SURVEILLANCE_NOTIFICATION                         \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x36))

/* Bar Code Scanner Page: Constant Electronic Article Surveillance [SF] */
#define HID_USAGE_POS_BARCODE_CONSTANT_ELECTRONIC_ARTICLE_SURVEILLANCE                             \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x37))

/* Bar Code Scanner Page: Error Indication [SF] */
#define HID_USAGE_POS_BARCODE_ERROR_INDICATION (HID_USAGE(HID_USAGE_POS_BARCODE, 0x38))

/* Bar Code Scanner Page: Fixed Beeper [SF] */
#define HID_USAGE_POS_BARCODE_FIXED_BEEPER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x39))

/* Bar Code Scanner Page: Good Decode Indication [SF] */
#define HID_USAGE_POS_BARCODE_GOOD_DECODE_INDICATION (HID_USAGE(HID_USAGE_POS_BARCODE, 0x3A))

/* Bar Code Scanner Page: Hands Free Scanning [SF] */
#define HID_USAGE_POS_BARCODE_HANDS_FREE_SCANNING (HID_USAGE(HID_USAGE_POS_BARCODE, 0x3B))

/* Bar Code Scanner Page: Intrinsically Safe [SF] */
#define HID_USAGE_POS_BARCODE_INTRINSICALLY_SAFE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x3C))

/* Bar Code Scanner Page: Klasse Eins Laser [SF] */
#define HID_USAGE_POS_BARCODE_KLASSE_EINS_LASER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x3D))

/* Bar Code Scanner Page: Long Range Scanner [SF] */
#define HID_USAGE_POS_BARCODE_LONG_RANGE_SCANNER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x3E))

/* Bar Code Scanner Page: Mirror Speed Control [SF] */
#define HID_USAGE_POS_BARCODE_MIRROR_SPEED_CONTROL (HID_USAGE(HID_USAGE_POS_BARCODE, 0x3F))

/* Bar Code Scanner Page: Not On File Indication [SF] */
#define HID_USAGE_POS_BARCODE_NOT_ON_FILE_INDICATION (HID_USAGE(HID_USAGE_POS_BARCODE, 0x40))

/* Bar Code Scanner Page: Programmable Beeper [SF] */
#define HID_USAGE_POS_BARCODE_PROGRAMMABLE_BEEPER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x41))

/* Bar Code Scanner Page: Triggerless [SF] */
#define HID_USAGE_POS_BARCODE_TRIGGERLESS (HID_USAGE(HID_USAGE_POS_BARCODE, 0x42))

/* Bar Code Scanner Page: Wand [SF] */
#define HID_USAGE_POS_BARCODE_WAND (HID_USAGE(HID_USAGE_POS_BARCODE, 0x43))

/* Bar Code Scanner Page: Water Resistant [SF] */
#define HID_USAGE_POS_BARCODE_WATER_RESISTANT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x44))

/* Bar Code Scanner Page: Multi-Range Scanner [SF] */
#define HID_USAGE_POS_BARCODE_MULTI_RANGE_SCANNER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x45))

/* Bar Code Scanner Page: Proximity Sensor [SF] */
#define HID_USAGE_POS_BARCODE_PROXIMITY_SENSOR (HID_USAGE(HID_USAGE_POS_BARCODE, 0x46))

/* Bar Code Scanner Page: Fragment Decoding [DF] */
#define HID_USAGE_POS_BARCODE_FRAGMENT_DECODING (HID_USAGE(HID_USAGE_POS_BARCODE, 0x4D))

/* Bar Code Scanner Page: Scanner Read Confidence [DV] */
#define HID_USAGE_POS_BARCODE_SCANNER_READ_CONFIDENCE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x4E))

/* Bar Code Scanner Page: Data Prefix [NAry] */
#define HID_USAGE_POS_BARCODE_DATA_PREFIX (HID_USAGE(HID_USAGE_POS_BARCODE, 0x4F))

/* Bar Code Scanner Page: Prefix AIMI [SEL] */
#define HID_USAGE_POS_BARCODE_PREFIX_AIMI (HID_USAGE(HID_USAGE_POS_BARCODE, 0x50))

/* Bar Code Scanner Page: Prefix None [SEL] */
#define HID_USAGE_POS_BARCODE_PREFIX_NONE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x51))

/* Bar Code Scanner Page: Prefix Proprietary [SEL] */
#define HID_USAGE_POS_BARCODE_PREFIX_PROPRIETARY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x52))

/* Bar Code Scanner Page: Active Time [DV] */
#define HID_USAGE_POS_BARCODE_ACTIVE_TIME (HID_USAGE(HID_USAGE_POS_BARCODE, 0x55))

/* Bar Code Scanner Page: Aiming Laser Pattern [DF] */
#define HID_USAGE_POS_BARCODE_AIMING_LASER_PATTERN (HID_USAGE(HID_USAGE_POS_BARCODE, 0x56))

/* Bar Code Scanner Page: Bar Code Present [OOC] */
#define HID_USAGE_POS_BARCODE_BAR_CODE_PRESENT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x57))

/* Bar Code Scanner Page: Beeper State [OOC] */
#define HID_USAGE_POS_BARCODE_BEEPER_STATE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x58))

/* Bar Code Scanner Page: Laser On Time [DV] */
#define HID_USAGE_POS_BARCODE_LASER_ON_TIME (HID_USAGE(HID_USAGE_POS_BARCODE, 0x59))

/* Bar Code Scanner Page: Laser State [OOC] */
#define HID_USAGE_POS_BARCODE_LASER_STATE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x5A))

/* Bar Code Scanner Page: Lockout Time [DV] */
#define HID_USAGE_POS_BARCODE_LOCKOUT_TIME (HID_USAGE(HID_USAGE_POS_BARCODE, 0x5B))

/* Bar Code Scanner Page: Motor State [OOC] */
#define HID_USAGE_POS_BARCODE_MOTOR_STATE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x5C))

/* Bar Code Scanner Page: Motor Timeout [DV] */
#define HID_USAGE_POS_BARCODE_MOTOR_TIMEOUT (HID_USAGE(HID_USAGE_POS_BARCODE, 0x5D))

/* Bar Code Scanner Page: Power On Reset Scanner [DF] */
#define HID_USAGE_POS_BARCODE_POWER_ON_RESET_SCANNER (HID_USAGE(HID_USAGE_POS_BARCODE, 0x5E))

/* Bar Code Scanner Page: Prevent Read of Barcodes [DF] */
#define HID_USAGE_POS_BARCODE_PREVENT_READ_OF_BARCODES (HID_USAGE(HID_USAGE_POS_BARCODE, 0x5F))

/* Bar Code Scanner Page: Initiate Barcode Read [DF] */
#define HID_USAGE_POS_BARCODE_INITIATE_BARCODE_READ (HID_USAGE(HID_USAGE_POS_BARCODE, 0x60))

/* Bar Code Scanner Page: Trigger State [OOC] */
#define HID_USAGE_POS_BARCODE_TRIGGER_STATE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x61))

/* Bar Code Scanner Page: Trigger Mode [NAry] */
#define HID_USAGE_POS_BARCODE_TRIGGER_MODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x62))

/* Bar Code Scanner Page: Trigger Mode Blinking Laser On [SEL] */
#define HID_USAGE_POS_BARCODE_TRIGGER_MODE_BLINKING_LASER_ON                                       \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x63))

/* Bar Code Scanner Page: Trigger Mode Continuous Laser On [SEL] */
#define HID_USAGE_POS_BARCODE_TRIGGER_MODE_CONTINUOUS_LASER_ON                                     \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x64))

/* Bar Code Scanner Page: Trigger Mode Laser on while Pulled [SEL] */
#define HID_USAGE_POS_BARCODE_TRIGGER_MODE_LASER_ON_WHILE_PULLED                                   \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x65))

/* Bar Code Scanner Page: Trigger Mode Laser stays on after Trigger release [SEL] */
#define HID_USAGE_POS_BARCODE_TRIGGER_MODE_LASER_STAYS_ON_AFTER_TRIGGER_RELEASE                    \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x66))

/* Bar Code Scanner Page: Commit Parameters to NVM [DF] */
#define HID_USAGE_POS_BARCODE_COMMIT_PARAMETERS_TO_NVM (HID_USAGE(HID_USAGE_POS_BARCODE, 0x6D))

/* Bar Code Scanner Page: Parameter Scanning [DF] */
#define HID_USAGE_POS_BARCODE_PARAMETER_SCANNING (HID_USAGE(HID_USAGE_POS_BARCODE, 0x6E))

/* Bar Code Scanner Page: Parameters Changed [OOC] */
#define HID_USAGE_POS_BARCODE_PARAMETERS_CHANGED (HID_USAGE(HID_USAGE_POS_BARCODE, 0x6F))

/* Bar Code Scanner Page: Set parameter default values [DF] */
#define HID_USAGE_POS_BARCODE_SET_PARAMETER_DEFAULT_VALUES (HID_USAGE(HID_USAGE_POS_BARCODE, 0x70))

/* Bar Code Scanner Page: Scanner In Cradle [OOC] */
#define HID_USAGE_POS_BARCODE_SCANNER_IN_CRADLE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x75))

/* Bar Code Scanner Page: Scanner In Range [OOC] */
#define HID_USAGE_POS_BARCODE_SCANNER_IN_RANGE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x76))

/* Bar Code Scanner Page: Aim Duration [DV] */
#define HID_USAGE_POS_BARCODE_AIM_DURATION (HID_USAGE(HID_USAGE_POS_BARCODE, 0x7A))

/* Bar Code Scanner Page: Good Read Lamp Duration [DV] */
#define HID_USAGE_POS_BARCODE_GOOD_READ_LAMP_DURATION (HID_USAGE(HID_USAGE_POS_BARCODE, 0x7B))

/* Bar Code Scanner Page: Good Read Lamp Intensity [DV] */
#define HID_USAGE_POS_BARCODE_GOOD_READ_LAMP_INTENSITY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x7C))

/* Bar Code Scanner Page: Good Read LED [DF] */
#define HID_USAGE_POS_BARCODE_GOOD_READ_LED (HID_USAGE(HID_USAGE_POS_BARCODE, 0x7D))

/* Bar Code Scanner Page: Good Read Tone Frequency [DV] */
#define HID_USAGE_POS_BARCODE_GOOD_READ_TONE_FREQUENCY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x7E))

/* Bar Code Scanner Page: Good Read Tone Length [DV] */
#define HID_USAGE_POS_BARCODE_GOOD_READ_TONE_LENGTH (HID_USAGE(HID_USAGE_POS_BARCODE, 0x7F))

/* Bar Code Scanner Page: Good Read Tone Volume [DV] */
#define HID_USAGE_POS_BARCODE_GOOD_READ_TONE_VOLUME (HID_USAGE(HID_USAGE_POS_BARCODE, 0x80))

/* Bar Code Scanner Page: No Read Message [DF] */
#define HID_USAGE_POS_BARCODE_NO_READ_MESSAGE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x82))

/* Bar Code Scanner Page: Not on File Volume [DV] */
#define HID_USAGE_POS_BARCODE_NOT_ON_FILE_VOLUME (HID_USAGE(HID_USAGE_POS_BARCODE, 0x83))

/* Bar Code Scanner Page: Powerup Beep [DF] */
#define HID_USAGE_POS_BARCODE_POWERUP_BEEP (HID_USAGE(HID_USAGE_POS_BARCODE, 0x84))

/* Bar Code Scanner Page: Sound Error Beep [DF] */
#define HID_USAGE_POS_BARCODE_SOUND_ERROR_BEEP (HID_USAGE(HID_USAGE_POS_BARCODE, 0x85))

/* Bar Code Scanner Page: Sound Good Read Beep [DF] */
#define HID_USAGE_POS_BARCODE_SOUND_GOOD_READ_BEEP (HID_USAGE(HID_USAGE_POS_BARCODE, 0x86))

/* Bar Code Scanner Page: Sound Not On File Beep [DF] */
#define HID_USAGE_POS_BARCODE_SOUND_NOT_ON_FILE_BEEP (HID_USAGE(HID_USAGE_POS_BARCODE, 0x87))

/* Bar Code Scanner Page: Good Read When to Write [NAry] */
#define HID_USAGE_POS_BARCODE_GOOD_READ_WHEN_TO_WRITE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x88))

/* Bar Code Scanner Page: GRWTI After Decode [SEL] */
#define HID_USAGE_POS_BARCODE_GRWTI_AFTER_DECODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x89))

/* Bar Code Scanner Page: GRWTI Beep/Lamp after transmit [SEL] */
#define HID_USAGE_POS_BARCODE_GRWTI_BEEP_LAMP_AFTER_TRANSMIT                                       \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x8A))

/* Bar Code Scanner Page: GRWTI No Beep/Lamp use at all [SEL] */
#define HID_USAGE_POS_BARCODE_GRWTI_NO_BEEP_LAMP_USE_AT_ALL (HID_USAGE(HID_USAGE_POS_BARCODE, 0x8B))

/* Bar Code Scanner Page: Bookland EAN [DF] */
#define HID_USAGE_POS_BARCODE_BOOKLAND_EAN (HID_USAGE(HID_USAGE_POS_BARCODE, 0x91))

/* Bar Code Scanner Page: Convert EAN 8 to 13 Type [DF] */
#define HID_USAGE_POS_BARCODE_CONVERT_EAN_8_TO_13_TYPE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x92))

/* Bar Code Scanner Page: Convert UPC A to EAN-13 [DF] */
#define HID_USAGE_POS_BARCODE_CONVERT_UPC_A_TO_EAN_13 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x93))

/* Bar Code Scanner Page: Convert UPC-E to A [DF] */
#define HID_USAGE_POS_BARCODE_CONVERT_UPC_E_TO_A (HID_USAGE(HID_USAGE_POS_BARCODE, 0x94))

/* Bar Code Scanner Page: EAN-13 [DF] */
#define HID_USAGE_POS_BARCODE_EAN_13 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x95))

/* Bar Code Scanner Page: EAN-8 [DF] */
#define HID_USAGE_POS_BARCODE_EAN_8 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x96))

/* Bar Code Scanner Page: EAN-99 128_Mandatory [DF] */
#define HID_USAGE_POS_BARCODE_EAN_99_128_MANDATORY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x97))

/* Bar Code Scanner Page: EAN-99 P5/128_Optional [DF] */
#define HID_USAGE_POS_BARCODE_EAN_99_P5_128_OPTIONAL (HID_USAGE(HID_USAGE_POS_BARCODE, 0x98))

/* Bar Code Scanner Page: UPC/EAN [DF] */
#define HID_USAGE_POS_BARCODE_UPC_EAN (HID_USAGE(HID_USAGE_POS_BARCODE, 0x9A))

/* Bar Code Scanner Page: UPC/EAN Coupon Code [DF] */
#define HID_USAGE_POS_BARCODE_UPC_EAN_COUPON_CODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x9B))

/* Bar Code Scanner Page: UPC/EAN Periodicals [DV] */
#define HID_USAGE_POS_BARCODE_UPC_EAN_PERIODICALS (HID_USAGE(HID_USAGE_POS_BARCODE, 0x9C))

/* Bar Code Scanner Page: UPC-A [DF] */
#define HID_USAGE_POS_BARCODE_UPC_A (HID_USAGE(HID_USAGE_POS_BARCODE, 0x9D))

/* Bar Code Scanner Page: UPC-A with 128 Mandatory [DF] */
#define HID_USAGE_POS_BARCODE_UPC_A_WITH_128_MANDATORY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x9E))

/* Bar Code Scanner Page: UPC-A with 128 Optional [DF] */
#define HID_USAGE_POS_BARCODE_UPC_A_WITH_128_OPTIONAL (HID_USAGE(HID_USAGE_POS_BARCODE, 0x9F))

/* Bar Code Scanner Page: UPC-A with P5 Optional [DF] */
#define HID_USAGE_POS_BARCODE_UPC_A_WITH_P5_OPTIONAL (HID_USAGE(HID_USAGE_POS_BARCODE, 0xA0))

/* Bar Code Scanner Page: UPC-E [DF] */
#define HID_USAGE_POS_BARCODE_UPC_E (HID_USAGE(HID_USAGE_POS_BARCODE, 0xA1))

/* Bar Code Scanner Page: UPC-E1 [DF] */
#define HID_USAGE_POS_BARCODE_UPC_E1 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xA2))

/* Bar Code Scanner Page: Periodical [NAry] */
#define HID_USAGE_POS_BARCODE_PERIODICAL (HID_USAGE(HID_USAGE_POS_BARCODE, 0xA9))

/* Bar Code Scanner Page: Periodical Auto-Discriminate + 2 [SEL] */
#define HID_USAGE_POS_BARCODE_PERIODICAL_AUTO_DISCRIMINATE_PLUS_2                                  \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0xAA))

/* Bar Code Scanner Page: Periodical Only Decode with + 2 [SEL] */
#define HID_USAGE_POS_BARCODE_PERIODICAL_ONLY_DECODE_WITH_PLUS_2                                   \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0xAB))

/* Bar Code Scanner Page: Periodical Ignore + 2 [SEL] */
#define HID_USAGE_POS_BARCODE_PERIODICAL_IGNORE_PLUS_2 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xAC))

/* Bar Code Scanner Page: Periodical Auto-Discriminate + 5 [SEL] */
#define HID_USAGE_POS_BARCODE_PERIODICAL_AUTO_DISCRIMINATE_PLUS_5                                  \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0xAD))

/* Bar Code Scanner Page: Periodical Only Decode with + 5 [SEL] */
#define HID_USAGE_POS_BARCODE_PERIODICAL_ONLY_DECODE_WITH_PLUS_5                                   \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0xAE))

/* Bar Code Scanner Page: Periodical Ignore + 5 [SEL] */
#define HID_USAGE_POS_BARCODE_PERIODICAL_IGNORE_PLUS_5 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xAF))

/* Bar Code Scanner Page: Check [NAry] */
#define HID_USAGE_POS_BARCODE_CHECK (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB0))

/* Bar Code Scanner Page: Check Disable Price [SEL] */
#define HID_USAGE_POS_BARCODE_CHECK_DISABLE_PRICE (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB1))

/* Bar Code Scanner Page: Check Enable 4 digit Price [SEL] */
#define HID_USAGE_POS_BARCODE_CHECK_ENABLE_4_DIGIT_PRICE (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB2))

/* Bar Code Scanner Page: Check Enable 5 digit Price [SEL] */
#define HID_USAGE_POS_BARCODE_CHECK_ENABLE_5_DIGIT_PRICE (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB3))

/* Bar Code Scanner Page: Check Enable European 4 digit Price [SEL] */
#define HID_USAGE_POS_BARCODE_CHECK_ENABLE_EUROPEAN_4_DIGIT_PRICE                                  \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB4))

/* Bar Code Scanner Page: Check Enable European 5 digit Price [SEL] */
#define HID_USAGE_POS_BARCODE_CHECK_ENABLE_EUROPEAN_5_DIGIT_PRICE                                  \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB5))

/* Bar Code Scanner Page: EAN Two Label [DF] */
#define HID_USAGE_POS_BARCODE_EAN_TWO_LABEL (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB7))

/* Bar Code Scanner Page: EAN Three Label [DF] */
#define HID_USAGE_POS_BARCODE_EAN_THREE_LABEL (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB8))

/* Bar Code Scanner Page: EAN 8 Flag Digit 1 [DV] */
#define HID_USAGE_POS_BARCODE_EAN_8_FLAG_DIGIT_1 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xB9))

/* Bar Code Scanner Page: EAN 8 Flag Digit 2 [DV] */
#define HID_USAGE_POS_BARCODE_EAN_8_FLAG_DIGIT_2 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xBA))

/* Bar Code Scanner Page: EAN 8 Flag Digit 3 [DV] */
#define HID_USAGE_POS_BARCODE_EAN_8_FLAG_DIGIT_3 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xBB))

/* Bar Code Scanner Page: EAN 13 Flag Digit 1 [DV] */
#define HID_USAGE_POS_BARCODE_EAN_13_FLAG_DIGIT_1 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xBC))

/* Bar Code Scanner Page: EAN 13 Flag Digit 2 [DV] */
#define HID_USAGE_POS_BARCODE_EAN_13_FLAG_DIGIT_2 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xBD))

/* Bar Code Scanner Page: Transmit Check Digit [NAry] */
#define HID_USAGE_POS_BARCODE_TRANSMIT_CHECK_DIGIT (HID_USAGE(HID_USAGE_POS_BARCODE, 0xF0))

/* Bar Code Scanner Page: Disable Check Digit Transmit [SEL] */
#define HID_USAGE_POS_BARCODE_DISABLE_CHECK_DIGIT_TRANSMIT (HID_USAGE(HID_USAGE_POS_BARCODE, 0xF1))

/* Bar Code Scanner Page: Enable Check Digit Transmit [SEL] */
#define HID_USAGE_POS_BARCODE_ENABLE_CHECK_DIGIT_TRANSMIT (HID_USAGE(HID_USAGE_POS_BARCODE, 0xF2))

/* Bar Code Scanner Page: Symbology Identifier 1 [DV] */
#define HID_USAGE_POS_BARCODE_SYMBOLOGY_IDENTIFIER_1 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xFB))

/* Bar Code Scanner Page: Symbology Identifier 2 [DV] */
#define HID_USAGE_POS_BARCODE_SYMBOLOGY_IDENTIFIER_2 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xFC))

/* Bar Code Scanner Page: Symbology Identifier 3 [DV] */
#define HID_USAGE_POS_BARCODE_SYMBOLOGY_IDENTIFIER_3 (HID_USAGE(HID_USAGE_POS_BARCODE, 0xFD))

/* Bar Code Scanner Page: Decoded Data [DV] */
#define HID_USAGE_POS_BARCODE_DECODED_DATA (HID_USAGE(HID_USAGE_POS_BARCODE, 0xFE))

/* Bar Code Scanner Page: Decode Data Continued [DF] */
#define HID_USAGE_POS_BARCODE_DECODE_DATA_CONTINUED (HID_USAGE(HID_USAGE_POS_BARCODE, 0xFF))

/* Bar Code Scanner Page: Bar Space Data [DV] */
#define HID_USAGE_POS_BARCODE_BAR_SPACE_DATA (HID_USAGE(HID_USAGE_POS_BARCODE, 0x100))

/* Bar Code Scanner Page: Scanner Data Accuracy [DV] */
#define HID_USAGE_POS_BARCODE_SCANNER_DATA_ACCURACY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x101))

/* Bar Code Scanner Page: Raw Data Polarity [NAry] */
#define HID_USAGE_POS_BARCODE_RAW_DATA_POLARITY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x102))

/* Bar Code Scanner Page: Polarity Inverted Bar Code [SEL] */
#define HID_USAGE_POS_BARCODE_POLARITY_INVERTED_BAR_CODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x103))

/* Bar Code Scanner Page: Polarity Normal Bar Code [SEL] */
#define HID_USAGE_POS_BARCODE_POLARITY_NORMAL_BAR_CODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x104))

/* Bar Code Scanner Page: Minimum Length to Decode [DV] */
#define HID_USAGE_POS_BARCODE_MINIMUM_LENGTH_TO_DECODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x106))

/* Bar Code Scanner Page: Maximum Length to Decode [DV] */
#define HID_USAGE_POS_BARCODE_MAXIMUM_LENGTH_TO_DECODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x107))

/* Bar Code Scanner Page: First Discrete Length to Decode [DV] */
#define HID_USAGE_POS_BARCODE_FIRST_DISCRETE_LENGTH_TO_DECODE                                      \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x108))

/* Bar Code Scanner Page: Second Discrete Length to Decode [DV] */
#define HID_USAGE_POS_BARCODE_SECOND_DISCRETE_LENGTH_TO_DECODE                                     \
    (HID_USAGE(HID_USAGE_POS_BARCODE, 0x109))

/* Bar Code Scanner Page: Data Length Method [NAry] */
#define HID_USAGE_POS_BARCODE_DATA_LENGTH_METHOD (HID_USAGE(HID_USAGE_POS_BARCODE, 0x10A))

/* Bar Code Scanner Page: DL Method Read any [SEL] */
#define HID_USAGE_POS_BARCODE_DL_METHOD_READ_ANY (HID_USAGE(HID_USAGE_POS_BARCODE, 0x10B))

/* Bar Code Scanner Page: DL Method Check in Range [SEL] */
#define HID_USAGE_POS_BARCODE_DL_METHOD_CHECK_IN_RANGE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x10C))

/* Bar Code Scanner Page: DL Method Check for Discrete [SEL] */
#define HID_USAGE_POS_BARCODE_DL_METHOD_CHECK_FOR_DISCRETE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x10D))

/* Bar Code Scanner Page: Aztec Code [DF] */
#define HID_USAGE_POS_BARCODE_AZTEC_CODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x110))

/* Bar Code Scanner Page: BC412 [DF] */
#define HID_USAGE_POS_BARCODE_BC412 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x111))

/* Bar Code Scanner Page: Channel Code [DF] */
#define HID_USAGE_POS_BARCODE_CHANNEL_CODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x112))

/* Bar Code Scanner Page: Code 16 [DF] */
#define HID_USAGE_POS_BARCODE_CODE_16 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x113))

/* Bar Code Scanner Page: Code 32 [DF] */
#define HID_USAGE_POS_BARCODE_CODE_32 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x114))

/* Bar Code Scanner Page: Code 49 [DF] */
#define HID_USAGE_POS_BARCODE_CODE_49 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x115))

/* Bar Code Scanner Page: Code One [DF] */
#define HID_USAGE_POS_BARCODE_CODE_ONE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x116))

/* Bar Code Scanner Page: Colorcode [DF] */
#define HID_USAGE_POS_BARCODE_COLORCODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x117))

/* Bar Code Scanner Page: Data Matrix [DF] */
#define HID_USAGE_POS_BARCODE_DATA_MATRIX (HID_USAGE(HID_USAGE_POS_BARCODE, 0x118))

/* Bar Code Scanner Page: MaxiCode [DF] */
#define HID_USAGE_POS_BARCODE_MAXICODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x119))

/* Bar Code Scanner Page: MicroPDF [DF] */
#define HID_USAGE_POS_BARCODE_MICROPDF (HID_USAGE(HID_USAGE_POS_BARCODE, 0x11A))

/* Bar Code Scanner Page: PDF-417 [DF] */
#define HID_USAGE_POS_BARCODE_PDF_417 (HID_USAGE(HID_USAGE_POS_BARCODE, 0x11B))

/* Bar Code Scanner Page: PosiCode [DF] */
#define HID_USAGE_POS_BARCODE_POSICODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x11C))

/* Bar Code Scanner Page: QR Code [DF] */
#define HID_USAGE_POS_BARCODE_QR_CODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x11D))

/* Bar Code Scanner Page: SuperCode [DF] */
#define HID_USAGE_POS_BARCODE_SUPERCODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x11E))

/* Bar Code Scanner Page: UltraCode [DF] */
#define HID_USAGE_POS_BARCODE_ULTRACODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x11F))

/* Bar Code Scanner Page: USD-5 (Slug Code) [DF] */
#define HID_USAGE_POS_BARCODE_USD_5_SLUG_CODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x120))

/* Bar Code Scanner Page: VeriCode [DF] */
#define HID_USAGE_POS_BARCODE_VERICODE (HID_USAGE(HID_USAGE_POS_BARCODE, 0x121))

/* Scale Page */
#define HID_USAGE_POS_SCALE (0x8D)

/* Scale Page: Undefined */
#define HID_USAGE_POS_SCALE_UNDEFINED (HID_USAGE(HID_USAGE_POS_SCALE, 0x00))

/* Scale Page: Weighing Device [CA] */
#define HID_USAGE_POS_SCALE_WEIGHING_DEVICE (HID_USAGE(HID_USAGE_POS_SCALE, 0x01))

/* Scale Page: Scale Device [CL] */
#define HID_USAGE_POS_SCALE_SCALE_DEVICE (HID_USAGE(HID_USAGE_POS_SCALE, 0x20))

/* Scale Page: Scale Class I Metric [CL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_I_METRIC (HID_USAGE(HID_USAGE_POS_SCALE, 0x21))

/* Scale Page: Scale Class I Metric (2) [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_I_METRIC_2 (HID_USAGE(HID_USAGE_POS_SCALE, 0x22))

/* Scale Page: Scale Class II Metric [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_II_METRIC (HID_USAGE(HID_USAGE_POS_SCALE, 0x23))

/* Scale Page: Scale Class III Metric [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_III_METRIC (HID_USAGE(HID_USAGE_POS_SCALE, 0x24))

/* Scale Page: Scale Class IIIL Metric [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_IIIL_METRIC (HID_USAGE(HID_USAGE_POS_SCALE, 0x25))

/* Scale Page: Scale Class IV Metric [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_IV_METRIC (HID_USAGE(HID_USAGE_POS_SCALE, 0x26))

/* Scale Page: Scale Class III English [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_III_ENGLISH (HID_USAGE(HID_USAGE_POS_SCALE, 0x27))

/* Scale Page: Scale Class IIIL English [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_IIIL_ENGLISH (HID_USAGE(HID_USAGE_POS_SCALE, 0x28))

/* Scale Page: Scale Class IV English [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_IV_ENGLISH (HID_USAGE(HID_USAGE_POS_SCALE, 0x29))

/* Scale Page: Scale Class Generic [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_CLASS_GENERIC (HID_USAGE(HID_USAGE_POS_SCALE, 0x2A))

/* Scale Page: Scale Attribute Report [CL] */
#define HID_USAGE_POS_SCALE_SCALE_ATTRIBUTE_REPORT (HID_USAGE(HID_USAGE_POS_SCALE, 0x30))

/* Scale Page: Scale Control Report [CL] */
#define HID_USAGE_POS_SCALE_SCALE_CONTROL_REPORT (HID_USAGE(HID_USAGE_POS_SCALE, 0x31))

/* Scale Page: Scale Data Report [CL] */
#define HID_USAGE_POS_SCALE_SCALE_DATA_REPORT (HID_USAGE(HID_USAGE_POS_SCALE, 0x32))

/* Scale Page: Scale Status Report [CL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_REPORT (HID_USAGE(HID_USAGE_POS_SCALE, 0x33))

/* Scale Page: Scale Weight Limit Report [CL] */
#define HID_USAGE_POS_SCALE_SCALE_WEIGHT_LIMIT_REPORT (HID_USAGE(HID_USAGE_POS_SCALE, 0x34))

/* Scale Page: Scale Statistics Report [CL] */
#define HID_USAGE_POS_SCALE_SCALE_STATISTICS_REPORT (HID_USAGE(HID_USAGE_POS_SCALE, 0x35))

/* Scale Page: Data Weight [DV] */
#define HID_USAGE_POS_SCALE_DATA_WEIGHT (HID_USAGE(HID_USAGE_POS_SCALE, 0x40))

/* Scale Page: Data Scaling [CV] */
#define HID_USAGE_POS_SCALE_DATA_SCALING (HID_USAGE(HID_USAGE_POS_SCALE, 0x41))

/* Scale Page: Weight Unit [CL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT (HID_USAGE(HID_USAGE_POS_SCALE, 0x50))

/* Scale Page: Weight Unit Milligram [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_MILLIGRAM (HID_USAGE(HID_USAGE_POS_SCALE, 0x51))

/* Scale Page: Weight Unit Gram [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_GRAM (HID_USAGE(HID_USAGE_POS_SCALE, 0x52))

/* Scale Page: Weight Unit Kilogram [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_KILOGRAM (HID_USAGE(HID_USAGE_POS_SCALE, 0x53))

/* Scale Page: Weight Unit Carats [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_CARATS (HID_USAGE(HID_USAGE_POS_SCALE, 0x54))

/* Scale Page: Weight Unit Taels [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_TAELS (HID_USAGE(HID_USAGE_POS_SCALE, 0x55))

/* Scale Page: Weight Unit Grains [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_GRAINS (HID_USAGE(HID_USAGE_POS_SCALE, 0x56))

/* Scale Page: Weight Unit Pennyweights [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_PENNYWEIGHTS (HID_USAGE(HID_USAGE_POS_SCALE, 0x57))

/* Scale Page: Weight Unit Metric Ton [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_METRIC_TON (HID_USAGE(HID_USAGE_POS_SCALE, 0x58))

/* Scale Page: Weight Unit Avoir Ton [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_AVOIR_TON (HID_USAGE(HID_USAGE_POS_SCALE, 0x59))

/* Scale Page: Weight Unit Troy Ounce [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_TROY_OUNCE (HID_USAGE(HID_USAGE_POS_SCALE, 0x5A))

/* Scale Page: Weight Unit Ounce [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_OUNCE (HID_USAGE(HID_USAGE_POS_SCALE, 0x5B))

/* Scale Page: Weight Unit Pound [SEL] */
#define HID_USAGE_POS_SCALE_WEIGHT_UNIT_POUND (HID_USAGE(HID_USAGE_POS_SCALE, 0x5C))

/* Scale Page: Calibration Count [DV] */
#define HID_USAGE_POS_SCALE_CALIBRATION_COUNT (HID_USAGE(HID_USAGE_POS_SCALE, 0x60))

/* Scale Page: Re-Zero Count [DV] */
#define HID_USAGE_POS_SCALE_RE_ZERO_COUNT (HID_USAGE(HID_USAGE_POS_SCALE, 0x61))

/* Scale Page: Scale Status [CL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS (HID_USAGE(HID_USAGE_POS_SCALE, 0x70))

/* Scale Page: Scale Status Fault [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_FAULT (HID_USAGE(HID_USAGE_POS_SCALE, 0x71))

/* Scale Page: Scale Status Stable at Center of Zero [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_STABLE_AT_CENTER_OF_ZERO                                  \
    (HID_USAGE(HID_USAGE_POS_SCALE, 0x72))

/* Scale Page: Scale Status In Motion [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_IN_MOTION (HID_USAGE(HID_USAGE_POS_SCALE, 0x73))

/* Scale Page: Scale Status Weight Stable [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_WEIGHT_STABLE (HID_USAGE(HID_USAGE_POS_SCALE, 0x74))

/* Scale Page: Scale Status Under Zero [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_UNDER_ZERO (HID_USAGE(HID_USAGE_POS_SCALE, 0x75))

/* Scale Page: Scale Status Over Weight Limit [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_OVER_WEIGHT_LIMIT (HID_USAGE(HID_USAGE_POS_SCALE, 0x76))

/* Scale Page: Scale Status Requires Calibration [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_REQUIRES_CALIBRATION (HID_USAGE(HID_USAGE_POS_SCALE, 0x77))

/* Scale Page: Scale Status Requires Re zeroing [SEL] */
#define HID_USAGE_POS_SCALE_SCALE_STATUS_REQUIRES_RE_ZEROING (HID_USAGE(HID_USAGE_POS_SCALE, 0x78))

/* Scale Page: Zero Scale [OOC] */
#define HID_USAGE_POS_SCALE_ZERO_SCALE (HID_USAGE(HID_USAGE_POS_SCALE, 0x80))

/* Scale Page: Enforced Zero Return [OOC] */
#define HID_USAGE_POS_SCALE_ENFORCED_ZERO_RETURN (HID_USAGE(HID_USAGE_POS_SCALE, 0x81))

/* Magnetic Stripe Reading (MSR) Devices Page */
#define HID_USAGE_POS_MSR (0x8E)

/* Magnetic Stripe Reading (MSR) Devices Page: Undefined */
#define HID_USAGE_POS_MSR_UNDEFINED (HID_USAGE(HID_USAGE_POS_MSR, 0x00))

/* Magnetic Stripe Reading (MSR) Devices Page: MSR Device Read-Only [CA] */
#define HID_USAGE_POS_MSR_MSR_DEVICE_READ_ONLY (HID_USAGE(HID_USAGE_POS_MSR, 0x01))

/* Magnetic Stripe Reading (MSR) Devices Page: Track 1 Length [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_1_LENGTH (HID_USAGE(HID_USAGE_POS_MSR, 0x11))

/* Magnetic Stripe Reading (MSR) Devices Page: Track 2 Length [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_2_LENGTH (HID_USAGE(HID_USAGE_POS_MSR, 0x12))

/* Magnetic Stripe Reading (MSR) Devices Page: Track 3 Length [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_3_LENGTH (HID_USAGE(HID_USAGE_POS_MSR, 0x13))

/* Magnetic Stripe Reading (MSR) Devices Page: Track JIS Length [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_JIS_LENGTH (HID_USAGE(HID_USAGE_POS_MSR, 0x14))

/* Magnetic Stripe Reading (MSR) Devices Page: Track Data [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_DATA (HID_USAGE(HID_USAGE_POS_MSR, 0x20))

/* Magnetic Stripe Reading (MSR) Devices Page: Track 1 Data [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_1_DATA (HID_USAGE(HID_USAGE_POS_MSR, 0x21))

/* Magnetic Stripe Reading (MSR) Devices Page: Track 2 Data [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_2_DATA (HID_USAGE(HID_USAGE_POS_MSR, 0x22))

/* Magnetic Stripe Reading (MSR) Devices Page: Track 3 Data [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_3_DATA (HID_USAGE(HID_USAGE_POS_MSR, 0x23))

/* Magnetic Stripe Reading (MSR) Devices Page: Track JIS Data [SF, DF, SEL] */
#define HID_USAGE_POS_MSR_TRACK_JIS_DATA (HID_USAGE(HID_USAGE_POS_MSR, 0x24))

/* Camera Control Page */
#define HID_USAGE_CAMERA (0x90)

/* Camera Control Page: Undefined */
#define HID_USAGE_CAMERA_UNDEFINED (HID_USAGE(HID_USAGE_CAMERA, 0x00))

/* Camera Control Page: Camera Auto-focus [OSC] */
#define HID_USAGE_CAMERA_CAMERA_AUTO_FOCUS (HID_USAGE(HID_USAGE_CAMERA, 0x20))

/* Camera Control Page: Camera Shutter [OSC] */
#define HID_USAGE_CAMERA_CAMERA_SHUTTER (HID_USAGE(HID_USAGE_CAMERA, 0x21))

/* FIDO Alliance Page */
#define HID_USAGE_FIDO (0xF1D0)

/* FIDO Alliance Page: Undefined */
#define HID_USAGE_FIDO_UNDEFINED (HID_USAGE(HID_USAGE_FIDO, 0x00))

/* FIDO Alliance Page: U2F Authenticator Device [CA] */
#define HID_USAGE_FIDO_U2F_AUTHENTICATOR_DEVICE (HID_USAGE(HID_USAGE_FIDO, 0x01))

/* FIDO Alliance Page: Input Report Data [DV] */
#define HID_USAGE_FIDO_INPUT_REPORT_DATA (HID_USAGE(HID_USAGE_FIDO, 0x20))

/* FIDO Alliance Page: Output Report Data [DV] */
#define HID_USAGE_FIDO_OUTPUT_REPORT_DATA (HID_USAGE(HID_USAGE_FIDO, 0x21))