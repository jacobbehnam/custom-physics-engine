#pragma once

#include <QAbstractNativeEventFilter>
#include <functional>
#include <windows.h>

class RawInputFilter : public QAbstractNativeEventFilter {
public:
    // Callback type: dx, dy raw mouse movement deltas
    using MouseCallback = std::function<void(int dx, int dy)>;

    // Constructor: provide a callback to receive raw deltas
    explicit RawInputFilter(MouseCallback callback);

    // QAbstractNativeEventFilter override
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    // Registers for raw mouse input on the given window handle
    void registerRawInput(HWND hwnd);

    MouseCallback mouseCallback;
    bool initialized = false;
};