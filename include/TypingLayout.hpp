#pragma once

#include <pu/Plutonium>

class TypingLayout : public pu::ui::Layout {
    private:
        pu::ui::elm::TextBlock::Ref statusText;
        pu::ui::elm::ProgressBar::Ref progressBar;
        pu::ui::elm::TextBlock::Ref progressText;
        pu::ui::elm::TextBlock::Ref cancelHint;

    public:
        TypingLayout();
        PU_SMART_CTOR(TypingLayout)

        void SetProgress(u32 current, u32 total);
        void ResetProgress(u32 total);
};
