#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../include/dualsense.h"

// Include common headers for GoldHEN compatibility
#include "plugin_common.h"

// --- Metadata for GoldHEN ---
attr_public const char *g_pluginName = "dualsense_bridge";
attr_public const char *g_pluginDesc = "PS5 Controller Support";
attr_public const char *g_pluginAuth = "rmuxnet";
attr_public uint32_t g_pluginVersion = 0x00000100; // 1.00

#define DS5_VID 0x054C
#define DS5_PID 0x0CE6
#define DS5_EP_IN 0x81

libusb_context *ctx = NULL;
libusb_device_handle *dev_handle = NULL;
int usb_active = 0;

// Function pointer for the original pad read
void* (*original_scePadRead)(int handle, ScePadData* data, int count);

void translate_ps5_to_ps4(const uint8_t* raw_usb_data, ScePadData* ps4_data) {
    if (!raw_usb_data || !ps4_data) return;

    const PS5_Input_Report* ps5 = (const PS5_Input_Report*)raw_usb_data;

    // Reset standard buttons
    ps4_data->buttons = 0;

    // Direct Analog mapping
    ps4_data->leftStickX  = ps5->left_stick_x;
    ps4_data->leftStickY  = ps5->left_stick_y;
    ps4_data->rightStickX = ps5->right_stick_x;
    ps4_data->rightStickY = ps5->right_stick_y;
    ps4_data->leftTrigger = ps5->l2_analog;
    ps4_data->rightTrigger= ps5->r2_analog;

    // Shapes
    if (ps5->btn_shapes & 0x10) ps4_data->buttons |= ORBIS_PAD_BUTTON_SQUARE;
    if (ps5->btn_shapes & 0x20) ps4_data->buttons |= ORBIS_PAD_BUTTON_CROSS;
    if (ps5->btn_shapes & 0x40) ps4_data->buttons |= ORBIS_PAD_BUTTON_CIRCLE;
    if (ps5->btn_shapes & 0x80) ps4_data->buttons |= ORBIS_PAD_BUTTON_TRIANGLE;

    // D-Pad
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

    // Misc
    if (ps5->btn_misc & 0x01) ps4_data->buttons |= ORBIS_PAD_BUTTON_L1;
    if (ps5->btn_misc & 0x02) ps4_data->buttons |= ORBIS_PAD_BUTTON_R1;
    if (ps5->btn_misc & 0x04) ps4_data->buttons |= ORBIS_PAD_BUTTON_L2;
    if (ps5->btn_misc & 0x08) ps4_data->buttons |= ORBIS_PAD_BUTTON_R2;
    if (ps5->btn_misc & 0x20) ps4_data->buttons |= ORBIS_PAD_BUTTON_OPTIONS;
    if (ps5->btn_misc & 0x40) ps4_data->buttons |= ORBIS_PAD_BUTTON_L3;
    if (ps5->btn_misc & 0x80) ps4_data->buttons |= ORBIS_PAD_BUTTON_R3;

    // System
    if (ps5->btn_system & 0x02) ps4_data->buttons |= ORBIS_PAD_BUTTON_TOUCH_PAD;
}

void check_usb_device() {
    if (!ctx) {
        // Use sceUsbdInit instead of libusb_init
        int r = sceUsbdInit(&ctx);
        if (r < 0) return;
    }
    
    if (!dev_handle) {
        // Use sceUsbdOpenDeviceWithVidPid
        dev_handle = sceUsbdOpenDeviceWithVidPid(ctx, DS5_VID, DS5_PID);
        if (dev_handle) {
            // Use sceUsbdKernelDriverActive / sceUsbdDetachKernelDriver
            if (sceUsbdKernelDriverActive(dev_handle, 0) == 1) {
                sceUsbdDetachKernelDriver(dev_handle, 0);
            }
            sceUsbdClaimInterface(dev_handle, 0);
            usb_active = 1;
        }
    }
}

// The Hooked Function
int hooked_scePadRead(int handle, ScePadData* data, int count) {
    // 1. Call original
    int ret = 0;
    if (original_scePadRead) {
        ret = original_scePadRead(handle, data, count);
    }

    // 2. Override with USB data if available
    check_usb_device();

    if (usb_active && dev_handle && ret == 0 && data != NULL) {
        uint8_t buffer[64];
        int transferred = 0;
        
        // Use sceUsbdInterruptTransfer
        int r = sceUsbdInterruptTransfer(dev_handle, DS5_EP_IN, buffer, 64, &transferred, 2);

        if (r == 0 && transferred > 10 && buffer[0] == 0x01) {
            translate_ps5_to_ps4(buffer, data);
        } else if (r == LIBUSB_ERROR_NO_DEVICE) {
            sceUsbdClose(dev_handle);
            dev_handle = NULL;
            usb_active = 0;
        }
    }
    return ret;
}

// --- Standard Plugin Exports ---

int32_t attr_public plugin_load(int32_t argc, const char* argv[]) {
    // Notify user on load
    char msg[128] = {0};
    snprintf(msg, sizeof(msg), "DualSense Bridge Loaded");
    NotifyStatic(TEX_ICON_SYSTEM, msg);
    
    // Initialize libusb context using native API
    sceUsbdInit(&ctx);
    
    // TODO: Register hook here via GoldHEN SDK API if dynamic hooking is supported,
    // otherwise this relies on static detours setup elsewhere in the SDK.
    // Example: DetourFunction("libScePad.sprx", "scePadRead", hooked_scePadRead, &original_scePadRead);
    
    return 0;
}

int32_t attr_public plugin_unload(int32_t argc, const char* argv[]) {
    if (dev_handle) {
        sceUsbdReleaseInterface(dev_handle, 0);
        sceUsbdClose(dev_handle);
    }
    if (ctx) sceUsbdExit(ctx);
    
    return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args) {
    return 0;
}

s32 attr_module_hidden module_stop(s64 argc, const void *args) {
    return 0;
}
