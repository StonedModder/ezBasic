#include <EzBasicApp.hpp>
#include <Utils.hpp>

EzBasicApp::~EzBasicApp() {
    this->cancelFlag.store(true, std::memory_order_release);
    if(this->typingThread.joinable()) {
        this->typingThread.join();
    }
    this->engine.Exit();
}

void EzBasicApp::OnLoad() {
    this->cancelRequested = false;
    this->typeRequested = false;
    this->currentLayout = ActiveLayout::Main;
    this->typingActive.store(false, std::memory_order_release);
    this->cancelFlag.store(false, std::memory_order_release);
    this->linesTyped.store(0, std::memory_order_relaxed);
    this->totalLines.store(0, std::memory_order_relaxed);
    this->elapsedSec.store(0.0, std::memory_order_relaxed);
    this->progressLines.store(0, std::memory_order_relaxed);
    this->usbReady.store(false, std::memory_order_release);
    this->usbInitStarted.store(false, std::memory_order_release);
    this->usbInitFinished.store(false, std::memory_order_release);
    this->usbInitSucceeded.store(false, std::memory_order_release);
    this->usbStatusUpdated = false;

    this->mainLayout = MainLayout::New();
    this->typingLayout = TypingLayout::New();
    this->doneLayout = DoneLayout::New();

    this->LoadLayout(this->mainLayout);
    this->mainLayout->SetSubtitle("Starting USB keyboard...");

    // Initialization only registers descriptors and enables usb:ds; it does
    // not wait for a host. Poll the protocol from the render loop until Plume's
    // descriptor-acknowledgement readiness condition is met.
    this->AddRenderCallback([this]() {
        if(!this->usbInitStarted.exchange(true, std::memory_order_acq_rel)) {
            const bool initialized = this->engine.Init();
            this->usbInitSucceeded.store(initialized, std::memory_order_release);
            this->usbInitFinished.store(true, std::memory_order_release);
        }

        if(!this->usbInitFinished.load(std::memory_order_acquire)) return;
        if(!this->usbInitSucceeded.load(std::memory_order_acquire)) {
            this->usbReady.store(false, std::memory_order_release);
            if(!this->usbStatusUpdated) {
                this->mainLayout->SetSubtitle("USB setup failed. Close other USB apps and relaunch ezBasic.");
                this->usbStatusUpdated = true;
            }
            return;
        }

        // TypeText pumps USB events on its own worker while sending reports.
        if(this->typingActive.load(std::memory_order_acquire)) return;

        const bool wasReady = this->usbReady.load(std::memory_order_acquire);
        const bool ready = this->engine.Poll();
        this->usbReady.store(ready, std::memory_order_release);
        if(ready != wasReady) {
            this->mainLayout->SetSubtitle(ready
                ? "USB ready. Press A to type."
                : "USB host disconnected. Waiting for connection...");
            this->usbStatusUpdated = true;
        } else if(!ready && !this->usbStatusUpdated) {
            this->mainLayout->SetSubtitle("Waiting for USB host and HID setup...");
            this->usbStatusUpdated = true;
        }
    });

    this->SetOnInput([&](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        (void)keys_up;
        (void)keys_held;
        (void)touch_pos;

        this->PollTyping();

        if(keys_down & HidNpadButton_Plus) {
            this->cancelFlag.store(true, std::memory_order_release);
            if(this->typingThread.joinable()) {
                this->typingThread.join();
            }
            this->Close();
        }
        else if(keys_down & HidNpadButton_A) {
            if(this->currentLayout == ActiveLayout::Done) {
                this->SwitchToMain();
            }
            else if(this->usbReady.load(std::memory_order_acquire)
                    && !this->typingActive.load(std::memory_order_acquire)) {
                this->typeRequested = true;
            } else if(!this->usbInitFinished.load(std::memory_order_acquire)) {
                this->mainLayout->SetSubtitle("Starting USB keyboard. Please wait.");
            } else if(!this->usbInitSucceeded.load(std::memory_order_acquire)) {
                this->mainLayout->SetSubtitle("USB setup failed. Close other USB apps and relaunch ezBasic.");
            } else {
                this->mainLayout->SetSubtitle("Waiting for USB host. Keep the data cable connected.");
            }
        }
        else if(keys_down & HidNpadButton_B) {
            if(this->typingActive.load(std::memory_order_acquire)) {
                this->cancelRequested = true;
                this->cancelFlag.store(true, std::memory_order_release);
            }
        }
    });
}

