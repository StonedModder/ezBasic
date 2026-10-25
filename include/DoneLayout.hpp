#pragma once

#include <pu/Plutonium>

class DoneLayout : public pu::ui::Layout {
    private:
        pu::ui::elm::TextBlock::Ref completeText;
        pu::ui::elm::TextBlock::Ref summaryText;
        pu::ui::elm::TextBlock::Ref exitHint;
        pu::ui::elm::Rectangle::Ref checkBg;

    public:
        DoneLayout();
        PU_SMART_CTOR(DoneLayout)

        void SetSummary(u32 linesTyped);
};
