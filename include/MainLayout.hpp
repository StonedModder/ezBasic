#pragma once

#include <pu/Plutonium>
#include <string>

class MainLayout : public pu::ui::Layout {
    private:
        pu::ui::elm::TextBlock::Ref titleText;
        pu::ui::elm::TextBlock::Ref subtitleText;
        pu::ui::elm::TextBlock::Ref hintText;
        pu::ui::elm::Rectangle::Ref topBar;

    public:
        MainLayout();
        PU_SMART_CTOR(MainLayout)
        void SetSubtitle(const std::string& text);
};
