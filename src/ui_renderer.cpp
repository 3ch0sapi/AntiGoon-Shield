#include "ui_renderer.h"
#include "dns_manager.h"
#include <cmath>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

UiRenderer::UiRenderer() :
    m_hWnd(NULL),
    m_gdiplusToken(0),
    m_hoverType(HitTestResult::None),
    m_pressedType(HitTestResult::None),
    m_animPulse(0.0f),
    m_animPulseGrowing(true),
    m_flushDnsFeedbackTicks(0)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
}

UiRenderer::~UiRenderer() {
    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
    }
}

void UiRenderer::Initialize(HWND hWnd) {
    m_hWnd = hWnd;
}

void UiRenderer::SetHoverElement(HitTestResult::Type type) {
    if (m_hoverType != type) {
        m_hoverType = type;
        if (m_hWnd) InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void UiRenderer::SetPressedElement(HitTestResult::Type type) {
    if (m_pressedType != type) {
        m_pressedType = type;
        if (m_hWnd) InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void UiRenderer::TriggerFlushDnsFeedback() {
    m_flushDnsFeedbackTicks = 35;
    if (m_hWnd) InvalidateRect(m_hWnd, NULL, FALSE);
}

void UiRenderer::UpdateAnimation() {
    if (m_animPulseGrowing) {
        m_animPulse += 0.04f;
        if (m_animPulse >= 1.0f) {
            m_animPulse = 1.0f;
            m_animPulseGrowing = false;
        }
    } else {
        m_animPulse -= 0.04f;
        if (m_animPulse <= 0.0f) {
            m_animPulse = 0.0f;
            m_animPulseGrowing = true;
        }
    }

    if (m_flushDnsFeedbackTicks > 0) {
        m_flushDnsFeedbackTicks--;
        if (m_flushDnsFeedbackTicks == 0 && m_hWnd) {
            InvalidateRect(m_hWnd, NULL, FALSE);
        }
    }

    if ((DnsManager::Instance().IsConnected() || DnsManager::Instance().IsBusy()) && m_hWnd) {
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void UiRenderer::FillRoundRect(Graphics& g, Brush* brush, float x, float y, float width, float height, float radius) {
    GraphicsPath path;
    float d = radius * 2.0f;
    if (d > width) d = width;
    if (d > height) d = height;

    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + width - d, y, d, d, 270, 90);
    path.AddArc(x + width - d, y + height - d, d, d, 0, 90);
    path.AddArc(x, y + height - d, d, d, 90, 90);
    path.CloseFigure();

    g.FillPath(brush, &path);
}

void UiRenderer::DrawRoundRect(Graphics& g, Pen* pen, float x, float y, float width, float height, float radius) {
    GraphicsPath path;
    float d = radius * 2.0f;
    if (d > width) d = width;
    if (d > height) d = height;

    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + width - d, y, d, d, 270, 90);
    path.AddArc(x + width - d, y + height - d, d, d, 0, 90);
    path.AddArc(x, y + height - d, d, d, 90, 90);
    path.CloseFigure();

    g.DrawPath(pen, &path);
}

void UiRenderer::Render(HDC hdc, int width, int height) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    SolidBrush bgBrush(Color(255, 15, 17, 26));
    g.FillRectangle(&bgBrush, 0, 0, width, height);

    LinearGradientBrush topGlow(
        Point(0, 0), Point(0, 200),
        Color(255, 23, 27, 42),
        Color(255, 15, 17, 26)
    );
    g.FillRectangle(&topGlow, 0, 0, width, 200);

    DrawHeader(g, width);
    DrawPowerButton(g, width);
    DrawProviderSelector(g, width);
    DrawNetworkInfoCard(g, width);
    DrawBottomBar(g, width, height);

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void UiRenderer::DrawHeader(Graphics& g, int width) {
    float iconX = 24.0f;
    float iconY = 20.0f;
    float iconW = 28.0f;
    float iconH = 34.0f;

    GraphicsPath shieldPath;
    shieldPath.AddLine(iconX, iconY, iconX + iconW, iconY);
    shieldPath.AddLine(iconX + iconW, iconY, iconX + iconW, iconY + iconH * 0.55f);
    shieldPath.AddBezier(
        iconX + iconW, iconY + iconH * 0.55f,
        iconX + iconW * 0.7f, iconY + iconH * 0.95f,
        iconX + iconW * 0.5f, iconY + iconH,
        iconX + iconW * 0.5f, iconY + iconH
    );
    shieldPath.AddBezier(
        iconX + iconW * 0.5f, iconY + iconH,
        iconX + iconW * 0.3f, iconY + iconH * 0.95f,
        iconX, iconY + iconH * 0.55f,
        iconX, iconY + iconH * 0.55f
    );
    shieldPath.CloseFigure();

    bool connected = DnsManager::Instance().IsConnected();
    bool busy = DnsManager::Instance().IsBusy();

    Color shieldColor = busy ? Color(255, 245, 158, 11) :
        (connected ? Color(255, 0, 230, 118) : Color(255, 99, 102, 241));
    SolidBrush shieldBrush(shieldColor);
    g.FillPath(&shieldBrush, &shieldPath);

    Pen innerPen(Color(255, 15, 17, 26), 2.2f);
    if (connected && !busy) {
        PointF pts[3] = {
            PointF(iconX + 7.0f, iconY + 16.0f),
            PointF(iconX + 12.0f, iconY + 22.0f),
            PointF(iconX + 21.0f, iconY + 11.0f)
        };
        g.DrawLines(&innerPen, pts, 3);
    } else {
        g.DrawArc(&innerPen, iconX + 8.5f, iconY + 8.0f, 11.0f, 11.0f, 180, 180);
        SolidBrush innerBrush(Color(255, 15, 17, 26));
        g.FillRectangle(&innerBrush, iconX + 7.0f, iconY + 14.0f, 14.0f, 12.0f);
    }

    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 15, FontStyleBold, UnitPixel);
    Font subFont(&fontFamily, 11, FontStyleRegular, UnitPixel);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    SolidBrush grayBrush(Color(255, 140, 148, 170));

    g.DrawString(L"ANTI-GOON", -1, &titleFont, PointF(iconX + iconW + 12.0f, iconY + 1.0f), &whiteBrush);
    g.DrawString(L"DNS Adult & Porn Blocker", -1, &subFont, PointF(iconX + iconW + 12.0f, iconY + 19.0f), &grayBrush);

    float pillW = 126.0f;
    float pillH = 26.0f;
    float pillX = width - pillW - 20.0f;
    float pillY = iconY + 4.0f;

    Color pillBg = busy ? Color(40, 245, 158, 11) :
        (connected ? Color(40, 0, 230, 118) : Color(35, 239, 68, 68));
    Color pillBorder = busy ? Color(120, 245, 158, 11) :
        (connected ? Color(120, 0, 230, 118) : Color(100, 239, 68, 68));
    SolidBrush pBrush(pillBg);
    Pen pPen(pillBorder, 1.0f);
    FillRoundRect(g, &pBrush, pillX, pillY, pillW, pillH, 13.0f);
    DrawRoundRect(g, &pPen, pillX, pillY, pillW, pillH, 13.0f);

    Color dotColor = busy ? Color(255, 245, 158, 11) :
        (connected ? Color(255, 0, 230, 118) : Color(255, 239, 68, 68));
    SolidBrush dotBrush(dotColor);
    g.FillEllipse(&dotBrush, pillX + 11.0f, pillY + 9.0f, 8.0f, 8.0f);

    Font pillFont(&fontFamily, 10, FontStyleBold, UnitPixel);
    SolidBrush pillTextBrush(dotColor);
    std::wstring statusText = busy ? L"WORKING..." : (connected ? L"PROTECTED" : L"DISCONNECTED");
    g.DrawString(statusText.c_str(), -1, &pillFont, PointF(pillX + 24.0f, pillY + 6.0f), &pillTextBrush);
}

void UiRenderer::DrawPowerButton(Graphics& g, int width) {
    bool connected = DnsManager::Instance().IsConnected();
    bool busy = DnsManager::Instance().IsBusy();
    bool hovered = (m_hoverType == HitTestResult::PowerButton);
    bool pressed = (m_pressedType == HitTestResult::PowerButton);

    float centerX = width * 0.5f;
    float centerY = 168.0f;
    float btnRadius = 56.0f;

    if (pressed) btnRadius -= 2.0f;

    if (connected || busy) {
        float pulseExtra = m_animPulse * 10.0f;
        int alphaPulse = (int)(70.0f - m_animPulse * 40.0f);
        if (alphaPulse < 10) alphaPulse = 10;
        Color glowColor = busy ? Color(alphaPulse, 245, 158, 11) : Color(alphaPulse, 0, 230, 118);
        Pen glowPen(glowColor, 7.0f);
        g.DrawEllipse(&glowPen, centerX - btnRadius - pulseExtra, centerY - btnRadius - pulseExtra,
            (btnRadius + pulseExtra) * 2.0f, (btnRadius + pulseExtra) * 2.0f);
    }

    Color outerRingColor = busy ? Color(255, 245, 158, 11) :
        (connected ? Color(255, 0, 230, 118) :
        (hovered ? Color(255, 99, 102, 241) : Color(255, 45, 52, 75)));
    Pen outerPen(outerRingColor, hovered ? 3.5f : 2.5f);
    g.DrawEllipse(&outerPen, centerX - btnRadius - 7.0f, centerY - btnRadius - 7.0f,
        (btnRadius + 7.0f) * 2.0f, (btnRadius + 7.0f) * 2.0f);

    GraphicsPath btnCircle;
    btnCircle.AddEllipse(centerX - btnRadius, centerY - btnRadius, btnRadius * 2.0f, btnRadius * 2.0f);

    if (busy) {
        LinearGradientBrush busyBrush(
            PointF(centerX, centerY - btnRadius),
            PointF(centerX, centerY + btnRadius),
            Color(255, 217, 119, 6),
            Color(255, 180, 83, 9)
        );
        g.FillPath(&busyBrush, &btnCircle);
    } else if (connected) {
        LinearGradientBrush connBrush(
            PointF(centerX, centerY - btnRadius),
            PointF(centerX, centerY + btnRadius),
            Color(255, 0, 220, 110),
            Color(255, 0, 150, 70)
        );
        g.FillPath(&connBrush, &btnCircle);
    } else {
        LinearGradientBrush discBrush(
            PointF(centerX, centerY - btnRadius),
            PointF(centerX, centerY + btnRadius),
            hovered ? Color(255, 36, 42, 62) : Color(255, 26, 30, 46),
            Color(255, 17, 20, 31)
        );
        g.FillPath(&discBrush, &btnCircle);
    }

    float iconRadius = 22.0f;
    Color iconColor = (connected || busy) ? Color(255, 255, 255, 255) :
        (hovered ? Color(255, 220, 225, 245) : Color(255, 160, 170, 195));
    Pen powerPen(iconColor, 4.0f);
    powerPen.SetStartCap(LineCapRound);
    powerPen.SetEndCap(LineCapRound);

    g.DrawArc(&powerPen, centerX - iconRadius, centerY - iconRadius + 3.0f,
        iconRadius * 2.0f, iconRadius * 2.0f, 135.0f, 270.0f);

    g.DrawLine(&powerPen, centerX, centerY - iconRadius - 2.0f, centerX, centerY - 2.0f);

    FontFamily fontFamily(L"Segoe UI");
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);

    if (busy) {
        Font statusFont(&fontFamily, 13, FontStyleBold, UnitPixel);
        SolidBrush yellowBrush(Color(255, 245, 158, 11));
        g.DrawString(L"PLEASE WAIT...", -1, &statusFont, PointF(centerX, centerY + btnRadius + 18.0f), &sf, &yellowBrush);

        Font subFont(&fontFamily, 11, FontStyleRegular, UnitPixel);
        SolidBrush subBrush(Color(255, 160, 170, 190));
        g.DrawString(L"Configuring network DNS...", -1, &subFont, PointF(centerX, centerY + btnRadius + 36.0f), &sf, &subBrush);
    } else if (connected) {
        Font statusFont(&fontFamily, 13, FontStyleBold, UnitPixel);
        SolidBrush greenBrush(Color(255, 0, 230, 118));
        g.DrawString(L"PROTECTION ACTIVE", -1, &statusFont, PointF(centerX, centerY + btnRadius + 18.0f), &sf, &greenBrush);

        Font subFont(&fontFamily, 11, FontStyleRegular, UnitPixel);
        SolidBrush subBrush(Color(255, 160, 170, 190));
        g.DrawString(L"Click to disconnect", -1, &subFont, PointF(centerX, centerY + btnRadius + 36.0f), &sf, &subBrush);
    } else {
        Font statusFont(&fontFamily, 13, FontStyleBold, UnitPixel);
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        g.DrawString(L"CONNECT (ACTIVATE)", -1, &statusFont, PointF(centerX, centerY + btnRadius + 18.0f), &sf, &whiteBrush);

        Font subFont(&fontFamily, 11, FontStyleRegular, UnitPixel);
        SolidBrush subBrush(Color(255, 140, 148, 170));
        g.DrawString(L"Click to block adult & NSFW content", -1, &subFont, PointF(centerX, centerY + btnRadius + 36.0f), &sf, &subBrush);
    }
}

void UiRenderer::DrawProviderSelector(Graphics& g, int width) {
    float startY = 282.0f;
    float cardX = 20.0f;
    float cardW = width - 40.0f;

    FontFamily fontFamily(L"Segoe UI");
    Font sectionFont(&fontFamily, 10, FontStyleBold, UnitPixel);
    SolidBrush labelBrush(Color(255, 130, 138, 162));

    g.DrawString(L"DNS FILTER SELECTION", -1, &sectionFont, PointF(cardX + 2.0f, startY), &labelBrush);

    auto providers = DnsManager::Instance().GetAvailableProviders();
    int currentSelected = DnsManager::Instance().GetCurrentProviderIndex();

    float itemY = startY + 20.0f;
    float itemH = 50.0f;
    float spacing = 8.0f;

    for (int i = 0; i < (int)providers.size() && i < 3; ++i) {
        bool isSelected = (i == currentSelected);
        bool isHovered = (m_hoverType == (HitTestResult::Type)(HitTestResult::ProviderItem0 + i));

        Color itemBg = isSelected ? Color(255, 28, 33, 50) :
            (isHovered ? Color(255, 23, 27, 41) : Color(255, 19, 22, 34));
        Color itemBorder = isSelected ? Color(255, 99, 102, 241) :
            (isHovered ? Color(255, 55, 62, 90) : Color(255, 36, 41, 62));

        SolidBrush bg(itemBg);
        Pen border(itemBorder, isSelected ? 1.8f : 1.0f);

        FillRoundRect(g, &bg, cardX, itemY, cardW, itemH, 8.0f);
        DrawRoundRect(g, &border, cardX, itemY, cardW, itemH, 8.0f);

        float radioX = cardX + 16.0f;
        float radioY = itemY + itemH * 0.5f;
        float radioR = 8.0f;

        Pen radioBorder(isSelected ? Color(255, 99, 102, 241) : Color(255, 75, 84, 115), 1.5f);
        g.DrawEllipse(&radioBorder, radioX - radioR, radioY - radioR, radioR * 2.0f, radioR * 2.0f);

        if (isSelected) {
            SolidBrush dotBrush(Color(255, 99, 102, 241));
            g.FillEllipse(&dotBrush, radioX - 4.5f, radioY - 4.5f, 9.0f, 9.0f);
        }

        Font nameFont(&fontFamily, 12, FontStyleBold, UnitPixel);
        Font descFont(&fontFamily, 10, FontStyleRegular, UnitPixel);

        SolidBrush nameBrush(Color(255, 255, 255, 255));
        SolidBrush descBrush(Color(255, 140, 148, 170));

        g.DrawString(providers[i].name.c_str(), -1, &nameFont, PointF(radioX + 16.0f, itemY + 8.0f), &nameBrush);
        g.DrawString(providers[i].shortDesc.c_str(), -1, &descFont, PointF(radioX + 16.0f, itemY + 26.0f), &descBrush);

        itemY += itemH + spacing;
    }
}

void UiRenderer::DrawNetworkInfoCard(Graphics& g, int width) {
    float cardX = 20.0f;
    float cardY = 472.0f;
    float cardW = width - 40.0f;
    float cardH = 88.0f;

    SolidBrush bg(Color(255, 19, 22, 34));
    Pen border(Color(255, 36, 41, 62), 1.0f);
    FillRoundRect(g, &bg, cardX, cardY, cardW, cardH, 8.0f);
    DrawRoundRect(g, &border, cardX, cardY, cardW, cardH, 8.0f);

    FontFamily fontFamily(L"Segoe UI");
    Font labelFont(&fontFamily, 10, FontStyleBold, UnitPixel);
    Font valueFont(&fontFamily, 11, FontStyleRegular, UnitPixel);

    SolidBrush labelBrush(Color(255, 120, 128, 150));
    SolidBrush valueBrush(Color(255, 230, 235, 250));

    g.DrawString(L"NETWORK ADAPTER", -1, &labelFont, PointF(cardX + 14.0f, cardY + 12.0f), &labelBrush);
    std::wstring adapter = DnsManager::Instance().GetActiveAdapterSummary();
    g.DrawString(adapter.c_str(), -1, &valueFont, PointF(cardX + 14.0f, cardY + 28.0f), &valueBrush);

    g.DrawString(L"ACTIVE DNS", -1, &labelFont, PointF(cardX + cardW * 0.52f, cardY + 12.0f), &labelBrush);
    std::wstring dnsDisplay = DnsManager::Instance().GetActiveDnsDisplay();
    bool connected = DnsManager::Instance().IsConnected();
    SolidBrush dnsValBrush(connected ? Color(255, 0, 230, 118) : Color(255, 200, 205, 220));
    g.DrawString(dnsDisplay.c_str(), -1, &valueFont, PointF(cardX + cardW * 0.52f, cardY + 28.0f), &dnsValBrush);

    Pen sep(Color(255, 30, 34, 52), 1.0f);
    g.DrawLine(&sep, cardX + 14.0f, cardY + 54.0f, cardX + cardW - 14.0f, cardY + 54.0f);

    Font tagFont(&fontFamily, 10, FontStyleRegular, UnitPixel);
    SolidBrush tagBrush(Color(255, 140, 150, 175));
    g.DrawString(connected ?
        L"✓ Blocked: Adult content, NSFW websites, explicit search" :
        L"○ Protection off – DNS queries route directly through your ISP",
        -1, &tagFont, PointF(cardX + 14.0f, cardY + 63.0f), &tagBrush);
}

void UiRenderer::DrawBottomBar(Graphics& g, int width, int height) {
    float btnX = 20.0f;
    float btnY = height - 52.0f;
    float btnW = 180.0f;
    float btnH = 34.0f;

    bool isHovered = (m_hoverType == HitTestResult::FlushDnsButton);
    Color flushBg = isHovered ? Color(255, 28, 33, 50) : Color(255, 19, 22, 34);
    Color flushBorder = isHovered ? Color(255, 80, 88, 125) : Color(255, 45, 52, 75);

    SolidBrush bg(flushBg);
    Pen border(flushBorder, 1.0f);
    FillRoundRect(g, &bg, btnX, btnY, btnW, btnH, 6.0f);
    DrawRoundRect(g, &border, btnX, btnY, btnW, btnH, 6.0f);

    FontFamily fontFamily(L"Segoe UI");
    Font btnFont(&fontFamily, 10, FontStyleBold, UnitPixel);
    SolidBrush textBrush(m_flushDnsFeedbackTicks > 0 ? Color(255, 0, 230, 118) : Color(255, 180, 190, 215));

    std::wstring flushText = (m_flushDnsFeedbackTicks > 0) ? L"✓ Cache Cleared" : L"Flush DNS Cache";
    g.DrawString(flushText.c_str(), -1, &btnFont, PointF(btnX + 32.0f, btnY + 9.0f), &textBrush);

    float trayBtnW = 175.0f;
    float trayBtnX = width - trayBtnW - 20.0f;
    bool trayHovered = (m_hoverType == HitTestResult::MinimizeTrayButton);
    Color trayBg = trayHovered ? Color(255, 28, 33, 50) : Color(255, 19, 22, 34);
    Color trayBorder = trayHovered ? Color(255, 80, 88, 125) : Color(255, 45, 52, 75);

    SolidBrush trayBgBrush(trayBg);
    Pen trayBorderPen(trayBorder, 1.0f);
    FillRoundRect(g, &trayBgBrush, trayBtnX, btnY, trayBtnW, btnH, 6.0f);
    DrawRoundRect(g, &trayBorderPen, trayBtnX, btnY, trayBtnW, btnH, 6.0f);

    SolidBrush trayTextBrush(Color(255, 180, 190, 215));
    g.DrawString(L"Minimize to Tray", -1, &btnFont, PointF(trayBtnX + 34.0f, btnY + 9.0f), &trayTextBrush);
}

HitTestResult UiRenderer::HitTest(int x, int y) {
    HitTestResult res;
    res.type = HitTestResult::None;

    float centerX = 210.0f;
    if (m_hWnd) {
        RECT rc;
        GetClientRect(m_hWnd, &rc);
        centerX = rc.right * 0.5f;
    }
    float centerY = 168.0f;
    float dx = (float)x - centerX;
    float dy = (float)y - centerY;
    if (std::sqrt(dx * dx + dy * dy) <= 65.0f) {
        res.type = HitTestResult::PowerButton;
        return res;
    }

    float startY = 282.0f;
    float cardX = 20.0f;
    float cardW = (centerX * 2.0f) - 40.0f;
    float itemY = startY + 20.0f;
    float itemH = 50.0f;
    float spacing = 8.0f;

    for (int i = 0; i < 3; ++i) {
        if (x >= cardX && x <= cardX + cardW && y >= itemY && y <= itemY + itemH) {
            res.type = (HitTestResult::Type)(HitTestResult::ProviderItem0 + i);
            return res;
        }
        itemY += itemH + spacing;
    }

    RECT clientRc;
    GetClientRect(m_hWnd, &clientRc);
    int h = clientRc.bottom;
    float btnY = h - 52.0f;

    if (x >= 20 && x <= 200 && y >= btnY && y <= btnY + 34.0f) {
        res.type = HitTestResult::FlushDnsButton;
        return res;
    }

    float trayBtnW = 175.0f;
    float trayBtnX = clientRc.right - trayBtnW - 20.0f;
    if (x >= trayBtnX && x <= trayBtnX + trayBtnW && y >= btnY && y <= btnY + 34.0f) {
        res.type = HitTestResult::MinimizeTrayButton;
        return res;
    }

    return res;
}