void EzBasicApp::TypingWorker() {
    this->linesTyped.store(0, std::memory_order_relaxed);
    this->progressLines.store(0, std::memory_order_relaxed);
    this->elapsedSec.store(0.0, std::memory_order_relaxed);

    auto start = std::chrono::steady_clock::now();

    size_t strokes = 0;
    size_t lines = 0;
    bool ok = this->engine.VerifyText(this->bootstrapText.c_str(), this->bootstrapText.size(),
                                      &strokes, &lines, nullptr);
    this->totalLines.store(lines, std::memory_order_relaxed);

    bool succeeded = false;
    if(ok && lines > 0) {
        auto progressCb = [&](size_t typedLines, size_t /*totalLines*/) {
            this->progressLines.store(typedLines, std::memory_order_relaxed);
        };

        succeeded = this->engine.TypeText(this->bootstrapText.c_str(), this->bootstrapText.size(),
                                          progressCb, this->cancelFlag);

        this->linesTyped.store(this->progressLines.load(std::memory_order_relaxed),
                               std::memory_order_relaxed);
    }

    auto end = std::chrono::steady_clock::now();
    this->elapsedSec.store(std::chrono::duration<double>(end - start).count(),
                           std::memory_order_relaxed);
    this->typingSucceeded.store(succeeded, std::memory_order_relaxed);
    this->typingFinished.store(true, std::memory_order_release);
    this->typingActive.store(false, std::memory_order_release);
}

void EzBasicApp::PollTyping() {
    u32 current = static_cast<u32>(this->progressLines.load(std::memory_order_relaxed));
    u32 total   = static_cast<u32>(this->totalLines.load(std::memory_order_relaxed));
    if(total > 0) {
        this->UpdateTypingProgress(current, total);
    }

    if(this->typeRequested && this->usbReady.load(std::memory_order_acquire)
       && !this->typingActive.load(std::memory_order_acquire)) {
        this->typeRequested = false;
        this->mainLayout->SetSubtitle("Press A to type bootstrap");
        this->SwitchToTyping();
        this->typingFinished.store(false, std::memory_order_release);
        this->typingSucceeded.store(false, std::memory_order_release);
        this->cancelFlag.store(false, std::memory_order_release);
        this->typingActive.store(true, std::memory_order_release);
        this->totalLines.store(0, std::memory_order_relaxed);
        this->progressLines.store(0, std::memory_order_relaxed);
        this->typingLayout->ResetProgress(0);
        if(this->typingThread.joinable()) {
            this->typingThread.join();
        }
        this->typingThread = std::thread(&EzBasicApp::TypingWorker, this);
    }

    if(this->currentLayout == ActiveLayout::Typing
       && this->typingFinished.load(std::memory_order_acquire)) {
        this->typingFinished.store(false, std::memory_order_release);
        if(this->typingSucceeded.load(std::memory_order_relaxed) &&
           !this->cancelFlag.load(std::memory_order_relaxed)) {
            this->SetTypingSummary(static_cast<u32>(this->linesTyped.load(std::memory_order_relaxed)));
            this->SwitchToDone();
        } else {
            this->mainLayout->SetSubtitle("Typing stopped. Check USB and press A to retry.");
            this->SwitchToMain();
        }
    }
}

void EzBasicApp::SwitchToMain() {
    this->LoadLayout(this->mainLayout);
    this->currentLayout = ActiveLayout::Main;
}

void EzBasicApp::SwitchToTyping() {
    this->LoadLayout(this->typingLayout);
    this->currentLayout = ActiveLayout::Typing;
}

void EzBasicApp::SwitchToDone() {
    this->LoadLayout(this->doneLayout);
    this->currentLayout = ActiveLayout::Done;
}

void EzBasicApp::UpdateTypingProgress(u32 current, u32 total) {
    this->typingLayout->SetProgress(current, total);
}

void EzBasicApp::SetTypingSummary(u32 typedLines) {
    this->doneLayout->SetSummary(typedLines);
}

bool EzBasicApp::IsCancelRequested() const {
    return this->cancelRequested;
}

bool EzBasicApp::IsTypeRequested() const {
    return this->typeRequested;
}

void EzBasicApp::ResetRequests() {
    this->cancelRequested = false;
    this->typeRequested = false;
}

void EzBasicApp::SetBootstrap(const std::string& text, const std::string& hash, std::size_t size) {
    this->bootstrapText = text;
    this->bootstrapHash = hash;
    this->bootstrapSize = size;
}
