#pragma once

#include <stdint.h>

// Attempt to include libusb. If the SDK pathing is weird, 
// we might need "orbis/libusb.h" or just "libusb.h". 
// Based on typical OO toolchains:
#include <orbis/libusb.h> 

#define ORBIS_PAD_BUTTON_L3          0x00000002
#define ORBIS_PAD_BUTTON_R3          0x00000004
#define ORBIS_PAD_BUTTON_OPTIONS     0x00000008
#define ORBIS_PAD_BUTTON_UP          0x00000010
#define ORBIS_PAD_BUTTON_RIGHT       0x00000020
#define ORBIS_PAD_BUTTON_DOWN        0x00000040
#define ORBIS_PAD_BUTTON_LEFT        0x00000080
#define ORBIS_PAD_BUTTON_L2          0x00000100
#define ORBIS_PAD_BUTTON_R2          0x00000200
#define ORBIS_PAD_BUTTON_L1          0x00000400
#define ORBIS_PAD_BUTTON_R1          0x00000800
#define ORBIS_PAD_BUTTON_TRIANGLE    0x00001000
#define ORBIS_PAD_BUTTON_CIRCLE      0x00002000
#define ORBIS_PAD_BUTTON_CROSS       0x00004000
#define ORBIS_PAD_BUTTON_SQUARE      0x00008000
#define ORBIS_PAD_BUTTON_TOUCH_PAD   0x00100000
#define ORBIS_PAD_BUTTON_INTERCEPTED 0x80000000

typedef struct ScePadData {
    uint32_t buttons;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t padding[2];
} ScePadData;

typedef struct PS5_Input_Report {
    uint8_t report_id;
    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;
    uint8_t l2_analog;
    uint8_t r2_analog;
    uint8_t seq_number;
    uint8_t btn_shapes;
    uint8_t btn_misc;
    uint8_t btn_system;
} PS5_Input_Report;
