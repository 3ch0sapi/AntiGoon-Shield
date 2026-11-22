#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include "dns_manager.h"
#include "ui_renderer.h"

#define IDI_APP_ICON 101

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define WM_TRAYICON (WM_APP + 1)
#define WM_DNS_FINISHED (WM_APP + 2)

#define ID_TRAY_OPEN 1001
#define ID_TRAY_CONNECT 1002
#define ID_TRAY_DISCONNECT 1003
#define ID_TRAY_FLUSH 1004
#define ID_TRAY_DISCONNECT_AND_EXIT 1005
#define ID_TRAY_RESET_DHCP 1006

#define TIMER_ANIMATION 1

static const wchar_t* CLASS_NAME = L"AntiGoonVPNShieldWindow";
static NOTIFYICONDATAW g_nid = { 0 };
static UiRenderer* g_pRenderer = nullptr;
static HCURSOR g_hHandCursor = NULL;
static HCURSOR g_hArrowCursor = NULL;
static HICON g_hAppIcon = NULL;

void UpdateTrayIcon(HWND hWnd) {
    bool connected = DnsManager::Instance().IsConnected();
    bool busy = DnsManager::Instance().IsBusy();
    std::wstring tip;
    if (busy) {
        tip = L"Anti-Goon: Configuring DNS...";
    } else if (connected) {
        tip = L"Anti-Goon: Protected (Adult & NSFW Blocked)";
    } else {
        tip = L"Anti-Goon: Disconnected (Default DNS)";
    }
    wcsncpy_s(g_nid.szTip, tip.c_str(), _countof(g_nid.szTip) - 1);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

void ShowTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    bool connected = DnsManager::Instance().IsConnected();
    bool busy = DnsManager::Instance().IsBusy();

    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, L"Open Anti-Goon");
    SetMenuDefaultItem(hMenu, ID_TRAY_OPEN, FALSE);

    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_TRAY_DISCONNECT_AND_EXIT, L"Disconnect & Exit Completely");
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_RESET_DHCP, L"Reset All Adapters to Default (DHCP)");

    InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    if (!connected) {
        InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_STRING | (busy ? MF_GRAYED : 0), ID_TRAY_CONNECT, L"Activate Protection");
    } else {
        InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_STRING | (busy ? MF_GRAYED : 0), ID_TRAY_DISCONNECT, L"Deactivate (Disconnect)");
    }

    InsertMenuW(hMenu, 5, MF_BYPOSITION | MF_STRING, ID_TRAY_FLUSH, L"Flush DNS Cache");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        g_hAppIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP_ICON));
        if (!g_hAppIcon) {
            g_hAppIcon = LoadIcon(NULL, IDI_SHIELD);
        }

        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)g_hAppIcon);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hAppIcon);

        g_nid.cbSize = sizeof(NOTIFYICONDATAW);
        g_nid.hWnd = hWnd;
        g_nid.uID = 1;
        g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        g_nid.uCallbackMessage = WM_TRAYICON;
        g_nid.hIcon = g_hAppIcon;
        wcsncpy_s(g_nid.szTip, L"Anti-Goon VPN Shield", _countof(g_nid.szTip) - 1);
        Shell_NotifyIconW(NIM_ADD, &g_nid);

        g_hHandCursor = LoadCursor(NULL, IDC_HAND);
        g_hArrowCursor = LoadCursor(NULL, IDC_ARROW);

        SetTimer(hWnd, TIMER_ANIMATION, 40, NULL);
        break;
    }

    case WM_TIMER: {
        if (wParam == TIMER_ANIMATION && g_pRenderer) {
            g_pRenderer->UpdateAnimation();
        }
        break;
    }

    case WM_DNS_FINISHED: {
        UpdateTrayIcon(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }

    case WM_MOUSEMOVE: {
        if (!g_pRenderer) break;
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        HitTestResult hit = g_pRenderer->HitTest(x, y);
        g_pRenderer->SetHoverElement(hit.type);

        if (hit.type != HitTestResult::None) {
            SetCursor(g_hHandCursor);
        } else {
            SetCursor(g_hArrowCursor);
        }

        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE: {
        if (g_pRenderer) {
            g_pRenderer->SetHoverElement(HitTestResult::None);
            g_pRenderer->SetPressedElement(HitTestResult::None);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        if (!g_pRenderer) break;
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        HitTestResult hit = g_pRenderer->HitTest(x, y);
        g_pRenderer->SetPressedElement(hit.type);
        break;
    }

    case WM_LBUTTONUP: {
        if (!g_pRenderer) break;
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        HitTestResult hit = g_pRenderer->HitTest(x, y);
        g_pRenderer->SetPressedElement(HitTestResult::None);

        if (hit.type == HitTestResult::PowerButton) {
            if (!DnsManager::Instance().IsBusy()) {
                if (DnsManager::Instance().IsConnected()) {
                    DnsManager::Instance().DisconnectAsync(hWnd, WM_DNS_FINISHED);
                } else {
                    DnsManager::Instance().ConnectAsync(DnsManager::Instance().GetCurrentProviderIndex(), hWnd, WM_DNS_FINISHED);
                }
                UpdateTrayIcon(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        } else if (hit.type >= HitTestResult::ProviderItem0 && hit.type <= HitTestResult::ProviderItem2) {
            int newIdx = (int)hit.type - (int)HitTestResult::ProviderItem0;
            if (newIdx != DnsManager::Instance().GetCurrentProviderIndex() && !DnsManager::Instance().IsBusy()) {
                if (DnsManager::Instance().IsConnected()) {
                    DnsManager::Instance().ConnectAsync(newIdx, hWnd, WM_DNS_FINISHED);
                } else {
                    DnsManager::Instance().SetCurrentProviderIndex(newIdx);
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
        } else if (hit.type == HitTestResult::FlushDnsButton) {
            DnsManager::Instance().FlushDnsCache();
            g_pRenderer->TriggerFlushDnsFeedback();
        } else if (hit.type == HitTestResult::MinimizeTrayButton) {
            ShowWindow(hWnd, SW_HIDE);
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        if (g_pRenderer) {
            g_pRenderer->Render(hdc, rc.right, rc.bottom);
        }
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_TRAYICON: {
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (lParam == WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case ID_TRAY_OPEN:
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        case ID_TRAY_CONNECT:
            if (!DnsManager::Instance().IsBusy()) {
                DnsManager::Instance().ConnectAsync(DnsManager::Instance().GetCurrentProviderIndex(), hWnd, WM_DNS_FINISHED);
                UpdateTrayIcon(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case ID_TRAY_DISCONNECT:
            if (!DnsManager::Instance().IsBusy()) {
                DnsManager::Instance().DisconnectAsync(hWnd, WM_DNS_FINISHED);
                UpdateTrayIcon(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case ID_TRAY_FLUSH:
            DnsManager::Instance().FlushDnsCache();
            if (g_pRenderer) g_pRenderer->TriggerFlushDnsFeedback();
            break;
        case ID_TRAY_RESET_DHCP:
            if (!DnsManager::Instance().IsBusy()) {
                DnsManager::Instance().ResetAllToDhcpAsync(hWnd, WM_DNS_FINISHED);
                UpdateTrayIcon(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case ID_TRAY_DISCONNECT_AND_EXIT:
            if (DnsManager::Instance().IsConnected()) {
                DnsManager::Instance().Disconnect();
            } else {
                DnsManager::Instance().ResetAllToDhcp();
            }
            DestroyWindow(hWnd);
            break;
        }
        break;
    }

    case WM_CLOSE: {
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }

    case WM_DESTROY: {
        KillTimer(hWnd, TIMER_ANIMATION);
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        if (DnsManager::Instance().IsConnected()) {
            DnsManager::Instance().Disconnect();
        }
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    if (!DnsManager::Instance().IsRunningAsAdmin()) {
        wchar_t szPath[MAX_PATH];
        if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath))) {
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;
            if (ShellExecuteExW(&sei)) {
                return 0;
            }
        }
        MessageBoxW(NULL,
            L"Anti-Goon requires Administrator privileges to switch network DNS adapters.\nPlease run the application as Administrator.",
            L"Administrator Privileges Required",
            MB_ICONERROR | MB_OK);
        return 1;
    }

    if (pCmdLine && (wcsstr(pCmdLine, L"--reset") || wcsstr(pCmdLine, L"-reset"))) {
        DnsManager::Instance().ResetAllToDhcp();
        return 0;
    }

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"AntiGoonShieldSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(CLASS_NAME, NULL);
        if (hExisting) {
            ShowWindow(hExisting, SW_SHOW);
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_SHIELD);

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = hIcon;
    wcex.hIconSm = hIcon;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszClassName = CLASS_NAME;

    if (!RegisterClassExW(&wcex)) {
        return 1;
    }

    int winWidth = 420;
    int winHeight = 620;

    RECT rc = { 0, 0, winWidth, winHeight };
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - (rc.right - rc.left)) / 2;
    int posY = (screenH - (rc.bottom - rc.top)) / 2;

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Anti-Goon | VPN DNS Shield",
        dwStyle,
        posX, posY,
        rc.right - rc.left,
        rc.bottom - rc.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd) {
        return 1;
    }

    UiRenderer renderer;
    renderer.Initialize(hWnd);
    g_pRenderer = &renderer;

    DnsManager::Instance().CheckAndRestorePreviousStateIfAny();

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_pRenderer = nullptr;
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return (int)msg.wParam;
}
