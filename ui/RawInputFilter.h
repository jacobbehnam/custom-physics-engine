#pragma once

#include <QAbstractNativeEventFilter>
#include <functional>
#include <windows.h>

class RawInputFilter : public QAbstractNativeEventFilter {
public:
    using MouseCallback = std::function<void(int dx, int dy)>;

    explicit RawInputFilter(MouseCallback callback);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void registerRawInput(HWND hwnd);

    MouseCallback mouseCallback;
    bool initialized = false;
};