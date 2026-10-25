#pragma once

#include <pu/Plutonium>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include <MainLayout.hpp>
#include <TypingLayout.hpp>
#include <DoneLayout.hpp>
#include <TypingEngine.hpp>

class EzBasicApp : public pu::ui::Application {
    private:
        MainLayout::Ref mainLayout;
        TypingLayout::Ref typingLayout;
        DoneLayout::Ref doneLayout;

        enum class ActiveLayout {
            Main,
            Typing,
            Done
        };
        ActiveLayout currentLayout;

        bool cancelRequested;
        bool typeRequested;

        std::string bootstrapText;
        std::string bootstrapHash;
        std::size_t bootstrapSize;

        ezbasic::TypingEngine engine;
        std::thread typingThread;

        std::atomic<bool> typingActive{false};
        std::atomic<bool> cancelFlag{false};
        std::atomic<std::size_t> linesTyped{0};
        std::atomic<std::size_t> totalLines{0};
        std::atomic<double> elapsedSec{0.0};
        std::atomic<std::size_t> progressLines{0};
        std::atomic<bool> typingFinished{false};
        std::atomic<bool> typingSucceeded{false};
        std::atomic<bool> usbReady{false};
        std::atomic<bool> usbInitStarted{false};
        std::atomic<bool> usbInitFinished{false};
        std::atomic<bool> usbInitSucceeded{false};
        bool usbStatusUpdated = false;

        void TypingWorker();

    public:
        using Application::Application;
        PU_SMART_CTOR(EzBasicApp)
        ~EzBasicApp() override;

        void OnLoad() override;

        void SwitchToMain();
        void SwitchToTyping();
        void SwitchToDone();

        void UpdateTypingProgress(u32 current, u32 total);
        void SetTypingSummary(u32 linesTyped);

        bool IsCancelRequested() const;
        bool IsTypeRequested() const;
        void ResetRequests();

        void SetBootstrap(const std::string& text, const std::string& hash, std::size_t size);
        void PollTyping();
};
