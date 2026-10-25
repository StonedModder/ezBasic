#include <DoneLayout.hpp>

namespace {
constexpr u32 UiFontSize = 56;
const std::string UiFontName = pu::ui::MakeDefaultFontName(UiFontSize);
}

DoneLayout::DoneLayout() : Layout::Layout() {
    this->SetBackgroundColor({0x1A, 0x1A, 0x1A, 0xFF});

    this->checkBg = pu::ui::elm::Rectangle::New(
        870, 250, 180, 180, pu::ui::Color{0x00, 0x99, 0x44, 0xFF});
    this->Add(this->checkBg);

    this->completeText = pu::ui::elm::TextBlock::New(0, 500, "Complete!");
    this->completeText->SetFont(UiFontName);
    this->completeText->SetColor({0x00, 0xE6, 0x8A, 0xFF});
    this->completeText->SetX(960 - (this->completeText->GetWidth() / 2));
    this->Add(this->completeText);

    this->summaryText = pu::ui::elm::TextBlock::New(0, 630, "Lines typed: 0");
    this->summaryText->SetFont(UiFontName);
    this->summaryText->SetColor({0xCC, 0xCC, 0xCC, 0xFF});
    this->summaryText->SetX(960 - (this->summaryText->GetWidth() / 2));
    this->Add(this->summaryText);

    this->exitHint = pu::ui::elm::TextBlock::New(0, 982, "Press + to exit");
    this->exitHint->SetFont(UiFontName);
    this->exitHint->SetColor({0x88, 0x88, 0x88, 0xFF});
    this->exitHint->SetX(960 - (this->exitHint->GetWidth() / 2));
    this->Add(this->exitHint);
}

void DoneLayout::SetSummary(u32 linesTyped) {
    this->summaryText->SetText("Lines typed: " + std::to_string(linesTyped));
    this->summaryText->SetX(960 - (this->summaryText->GetWidth() / 2));
}