#include "RawInputFilter.h"
#include <QGuiApplication>
#include <QWindow>
#include <vector>
#include <iostream>

RawInputFilter::RawInputFilter(MouseCallback callback)
    : mouseCallback(std::move(callback)) {}

bool RawInputFilter::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    // Only handle Windows generic messages
    if (eventType != "windows_generic_MSG")
        return false;

    MSG* msg = static_cast<MSG*>(message);

    // On first message with a valid hwnd, register for raw input
    if (!initialized && msg->hwnd) {
        registerRawInput(msg->hwnd);
        initialized = true;
    }

    if (msg->message == WM_INPUT) {
        // Determine size of raw data
        UINT dataSize = 0;
        GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
        std::vector<BYTE> buffer(dataSize);
        if (GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
            return false;

        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (raw->header.dwType == RIM_TYPEMOUSE) {
            // Ignore absolute movements
            if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
                int dx = raw->data.mouse.lLastX;
                int dy = raw->data.mouse.lLastY;
                // Invoke user callback
                if (mouseCallback) mouseCallback(dx, dy);
            }
        }
    }

    return false; // allow other handlers to process
}

void RawInputFilter::registerRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; // Generic desktop controls
    rid.usUsage     = 0x02; // Mouse
    rid.dwFlags     = RIDEV_INPUTSINK;; // receive even when not focused
    rid.hwndTarget  = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        std::cerr << "RawInputFilter: Failed to register raw input device\n";
    } else {
        std::cout << "RawInputFilter: Registered raw mouse input on window handle" << std::endl;
    }
}
