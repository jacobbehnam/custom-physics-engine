#pragma once

#include <QAbstractNativeEventFilter>
#include <QWindow>
#include <functional>
#include <thread>
#include <atomic>

class RawInputFilter : public QAbstractNativeEventFilter {
public:
    using MouseCallback = std::function<void(int dx, int dy)>;

    explicit RawInputFilter(MouseCallback callback);
    ~RawInputFilter() override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

    // For non-Windows platforms: call from mouseMoveEvent
    void handleMouseMove(int dx, int dy);

private:
    bool initialized = false;

#ifdef _WIN32
    void registerRawInput(HWND hwnd);
#elif defined(__linux__)
    void registerRawInput();
    int xi_opcode = -1;
#endif

    std::thread x11Thread;
    std::atomic<bool> runX11Thread{true};

    MouseCallback mouseCallback;
};

