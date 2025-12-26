#pragma once

#include <QAbstractNativeEventFilter>
#include <QWindow>
#include <functional>

class RawInputFilter : public QAbstractNativeEventFilter {
public:
    using MouseCallback = std::function<void(int dx, int dy)>;

    explicit RawInputFilter(MouseCallback callback);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

    // For non-Windows platforms: call from mouseMoveEvent
    void handleMouseMove(int dx, int dy);

private:
#ifdef _WIN32
    void registerRawInput(HWND hwnd);
    bool initialized = false;
#endif

    MouseCallback mouseCallback;
};

