#include <TypingLayout.hpp>

namespace {
constexpr u32 UiFontSize = 56;
const std::string UiFontName = pu::ui::MakeDefaultFontName(UiFontSize);
}

TypingLayout::TypingLayout() : Layout::Layout() {
    this->SetBackgroundColor({0x1A, 0x1A, 0x1A, 0xFF});

    this->statusText = pu::ui::elm::TextBlock::New(0, 300, "Typing bootstrap.bas...");
    this->statusText->SetFont(UiFontName);
    this->statusText->SetColor({0xFF, 0xFF, 0xFF, 0xFF});
    this->statusText->SetX(960 - (this->statusText->GetWidth() / 2));
    this->Add(this->statusText);

    this->progressBar = pu::ui::elm::ProgressBar::New(300, 500, 1320, 56, 100);
    this->progressBar->SetProgressColor({0x00, 0xE6, 0x8A, 0xFF});
    this->progressBar->SetBackgroundColor({0x33, 0x33, 0x33, 0xFF});
    this->Add(this->progressBar);

    this->progressText = pu::ui::elm::TextBlock::New(0, 610, "Line 0 / 0");
    this->progressText->SetFont(UiFontName);
    this->progressText->SetColor({0xCC, 0xCC, 0xCC, 0xFF});
    this->progressText->SetX(960 - (this->progressText->GetWidth() / 2));
    this->Add(this->progressText);

    this->cancelHint = pu::ui::elm::TextBlock::New(0, 982, "Press B to cancel");
    this->cancelHint->SetFont(UiFontName);
    this->cancelHint->SetColor({0x88, 0x88, 0x88, 0xFF});
    this->cancelHint->SetX(960 - (this->cancelHint->GetWidth() / 2));
    this->Add(this->cancelHint);
}

void TypingLayout::SetProgress(u32 current, u32 total) {
    if(total == 0) {
        this->progressBar->SetProgress(0);
        this->progressText->SetText("Line 0 / 0");
    } else {
        u32 pct = (current * 100) / total;
        this->progressBar->SetProgress(pct);
        this->progressText->SetText("Line " + std::to_string(current) + " / " + std::to_string(total));
    }
    this->progressText->SetX(960 - (this->progressText->GetWidth() / 2));
}

void TypingLayout::ResetProgress(u32 total) {
    this->progressBar->SetProgress(0);
    this->progressText->SetText("Line 0 / " + std::to_string(total));
    this->progressText->SetX(960 - (this->progressText->GetWidth() / 2));
}