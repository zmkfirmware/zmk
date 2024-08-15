/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/dt-bindings/dt-util.h>

// NOTE MIDI octave index is +2 vs "normal" octave index
//  so standard C3 is MIDI C5 here

// All NOTE_* have the identical keycode and midi code
#define NOTE_C 0x0
#define NOTE_Cs 0x1
#define NOTE_Db NOTE_Cs
#define NOTE_D 0x2
#define NOTE_Ds 0x3
#define NOTE_Eb NOTE_Ds
#define NOTE_E 0x4
#define NOTE_F 0x5
#define NOTE_Fs 0x6
#define NOTE_Gb NOTE_Fs
#define NOTE_G 0x7
#define NOTE_Gs 0x8
#define NOTE_Ab NOTE_Gs
#define NOTE_A 0x9
#define NOTE_As 0xa
#define NOTE_Bb NOTE_As
#define NOTE_B 0xb

#define NOTE_C_1 0xc
#define NOTE_Cs_1 0xd
#define NOTE_Db_1 NOTE_Cs_1
#define NOTE_D_1 0xe
#define NOTE_Ds_1 0xf
#define NOTE_Eb_1 NOTE_Ds_1
#define NOTE_E_1 0x10
#define NOTE_F_1 0x11
#define NOTE_Fs_1 0x12
#define NOTE_Gb_1 NOTE_Fs_1
#define NOTE_G_1 0x13
#define NOTE_Gs_1 0x14
#define NOTE_Ab_1 NOTE_Gs_1
#define NOTE_A_1 0x15
#define NOTE_As_1 0x16
#define NOTE_Bb_1 NOTE_As_1
#define NOTE_B_1 0x17

#define NOTE_C_2 0x18
#define NOTE_Cs_2 0x19
#define NOTE_Db_2 NOTE_Cs_2
#define NOTE_D_2 0x1a
#define NOTE_Ds_2 0x1b
#define NOTE_Eb_2 NOTE_Ds_2
#define NOTE_E_2 0x1c
#define NOTE_F_2 0x1d
#define NOTE_Fs_2 0x1e
#define NOTE_Gb_2 NOTE_Fs_2
#define NOTE_G_2 0x1f
#define NOTE_Gs_2 0x20
#define NOTE_Ab_2 NOTE_Gs_2
#define NOTE_A_2 0x21
#define NOTE_As_2 0x22
#define NOTE_Bb_2 NOTE_As_2
#define NOTE_B_2 0x23

#define NOTE_C_3 0x24
#define NOTE_Cs_3 0x25
#define NOTE_Db_3 NOTE_Cs_3
#define NOTE_D_3 0x26
#define NOTE_Ds_3 0x27
#define NOTE_Eb_3 NOTE_Ds_3
#define NOTE_E_3 0x28
#define NOTE_F_3 0x29
#define NOTE_Fs_3 0x2a
#define NOTE_Gb_3 NOTE_Fs_3
#define NOTE_G_3 0x2b
#define NOTE_Gs_3 0x2c
#define NOTE_Ab_3 NOTE_Gs_3
#define NOTE_A_3 0x2d
#define NOTE_As_3 0x2e
#define NOTE_Bb_3 NOTE_As_3
#define NOTE_B_3 0x2f

#define NOTE_C_4 0x30
#define NOTE_Cs_4 0x31
#define NOTE_Db_4 NOTE_Cs_4
#define NOTE_D_4 0x32
#define NOTE_Ds_4 0x33
#define NOTE_Eb_4 NOTE_Ds_4
#define NOTE_E_4 0x34
#define NOTE_F_4 0x35
#define NOTE_Fs_4 0x36
#define NOTE_Gb_4 NOTE_Fs_4
#define NOTE_G_4 0x37
#define NOTE_Gs_4 0x38
#define NOTE_Ab_4 NOTE_Gs_4
#define NOTE_A_4 0x39
#define NOTE_As_4 0x3a
#define NOTE_Bb_4 NOTE_As_4
#define NOTE_B_4 0x3b

#define NOTE_C_5 0x3c
#define NOTE_Cs_5 0x3d
#define NOTE_Db_5 NOTE_Cs_5
#define NOTE_D_5 0x3e
#define NOTE_Ds_5 0x3f
#define NOTE_Eb_5 NOTE_Ds_5
#define NOTE_E_5 0x40
#define NOTE_F_5 0x41
#define NOTE_Fs_5 0x42
#define NOTE_Gb_5 NOTE_Fs_5
#define NOTE_G_5 0x43
#define NOTE_Gs_5 0x44
#define NOTE_Ab_5 NOTE_Gs_5
#define NOTE_A_5 0x45
#define NOTE_As_5 0x46
#define NOTE_Bb_5 NOTE_As_5
#define NOTE_B_5 0x47

