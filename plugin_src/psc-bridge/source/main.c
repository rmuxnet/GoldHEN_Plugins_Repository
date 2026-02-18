#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <orbis/libkernel.h>
#include "../include/dualsense.h"
#include <Detour.h>
#include "plugin_common.h"

void* DetourFunction(const char* library, const char* symbol, void* hook, void** original) {
    int handle = 0;
    void* func_ptr = NULL;

    if (sys_dynlib_load_prx(library, &handle) == 0 || handle != 0) {
        if (sys_dynlib_dlsym(handle, symbol, &func_ptr) == 0 && func_ptr != NULL) {
            
            Detour* detour = (Detour*)malloc(sizeof(Detour));
            if (!detour) return NULL;
            
            Detour_Construct(detour, DetourMode_x64);
            
            void* result = Detour_DetourFunction(detour, (uint64_t)func_ptr, hook);
            
            if (original) {
                *original = result;
            }
            
            return result;
        }
    }
    return NULL;
}

attr_public const char *g_pluginName = "dualsense_bridge";
attr_public const char *g_pluginDesc = "PS5 Controller Support";
attr_public const char *g_pluginAuth = "rmuxnet";
attr_public uint32_t g_pluginVersion = 0x00000100;

#define DS5_VID 0x054C
#define DS5_PID 0x0CE6
#define DS5_EP_IN 0x81
#define DS5_EP_OUT 0x02

struct libusb_context *ctx = NULL;
libusb_device_handle *dev_handle = NULL;
int usb_active = 0;
OrbisPthread keepalive_thread_id;
int keepalive_running = 0;

int (*original_scePadRead)(int handle, ScePadData* data, int count);

void translate_ps5_to_ps4(const uint8_t* raw_usb_data, ScePadData* ps4_data) {
    if (!raw_usb_data || !ps4_data) return;

    const PS5_Input_Report* ps5 = (const PS5_Input_Report*)raw_usb_data;

    // Preserve existing buttons from original read if desired, but usually we overwrite for player 1
    ps4_data->buttons = 0;

    ps4_data->leftStick.x  = ps5->left_stick_x;
    ps4_data->leftStick.y  = ps5->left_stick_y;
    ps4_data->rightStick.x = ps5->right_stick_x;
    ps4_data->rightStick.y = ps5->right_stick_y;
    ps4_data->analogButtons.l2 = ps5->l2_analog;
    ps4_data->analogButtons.r2 = ps5->r2_analog;

    // Ensure connected flag is set
    ps4_data->connected = 1;

    // Byte [08]: Shapes & Dpad
    // bit 4: Square (0x10)
    // bit 5: Cross (0x20)
    // bit 6: Circle (0x40)
    // bit 7: Triangle (0x80)
    if (ps5->btn_shapes & 0x10) ps4_data->buttons |= ORBIS_PAD_BUTTON_SQUARE;
    if (ps5->btn_shapes & 0x20) ps4_data->buttons |= ORBIS_PAD_BUTTON_CROSS;
    if (ps5->btn_shapes & 0x40) ps4_data->buttons |= ORBIS_PAD_BUTTON_CIRCLE;
    if (ps5->btn_shapes & 0x80) ps4_data->buttons |= ORBIS_PAD_BUTTON_TRIANGLE;

    // D-Pad is the lower 4 bits of Byte [08]
    // 0=Up, 1=UpRight, 2=Right, 3=DownRight, 4=Down, 5=DownLeft, 6=Left, 7=UpLeft, 8=Released
    uint8_t dpad = ps5->btn_shapes & 0x0F;
    if (dpad == 0) ps4_data->buttons |= ORBIS_PAD_BUTTON_UP;
    else if (dpad == 1) ps4_data->buttons |= (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_RIGHT);
    else if (dpad == 2) ps4_data->buttons |= ORBIS_PAD_BUTTON_RIGHT;
    else if (dpad == 3) ps4_data->buttons |= (ORBIS_PAD_BUTTON_RIGHT | ORBIS_PAD_BUTTON_DOWN);
    else if (dpad == 4) ps4_data->buttons |= ORBIS_PAD_BUTTON_DOWN;
    else if (dpad == 5) ps4_data->buttons |= (ORBIS_PAD_BUTTON_DOWN | ORBIS_PAD_BUTTON_LEFT);
    else if (dpad == 6) ps4_data->buttons |= ORBIS_PAD_BUTTON_LEFT;
    else if (dpad == 7) ps4_data->buttons |= (ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_UP);

    // Byte [09]: Misc Buttons
    // bit 0: L1
    // bit 1: R1
    // bit 2: L2 (Digital)
    // bit 3: R2 (Digital)
    // bit 4: Create/Share (0x10) -> Maps to SHARE? Usually mapping to TOUCHPAD on PS4 if SHARE isn't standard
    // bit 5: Options (0x20)
    // bit 6: L3 (0x40)
    // bit 7: R3 (0x80)

    if (ps5->btn_misc & 0x01) ps4_data->buttons |= ORBIS_PAD_BUTTON_L1;
    if (ps5->btn_misc & 0x02) ps4_data->buttons |= ORBIS_PAD_BUTTON_R1;
    if (ps5->btn_misc & 0x04) ps4_data->buttons |= ORBIS_PAD_BUTTON_L2;
    if (ps5->btn_misc & 0x08) ps4_data->buttons |= ORBIS_PAD_BUTTON_R2;
    
    // Create/Share button (0x10) -> often mapped to "Share" on PS4, but internal flag is usually just labeled OPTIONS/TOUCH
    // Attempting to map Share to INTERCEPTED or specific system button if needed, but standard games use OPTIONS mostly.
    // NOTE: PS4 SDK/Homebrew usually doesn't expose a dedicated "Share" bit in the standard button mask easily, 
    // it's often handled by system. Mapping to nothing or finding specific bit.
    
    if (ps5->btn_misc & 0x20) ps4_data->buttons |= ORBIS_PAD_BUTTON_OPTIONS;
    if (ps5->btn_misc & 0x40) ps4_data->buttons |= ORBIS_PAD_BUTTON_L3;
    if (ps5->btn_misc & 0x80) ps4_data->buttons |= ORBIS_PAD_BUTTON_R3;

    // Byte [10]: System Buttons
    // bit 0: PS Button (0x01) -> ORBIS_PAD_BUTTON_INTERCEPTED usually acts as PS Home
    // bit 1: Touchpad Click (0x02) -> ORBIS_PAD_BUTTON_TOUCH_PAD
    // bit 2: Mute
    
    if (ps5->btn_system & 0x01) ps4_data->buttons |= ORBIS_PAD_BUTTON_INTERCEPTED; // PS Home
    if (ps5->btn_system & 0x02) ps4_data->buttons |= ORBIS_PAD_BUTTON_TOUCH_PAD;  // Touch Click (Pad Press)
}

