#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>

enum class ButtonState {
    Normal,
    Hover,
    Pressed
};

struct HitTestResult {
    enum Type {
        None,
        PowerButton,
        ProviderItem0,
        ProviderItem1,
        ProviderItem2,
        FlushDnsButton,
        MinimizeTrayButton
    } type;
};

class UiRenderer {
public:
    UiRenderer();
    ~UiRenderer();

    void Initialize(HWND hWnd);
    void Render(HDC hdc, int width, int height);
    HitTestResult HitTest(int x, int y);
    void SetHoverElement(HitTestResult::Type type);
    void SetPressedElement(HitTestResult::Type type);
    void TriggerFlushDnsFeedback();
    void UpdateAnimation();

private:
    void DrawHeader(Gdiplus::Graphics& g, int width);
    void DrawPowerButton(Gdiplus::Graphics& g, int width);
    void DrawProviderSelector(Gdiplus::Graphics& g, int width);
    void DrawNetworkInfoCard(Gdiplus::Graphics& g, int width);
    void DrawBottomBar(Gdiplus::Graphics& g, int width, int height);

    void FillRoundRect(Gdiplus::Graphics& g, Gdiplus::Brush* brush, float x, float y, float width, float height, float radius);
    void DrawRoundRect(Gdiplus::Graphics& g, Gdiplus::Pen* pen, float x, float y, float width, float height, float radius);

    HWND m_hWnd;
    ULONG_PTR m_gdiplusToken;
    HitTestResult::Type m_hoverType;
    HitTestResult::Type m_pressedType;
    float m_animPulse;
    bool m_animPulseGrowing;
    int m_flushDnsFeedbackTicks;
};
