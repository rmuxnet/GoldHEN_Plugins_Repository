#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../include/dualsense.h"

#define DLLEXPORT __declspec(dllexport)
#define DS5_VID 0x054C
#define DS5_PID 0x0CE6
#define DS5_EP_IN 0x81

libusb_context *ctx = NULL;
libusb_device_handle *dev_handle = NULL;
int usb_active = 0;

void* (*original_scePadRead)(int handle, ScePadData* data, int count);

void translate_ps5_to_ps4(const uint8_t* raw_usb_data, ScePadData* ps4_data) {
    if (!raw_usb_data || !ps4_data) return;

    const PS5_Input_Report* ps5 = (const PS5_Input_Report*)raw_usb_data;

    ps4_data->buttons = 0;
    ps4_data->leftStickX  = ps5->left_stick_x;
    ps4_data->leftStickY  = ps5->left_stick_y;
    ps4_data->rightStickX = ps5->right_stick_x;
    ps4_data->rightStickY = ps5->right_stick_y;
    ps4_data->leftTrigger = ps5->l2_analog;
    ps4_data->rightTrigger= ps5->r2_analog;

    if (ps5->btn_shapes & 0x10) ps4_data->buttons |= ORBIS_PAD_BUTTON_SQUARE;
    if (ps5->btn_shapes & 0x20) ps4_data->buttons |= ORBIS_PAD_BUTTON_CROSS;
    if (ps5->btn_shapes & 0x40) ps4_data->buttons |= ORBIS_PAD_BUTTON_CIRCLE;
    if (ps5->btn_shapes & 0x80) ps4_data->buttons |= ORBIS_PAD_BUTTON_TRIANGLE;

    uint8_t dpad = ps5->btn_shapes & 0x0F;
    switch (dpad) {
        case 0: ps4_data->buttons |= ORBIS_PAD_BUTTON_UP; break;
        case 1: ps4_data->buttons |= (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_RIGHT); break;
        case 2: ps4_data->buttons |= ORBIS_PAD_BUTTON_RIGHT; break;
        case 3: ps4_data->buttons |= (ORBIS_PAD_BUTTON_RIGHT | ORBIS_PAD_BUTTON_DOWN); break;
        case 4: ps4_data->buttons |= ORBIS_PAD_BUTTON_DOWN; break;
        case 5: ps4_data->buttons |= (ORBIS_PAD_BUTTON_DOWN | ORBIS_PAD_BUTTON_LEFT); break;
        case 6: ps4_data->buttons |= ORBIS_PAD_BUTTON_LEFT; break;
        case 7: ps4_data->buttons |= (ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_UP); break;
    }

    if (ps5->btn_misc & 0x01) ps4_data->buttons |= ORBIS_PAD_BUTTON_L1;
    if (ps5->btn_misc & 0x02) ps4_data->buttons |= ORBIS_PAD_BUTTON_R1;
    if (ps5->btn_misc & 0x04) ps4_data->buttons |= ORBIS_PAD_BUTTON_L2;
    if (ps5->btn_misc & 0x08) ps4_data->buttons |= ORBIS_PAD_BUTTON_R2;
    if (ps5->btn_misc & 0x20) ps4_data->buttons |= ORBIS_PAD_BUTTON_OPTIONS;
    if (ps5->btn_misc & 0x40) ps4_data->buttons |= ORBIS_PAD_BUTTON_L3;
    if (ps5->btn_misc & 0x80) ps4_data->buttons |= ORBIS_PAD_BUTTON_R3;

    if (ps5->btn_system & 0x02) ps4_data->buttons |= ORBIS_PAD_BUTTON_TOUCH_PAD;
}

void check_usb_device() {
    if (!ctx) libusb_init(&ctx);
    
    if (!dev_handle) {
        dev_handle = libusb_open_device_with_vid_pid(ctx, DS5_VID, DS5_PID);
        if (dev_handle) {
            if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
                libusb_detach_kernel_driver(dev_handle, 0);
            }
            libusb_claim_interface(dev_handle, 0);
            usb_active = 1;
        }
    }
}

extern "C" int hooked_scePadRead(int handle, ScePadData* data, int count) {
    int ret = original_scePadRead(handle, data, count);

    check_usb_device();

    if (usb_active && dev_handle && ret == 0 && data != NULL) {
        uint8_t buffer[64];
        int transferred = 0;
        int r = libusb_interrupt_transfer(dev_handle, DS5_EP_IN, buffer, 64, &transferred, 2);

        if (r == 0 && transferred > 10) {
            if (buffer[0] == 0x01) {
                translate_ps5_to_ps4(buffer, data);
            }
        } else if (r == LIBUSB_ERROR_NO_DEVICE) {
            libusb_close(dev_handle);
            dev_handle = NULL;
            usb_active = 0;
        }
    }
    return ret;
}

extern "C" {
    DLLEXPORT void _init() {
        libusb_init(&ctx);
    }
    
    DLLEXPORT void _fini() {
        if (dev_handle) {
            libusb_release_interface(dev_handle, 0);
            libusb_close(dev_handle);
        }
        if (ctx) libusb_exit(ctx);
    }
}