void send_keepalive_packet() {
    if (!dev_handle || !usb_active) return;

    // Standard Output Report 0x02
    uint8_t output_report[48];
    memset(output_report, 0, sizeof(output_report));
    
    output_report[0] = 0x02; // Report ID
    output_report[1] = 0xFF; // Flags (Update everything)
    output_report[2] = 0x55; // Standard flag
    
    int transferred = 0;
    sceUsbdInterruptTransfer(dev_handle, DS5_EP_OUT, output_report, sizeof(output_report), &transferred, 100);
}

struct libusb_device_handle* find_dualsense(struct libusb_context* context) {
    struct libusb_device **devs;
    struct libusb_device_handle *handle = NULL;
    struct libusb_device_descriptor desc;
    ssize_t cnt;
    
    cnt = sceUsbdGetDeviceList(&devs);
    if (cnt < 0) return NULL;

    for (ssize_t i = 0; i < cnt; i++) {
        if (sceUsbdGetDeviceDescriptor(devs[i], &desc) < 0) continue;

        if (desc.idVendor == DS5_VID && desc.idProduct == DS5_PID) {
            int r = sceUsbdOpen(devs[i], &handle);
            if (r == 0 && handle) {
                break;
            }
        }
    }

    sceUsbdFreeDeviceList(devs);
    return handle;
}

void* keepalive_thread_func(void* arg) {
    while (keepalive_running) {
        if (!ctx) {
             sceUsbdInit(&ctx);
        }

        if (!dev_handle && ctx) {
            dev_handle = find_dualsense(ctx);
            if (dev_handle) {
                // Connection event
                NotifyStatic(TEX_ICON_SYSTEM, "DualSense Connected");
                usb_active = 1;
                send_keepalive_packet();
            }
        }

        if (dev_handle) {
            send_keepalive_packet();
        }
        sceKernelUsleep(2000000); // Sleep 2 seconds
    }
    return NULL;
}

void check_usb_device() {
    // Legacy hook point, now handled by thread
}

int hooked_scePadRead(int handle, ScePadData* data, int count) {
    int ret = 0;
    if (original_scePadRead) {
        ret = original_scePadRead(handle, data, count);
    }

    if (usb_active && dev_handle && ret == 0 && data != NULL) {
        uint8_t buffer[64];
        int transferred = 0;
        
        int r = sceUsbdInterruptTransfer(dev_handle, DS5_EP_IN, buffer, sizeof(buffer), &transferred, 2);

        if (r == 0 && transferred > 10) {
            if (buffer[0] == 0x01) {
                translate_ps5_to_ps4(buffer, data);
            }
        } else if (r < 0 && r != -99) { 
             if (r != -7) { 
                 NotifyStatic(TEX_ICON_SYSTEM, "DualSense Disconnected");
                 sceUsbdReleaseInterface(dev_handle, 0);
                 sceUsbdClose(dev_handle);
                 dev_handle = NULL;
                 usb_active = 0;
             }
        }
    }
    return ret;
}

int32_t attr_public plugin_load(int32_t argc, const char* argv[]) {
    char msg[128] = {0};
    snprintf(msg, sizeof(msg), "DualSense Bridge Loaded");
    NotifyStatic(TEX_ICON_SYSTEM, msg);
    
    sceUsbdInit(&ctx);
    
    keepalive_running = 1;
    scePthreadCreate(&keepalive_thread_id, NULL, keepalive_thread_func, NULL, "ds_keepalive");
    
    DetourFunction("libScePad.sprx", "scePadRead", (void*)hooked_scePadRead, (void**)&original_scePadRead);
    
    return 0;
}

int32_t attr_public plugin_unload(int32_t argc, const char* argv[]) {
    keepalive_running = 0;
    scePthreadJoin(keepalive_thread_id, NULL);

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
