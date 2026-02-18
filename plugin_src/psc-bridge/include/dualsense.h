#pragma once

#include <stdint.h>
#include <orbis/Usbd.h> 

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

// Correct SDK Structure Definitions
typedef struct ScePadAnalogStick {
    uint8_t x;
    uint8_t y;
} ScePadAnalogStick;

typedef struct ScePadAnalogButtons {
    uint8_t l2;
    uint8_t r2;
    uint8_t padding[2];
} ScePadAnalogButtons;

typedef struct SceFQuaternion {
    float x, y, z, w;
} SceFQuaternion;

typedef struct SceFVector3 {
    float x, y, z;
} SceFVector3;

typedef struct ScePadTouch {
    uint16_t x;
    uint16_t y;
    uint8_t id;
    uint8_t reserve[3];
} ScePadTouch;

typedef struct ScePadTouchData {
    uint8_t touchNum;
    uint8_t reserve[3];
    uint32_t reserve1;
    ScePadTouch touch[2];
} ScePadTouchData;

typedef struct ScePadExtensionUnitData {
    uint32_t extensionUnitId;
    uint8_t reserve[1];
    uint8_t dataLength;
    uint8_t data[10];
} ScePadExtensionUnitData;

typedef struct ScePadData {
    uint32_t buttons;
    ScePadAnalogStick leftStick;
    ScePadAnalogStick rightStick;
    ScePadAnalogButtons analogButtons;
    SceFQuaternion orientation;
    SceFVector3 acceleration;
    SceFVector3 angularVelocity;
    ScePadTouchData touchData;
    int connected;
    uint64_t timestamp;
    ScePadExtensionUnitData extensionUnitData;
    uint8_t connectedCount;
    uint8_t reserve[2];
    uint8_t deviceUniqueDataLen;
    uint8_t deviceUniqueData[12];
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
