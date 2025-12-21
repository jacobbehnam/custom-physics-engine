#include "RawInputFilter.h"
#include <QGuiApplication>
#include <QWindow>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

RawInputFilter::RawInputFilter(MouseCallback callback)
    : mouseCallback(std::move(callback)) {}

bool RawInputFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef _WIN32
    if (eventType != "windows_generic_MSG")
        return false;

    MSG* msg = static_cast<MSG*>(message);

    if (!initialized && msg->hwnd) {
        registerRawInput(msg->hwnd);
        initialized = true;
    }

    if (msg->message == WM_INPUT) {
        UINT dataSize = 0;
        GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
        std::vector<BYTE> buffer(dataSize);
        if (GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
            return false;

        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (raw->header.dwType == RIM_TYPEMOUSE) {
            if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
                int dx = raw->data.mouse.lLastX;
                int dy = raw->data.mouse.lLastY;
                if (mouseCallback) mouseCallback(dx, dy);
            }
        }
    }
    return false;
#elif defined(__linux__)
    // TODO: Use X11 XInput2 raw events or evdev (Linux-specific)
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)
    return false;
#elif defined(__APPLE__)
    // TODO: Use CGEventTap to intercept mouse motion on macOS
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)
    return false;
#else
    return false;
#endif
}

#ifdef _WIN32
void RawInputFilter::registerRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; 
    rid.usUsage     = 0x02; 
    rid.dwFlags     = RIDEV_INPUTSINK;
    rid.hwndTarget  = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        std::cerr << "RawInputFilter: Failed to register raw input device\n";
    } else {
        std::cout << "RawInputFilter: Registered raw mouse input on window handle" << std::endl;
    }
}
#endif

