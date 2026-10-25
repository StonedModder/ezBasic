#include <MainLayout.hpp>

namespace {
constexpr u32 UiFontSize = 56;
const std::string UiFontName = pu::ui::MakeDefaultFontName(UiFontSize);
}

MainLayout::MainLayout() : Layout::Layout() {
    this->SetBackgroundColor({0x1A, 0x1A, 0x1A, 0xFF});

    this->topBar = pu::ui::elm::Rectangle::New(
        0, 0, 1920, 128, pu::ui::Color{0x2D, 0x2D, 0x2D, 0xFF});
    this->Add(this->topBar);

    this->titleText = pu::ui::elm::TextBlock::New(0, 30, "ezBasic");
    this->titleText->SetFont(UiFontName);
    this->titleText->SetColor({0xFF, 0xFF, 0xFF, 0xFF});
    this->titleText->SetX(960 - (this->titleText->GetWidth() / 2));
    this->Add(this->titleText);

    this->subtitleText = pu::ui::elm::TextBlock::New(0, 470, "Press A to type bootstrap");
    this->subtitleText->SetFont(UiFontName);
    this->subtitleText->SetColor({0xCC, 0xCC, 0xCC, 0xFF});
    this->subtitleText->SetX(960 - (this->subtitleText->GetWidth() / 2));
    this->Add(this->subtitleText);

    this->hintText = pu::ui::elm::TextBlock::New(0, 982, "Press + to exit at any time");
    this->hintText->SetFont(UiFontName);
    this->hintText->SetColor({0x88, 0x88, 0x88, 0xFF});
    this->hintText->SetX(960 - (this->hintText->GetWidth() / 2));
    this->Add(this->hintText);
}

void MainLayout::SetSubtitle(const std::string& text) {
    this->subtitleText->SetText(text);
    this->subtitleText->SetX(960 - (this->subtitleText->GetWidth() / 2));
}