#define NOTE_C_6 0x48
#define NOTE_Cs_6 0x49
#define NOTE_Db_6 NOTE_Cs_6
#define NOTE_D_6 0x4a
#define NOTE_Ds_6 0x4b
#define NOTE_Eb_6 NOTE_Ds_6
#define NOTE_E_6 0x4c
#define NOTE_F_6 0x4d
#define NOTE_Fs_6 0x4e
#define NOTE_Gb_6 NOTE_Fs_6
#define NOTE_G_6 0x4f
#define NOTE_Gs_6 0x50
#define NOTE_Ab_6 NOTE_Gs_6
#define NOTE_A_6 0x51
#define NOTE_As_6 0x52
#define NOTE_Bb_6 NOTE_As_6
#define NOTE_B_6 0x53

#define NOTE_C_7 0x54
#define NOTE_Cs_7 0x55
#define NOTE_Db_7 NOTE_Cs_7
#define NOTE_D_7 0x56
#define NOTE_Ds_7 0x57
#define NOTE_Eb_7 NOTE_Ds_7
#define NOTE_E_7 0x58
#define NOTE_F_7 0x59
#define NOTE_Fs_7 0x5a
#define NOTE_Gb_7 NOTE_Fs_7
#define NOTE_G_7 0x5b
#define NOTE_Gs_7 0x5c
#define NOTE_Ab_7 NOTE_Gs_7
#define NOTE_A_7 0x5d
#define NOTE_As_7 0x5e
#define NOTE_Bb_7 NOTE_As_7
#define NOTE_B_7 0x5f

#define NOTE_C_8 0x60
#define NOTE_Cs_8 0x61
#define NOTE_Db_8 NOTE_Cs_8
#define NOTE_D_8 0x62
#define NOTE_Ds_8 0x63
#define NOTE_Eb_8 NOTE_Ds_8
#define NOTE_E_8 0x64
#define NOTE_F_8 0x65
#define NOTE_Fs_8 0x66
#define NOTE_Gb_8 NOTE_Fs_8
#define NOTE_G_8 0x67
#define NOTE_Gs_8 0x68
#define NOTE_Ab_8 NOTE_Gs_8
#define NOTE_A_8 0x69
#define NOTE_As_8 0x6a
#define NOTE_Bb_8 NOTE_As_8
#define NOTE_B_8 0x6b

#define NOTE_C_9 0x6c
#define NOTE_Cs_9 0x6d
#define NOTE_Db_9 NOTE_Cs_9
#define NOTE_D_9 0x6e
#define NOTE_Ds_9 0x6f
#define NOTE_Eb_9 NOTE_Ds_9
#define NOTE_E_9 0x70
#define NOTE_F_9 0x71
#define NOTE_Fs_9 0x72
#define NOTE_Gb_9 NOTE_Fs_9
#define NOTE_G_9 0x73
#define NOTE_Gs_9 0x74
#define NOTE_Ab_9 NOTE_Gs_9
#define NOTE_A_9 0x75
#define NOTE_As_9 0x76
#define NOTE_Bb_9 NOTE_As_9
#define NOTE_B_9 0x77

#define NOTE_C_10 0x78
#define NOTE_Cs_10 0x79
#define NOTE_Db_10 NOTE_Cs_10
#define NOTE_D_10 0x7a
#define NOTE_Ds_10 0x7b
#define NOTE_Eb_10 NOTE_Ds_10
#define NOTE_E_10 0x7c
#define NOTE_F_10 0x7d
#define NOTE_Fs_10 0x7e
#define NOTE_Gb_10 NOTE_Fs_10
#define NOTE_G_10 0x7f
// 0x7f aka 127 is the max value

// NOTE sentinals
#define MIDI_MIN_NOTE NOTE_C
#define MIDI_MAX_NOTE NOTE_G_10
#define MIDI_INVALID 0xFF

// Midi control change keycodes
// appended with 0xB0
#define SUSTAIN 0xB040
#define PORTAMENTO 0xB041
#define SOSTENUTO 0xB042
#define OCT_UP 0xB081
#define OCT_DOWN 0xB082

// midi control sentinals
#define MIDI_MIN_CONTROL SUSTAIN
#define MIDI_MAX_CONTROL OCT_DOWN
