#include "RawInputFilter.h"
#include <QGuiApplication>
#include <QWindow>
#include <vector>
#include <iostream>
#include <QMetaObject>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <sys/select.h>
#include <unistd.h>
#endif

RawInputFilter::RawInputFilter(MouseCallback callback)
    : mouseCallback(std::move(callback)) {}

RawInputFilter::~RawInputFilter() {
#ifdef __linux__
    runX11Thread = false;
    if (x11Thread.joinable()) {
        x11Thread.join();
    }
#endif
}

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
    if (!initialized) {
        registerRawInput();
        initialized = true;
    }
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
#elif defined(__linux__)
void RawInputFilter::registerRawInput() {
    x11Thread = std::thread([this]() {
        Display* dpy = XOpenDisplay(nullptr);
        if (!dpy) return;

        int event, error;
        if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
            std::cerr << "RawInputFilter: XInput extension not available.\n";
            XCloseDisplay(dpy);
            return;
        }

        Window root = DefaultRootWindow(dpy);
        XIEventMask evmask;
        unsigned char mask[(XI_LASTEVENT + 7)/8] = { 0 };

        evmask.deviceid = XIAllMasterDevices;
        evmask.mask_len = sizeof(mask);
        evmask.mask = mask;
        XISetMask(mask, XI_RawMotion);
        
        XISelectEvents(dpy, root, &evmask, 1);
        XSync(dpy, False);
        
        int fd = ConnectionNumber(dpy);
        fd_set in_fds;
        struct timeval tv;

        while (runX11Thread) {
            FD_ZERO(&in_fds);
            FD_SET(fd, &in_fds);
            tv.tv_sec = 0;
            tv.tv_usec = 50000; // 50ms timeout

            if (select(fd + 1, &in_fds, nullptr, nullptr, &tv) > 0) {
                while (XPending(dpy)) {
                    XEvent ev;
                    XNextEvent(dpy, &ev);
                    
                    if (ev.xcookie.type == GenericEvent && ev.xcookie.extension == xi_opcode) {
                        if (XGetEventData(dpy, &ev.xcookie)) {
                            if (ev.xcookie.evtype == XI_RawMotion) {
                                XIRawEvent* raw = (XIRawEvent*)ev.xcookie.data;
                                double dx = 0.0, dy = 0.0;
                                double* values = raw->raw_values;
                                
                                if (XIMaskIsSet(raw->valuators.mask, 0)) {
                                    dx = *values;
                                    values++;
                                }
                                if (XIMaskIsSet(raw->valuators.mask, 1)) {
                                    dy = *values;
                                }
                                
                                if (mouseCallback && (dx != 0.0 || dy != 0.0)) {
                                    int idx = static_cast<int>(dx);
                                    int idy = static_cast<int>(dy);
                                    QMetaObject::invokeMethod(qApp, [this, idx, idy]() {
                                        mouseCallback(idx, idy);
                                    }, Qt::QueuedConnection);
                                }
                            }
                            XFreeEventData(dpy, &ev.xcookie);
                        }
                    }
                }
            }
        }
        XCloseDisplay(dpy);
    });
    std::cout << "RawInputFilter: Registered XInput2 raw mouse on background thread.\n";
}
#endif

