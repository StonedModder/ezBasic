#pragma once
#include <functional>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace ezbasic {

class TypingEngine {
public:
    TypingEngine();
    ~TypingEngine();

    // Initialize usbDs and expose a HID keyboard interface. This does not wait
    // for host enumeration; call Poll() from the render loop until it returns true.
    bool Init();

    // Service USB state and control-transfer events. Returns true only after
    // the host configures the device and acknowledges the HID report descriptor.
    bool Poll();
    bool IsInitialized() const;
    bool IsReady() const;

    // Clean up usbDs state.
    void Exit();

    // Verify that every character in text is typable.
    // Returns true if valid; if false and failOffset is non-null, sets it.
    bool VerifyText(const char* text, size_t len,
                    size_t* outStrokes = nullptr,
                    size_t* outLines = nullptr,
                    size_t* failOffset = nullptr);

    // Type text via USB HID keyboard.
    // progressCb is invoked after each completed line: (linesTyped, totalLines).
    // cancelFlag is polled between keystrokes; typing aborts when true.
    bool TypeText(const char* text, size_t len,
                  std::function<void(size_t lines, size_t totalLines)> progressCb,
                  std::atomic<bool>& cancelFlag);

    void SetDelayMs(int delayMs) { m_delayMs = delayMs; }
    int GetDelayMs() const { return m_delayMs; }

private:
    struct State;
    State* m_state;
    int m_delayMs;

    bool SendHidReport(uint8_t mods, uint8_t key);
    void HandleSetupPacket(uint64_t nowMs);
    bool QueueControlIn(const void* data, size_t size, uint16_t requested, uint64_t nowMs);
    bool QueueControlOutStatus(uint64_t nowMs);
    void StallControl();
    void CancelPendingTransfer();
};

} // namespace ezbasic
