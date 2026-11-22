#include "dns_manager.h"
#include <shlobj.h>
#include <sstream>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")

typedef BOOL(WINAPI* pfnDnsFlushResolverCache)();

DnsManager& DnsManager::Instance() {
    static DnsManager instance;
    return instance;
}

DnsManager::DnsManager() : m_isConnected(false), m_isBusy(false), m_currentProviderIndex(0) {
    m_providers.push_back({
        L"Cloudflare Family (1.1.1.3)",
        L"Lightning fast speed + malware & adult content blocking.",
        L"1.1.1.3",
        L"1.0.0.3",
        L"2606:4700:4700::1113",
        L"2606:4700:4700::1003"
    });

    m_providers.push_back({
        L"CleanBrowsing Family",
        L"Strict adult filter + SafeSearch on Google & Bing.",
        L"185.228.168.168",
        L"185.228.169.168",
        L"2a0d:2a00:1::",
        L"2a0d:2a00:2::"
    });

    m_providers.push_back({
        L"AdGuard Family",
        L"Blocks adult content, adult ads, and online trackers.",
        L"94.140.14.15",
        L"94.140.15.16",
        L"2a10:50c0::bad1:ff",
        L"2a10:50c0::bad2:ff"
    });

    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        std::wstring p(exePath);
        size_t lastSlash = p.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            m_backupFilePath = p.substr(0, lastSlash + 1) + L"dns_backup_state.ini";
        } else {
            m_backupFilePath = L"dns_backup_state.ini";
        }
    }
}

DnsManager::~DnsManager() {
    if (m_isConnected) {
        Disconnect();
    }
}

bool DnsManager::IsKnownFilterDns(const std::wstring& ip) {
    if (ip == L"1.1.1.3" || ip == L"1.0.0.3" ||
        ip == L"185.228.168.168" || ip == L"185.228.169.168" ||
        ip == L"94.140.14.15" || ip == L"94.140.15.16" ||
        ip.find(L"2606:4700:4700::") != std::wstring::npos ||
        ip.find(L"2a0d:2a00:") != std::wstring::npos ||
        ip.find(L"2a10:50c0:") != std::wstring::npos) {
        return true;
    }
    return false;
}

bool DnsManager::IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

std::vector<DnsProvider> DnsManager::GetAvailableProviders() {
    return m_providers;
}

std::vector<std::wstring> DnsManager::GetActiveAdapters() {
    std::vector<std::wstring> adapters;
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    }

    if (pAddresses && GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddresses, &outBufLen) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES curr = pAddresses; curr; curr = curr->Next) {
            if (curr->OperStatus == IfOperStatusUp &&
                curr->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
                curr->IfType != IF_TYPE_TUNNEL) {
                if (curr->FriendlyName && wcslen(curr->FriendlyName) > 0) {
                    adapters.push_back(curr->FriendlyName);
                }
            }
        }
    }

    if (pAddresses) {
        free(pAddresses);
    }

    return adapters;
}

std::wstring DnsManager::GetActiveAdapterSummary() {
    auto adapters = GetActiveAdapters();
    if (adapters.empty()) return L"No active network adapter";
    if (adapters.size() == 1) return adapters[0];
    return adapters[0] + L" (+" + std::to_wstring(adapters.size() - 1) + L" more)";
}

std::wstring DnsManager::GetActiveDnsDisplay() {
    if (m_isBusy) {
        return m_isConnected ? L"Updating..." : L"Connecting...";
    }
    if (!m_isConnected) {
        return L"Default (ISP / DHCP)";
    }
    if (m_currentProviderIndex >= 0 && m_currentProviderIndex < (int)m_providers.size()) {
        const auto& p = m_providers[m_currentProviderIndex];
        return p.ipv4Primary + L", " + p.ipv4Secondary;
    }
    return L"Protected";
}

bool DnsManager::RunHiddenProcess(const std::wstring& cmd) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::wstring cmdCopy = cmd;
    BOOL success = CreateProcessW(
        NULL,
        &cmdCopy[0],
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (success) {
        WaitForSingleObject(pi.hProcess, 3000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

bool DnsManager::FlushDnsCache() {
    HMODULE hDnsApi = LoadLibraryA("dnsapi.dll");
    if (hDnsApi) {
        pfnDnsFlushResolverCache pFlush = (pfnDnsFlushResolverCache)GetProcAddress(hDnsApi, "DnsFlushResolverCache");
        if (pFlush) {
            pFlush();
        }
        FreeLibrary(hDnsApi);
    }
    RunHiddenProcess(L"ipconfig.exe /flushdns");
    RunHiddenProcess(L"ipconfig.exe /registerdns");
    RunHiddenProcess(L"nbtstat.exe -R");
    return true;
}

bool DnsManager::Connect(int providerIndex) {
    if (providerIndex < 0 || providerIndex >= (int)m_providers.size()) {
        providerIndex = 0;
    }
    m_currentProviderIndex = providerIndex;
    const auto& provider = m_providers[providerIndex];

    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    }

    if (!pAddresses || GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddresses, &outBufLen) != NO_ERROR) {
        if (pAddresses) free(pAddresses);
        return false;
    }

    m_backups.clear();

    for (PIP_ADAPTER_ADDRESSES curr = pAddresses; curr; curr = curr->Next) {
        if (curr->OperStatus == IfOperStatusUp &&
            curr->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
            curr->IfType != IF_TYPE_TUNNEL) {
            
            std::wstring adapterName = curr->FriendlyName ? curr->FriendlyName : L"";
            std::wstring adapterGuid;
            if (curr->AdapterName) {
                int len = MultiByteToWideChar(CP_ACP, 0, curr->AdapterName, -1, NULL, 0);
                if (len > 0) {
                    std::vector<wchar_t> wbuf(len);
                    MultiByteToWideChar(CP_ACP, 0, curr->AdapterName, -1, &wbuf[0], len);
                    adapterGuid = &wbuf[0];
                }
            }

            if (adapterName.empty()) continue;

            AdapterDnsBackup backup;
            backup.ifIndex = curr->IfIndex;
            backup.adapterName = adapterName;
            backup.adapterDesc = curr->Description ? curr->Description : L"";
            backup.wasDhcp = true;

            if (!adapterGuid.empty()) {
                std::wstring regSubKey = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + adapterGuid;
                HKEY hKey;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regSubKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    wchar_t valBuf[512] = {0};
                    DWORD valLen = sizeof(valBuf);
                    DWORD type = 0;
                    if (RegQueryValueExW(hKey, L"NameServer", NULL, &type, (LPBYTE)valBuf, &valLen) == ERROR_SUCCESS) {
                        if (wcslen(valBuf) > 0) {
                            std::wstringstream ss(valBuf);
                            std::wstring item;
                            bool isFilterDns = false;
                            std::vector<std::wstring> ips;
                            while (std::getline(ss, item, L',')) {
                                if (!item.empty()) {
                                    if (IsKnownFilterDns(item)) isFilterDns = true;
                                    ips.push_back(item);
                                }
                            }
                            if (!isFilterDns && !ips.empty()) {
                                backup.wasDhcp = false;
                                backup.originalIpv4 = ips;
                            }
                        }
                    }
                    RegCloseKey(hKey);
                }
            }

            m_backups.push_back(backup);

            RunHiddenProcess(L"netsh.exe interface ipv4 set dnsservers name=\"" + adapterName + L"\" source=static address=" + provider.ipv4Primary + L" validate=no");
            RunHiddenProcess(L"netsh.exe interface ipv4 add dnsservers name=\"" + adapterName + L"\" address=" + provider.ipv4Secondary + L" index=2 validate=no");

            if (!provider.ipv6Primary.empty()) {
                RunHiddenProcess(L"netsh.exe interface ipv6 set dnsservers name=\"" + adapterName + L"\" source=static address=" + provider.ipv6Primary + L" validate=no");
                RunHiddenProcess(L"netsh.exe interface ipv6 add dnsservers name=\"" + adapterName + L"\" address=" + provider.ipv6Secondary + L" index=2 validate=no");
            }
        }
    }

    free(pAddresses);

    FlushDnsCache();
    m_isConnected = true;
    SaveBackupState();
    return true;
}

bool DnsManager::Disconnect() {
    auto adapters = GetActiveAdapters();
    for (const auto& name : adapters) {
        bool restoredStatic = false;
        for (const auto& b : m_backups) {
            if (b.adapterName == name && !b.wasDhcp && !b.originalIpv4.empty()) {
                bool hasFilter = false;
                for (const auto& ip : b.originalIpv4) {
                    if (IsKnownFilterDns(ip)) { hasFilter = true; break; }
                }
                if (!hasFilter) {
                    RunHiddenProcess(L"netsh.exe interface ipv4 set dnsservers name=\"" + name + L"\" source=static address=" + b.originalIpv4[0] + L" validate=no");
                    for (size_t i = 1; i < b.originalIpv4.size(); ++i) {
                        RunHiddenProcess(L"netsh.exe interface ipv4 add dnsservers name=\"" + name + L"\" address=" + b.originalIpv4[i] + L" index=" + std::to_wstring(i + 1) + L" validate=no");
                    }
                    restoredStatic = true;
                    break;
                }
            }
        }

        if (!restoredStatic) {
            RunHiddenProcess(L"netsh.exe interface ipv4 set dnsservers name=\"" + name + L"\" source=dhcp");
            RunHiddenProcess(L"netsh.exe interface ipv6 set dnsservers name=\"" + name + L"\" source=dhcp");
        }
    }

    FlushDnsCache();
    m_isConnected = false;
    m_backups.clear();
    DeleteBackupStateFile();
    return true;
}

bool DnsManager::ResetAllToDhcp() {
    auto adapters = GetActiveAdapters();
    for (const auto& name : adapters) {
        RunHiddenProcess(L"netsh.exe interface ipv4 set dnsservers name=\"" + name + L"\" source=dhcp");
        RunHiddenProcess(L"netsh.exe interface ipv6 set dnsservers name=\"" + name + L"\" source=dhcp");
    }
    FlushDnsCache();
    m_isConnected = false;
    m_backups.clear();
    DeleteBackupStateFile();
    return true;
}

void DnsManager::ConnectAsync(int providerIndex, HWND notifyWnd, UINT notifyMsg) {
    if (m_isBusy) return;
    m_isBusy = true;

    std::thread([this, providerIndex, notifyWnd, notifyMsg]() {
        Connect(providerIndex);
        m_isBusy = false;
        if (notifyWnd && notifyMsg) {
            PostMessage(notifyWnd, notifyMsg, 0, 0);
        }
    }).detach();
}

void DnsManager::DisconnectAsync(HWND notifyWnd, UINT notifyMsg) {
    if (m_isBusy) return;
    m_isBusy = true;

    std::thread([this, notifyWnd, notifyMsg]() {
        Disconnect();
        m_isBusy = false;
        if (notifyWnd && notifyMsg) {
            PostMessage(notifyWnd, notifyMsg, 0, 0);
        }
    }).detach();
}

void DnsManager::ResetAllToDhcpAsync(HWND notifyWnd, UINT notifyMsg) {
    if (m_isBusy) return;
    m_isBusy = true;

    std::thread([this, notifyWnd, notifyMsg]() {
        ResetAllToDhcp();
        m_isBusy = false;
        if (notifyWnd && notifyMsg) {
            PostMessage(notifyWnd, notifyMsg, 0, 0);
        }
    }).detach();
}

bool DnsManager::SaveBackupState() {
    std::wofstream out(m_backupFilePath);
    if (!out.is_open()) return false;

    out << L"[AntiGoonState]\n";
    out << L"Connected=1\n";
    out << L"Provider=" << m_currentProviderIndex << L"\n";
    out << L"AdapterCount=" << m_backups.size() << L"\n";

    for (size_t i = 0; i < m_backups.size(); ++i) {
        out << L"[Adapter_" << i << L"]\n";
        out << L"IfIndex=" << m_backups[i].ifIndex << L"\n";
        out << L"Name=" << m_backups[i].adapterName << L"\n";
        out << L"WasDhcp=" << (m_backups[i].wasDhcp ? 1 : 0) << L"\n";
        out << L"Ipv4Count=" << m_backups[i].originalIpv4.size() << L"\n";
        for (size_t j = 0; j < m_backups[i].originalIpv4.size(); ++j) {
            out << L"Ipv4_" << j << L"=" << m_backups[i].originalIpv4[j] << L"\n";
        }
    }
    return true;
}

bool DnsManager::LoadBackupState() {
    std::wifstream in(m_backupFilePath);
    if (!in.is_open()) return false;

    m_backups.clear();
    std::wstring line;
    AdapterDnsBackup curr;

    while (std::getline(in, line)) {
        if (line.find(L"Provider=") == 0) {
            m_currentProviderIndex = std::stoi(line.substr(9));
        } else if (line.find(L"IfIndex=") == 0) {
            curr.ifIndex = (DWORD)std::stoul(line.substr(8));
        } else if (line.find(L"Name=") == 0) {
            if (!curr.adapterName.empty()) {
                m_backups.push_back(curr);
                curr = AdapterDnsBackup();
            }
            curr.adapterName = line.substr(5);
        } else if (line.find(L"WasDhcp=") == 0) {
            curr.wasDhcp = (line.substr(8) == L"1");
        } else if (line.find(L"Ipv4_") == 0) {
            size_t eq = line.find(L'=');
            if (eq != std::wstring::npos) {
                curr.originalIpv4.push_back(line.substr(eq + 1));
            }
        }
    }
    if (!curr.adapterName.empty()) {
        m_backups.push_back(curr);
    }
    return !m_backups.empty();
}

void DnsManager::DeleteBackupStateFile() {
    DeleteFileW(m_backupFilePath.c_str());
}

bool DnsManager::CheckAndRestorePreviousStateIfAny() {
    if (GetFileAttributesW(m_backupFilePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (LoadBackupState()) {
            m_isConnected = true;
            return true;
        }
    }

    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    }

    bool detectedFilter = false;
    if (pAddresses && GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddresses, &outBufLen) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES curr = pAddresses; curr; curr = curr->Next) {
            if (curr->OperStatus == IfOperStatusUp && curr->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
                if (curr->AdapterName) {
                    int len = MultiByteToWideChar(CP_ACP, 0, curr->AdapterName, -1, NULL, 0);
                    if (len > 0) {
                        std::vector<wchar_t> wbuf(len);
                        MultiByteToWideChar(CP_ACP, 0, curr->AdapterName, -1, &wbuf[0], len);
                        std::wstring regSubKey = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + std::wstring(&wbuf[0]);
                        HKEY hKey;
                        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regSubKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                            wchar_t valBuf[512] = {0};
                            DWORD valLen = sizeof(valBuf);
                            DWORD type = 0;
                            if (RegQueryValueExW(hKey, L"NameServer", NULL, &type, (LPBYTE)valBuf, &valLen) == ERROR_SUCCESS) {
                                if (IsKnownFilterDns(valBuf)) {
                                    detectedFilter = true;
                                    for (size_t i = 0; i < m_providers.size(); ++i) {
                                        if (std::wstring(valBuf).find(m_providers[i].ipv4Primary) != std::wstring::npos) {
                                            m_currentProviderIndex = (int)i;
                                            break;
                                        }
                                    }
                                }
                            }
                            RegCloseKey(hKey);
                        }
                    }
                }
            }
        }
    }
    if (pAddresses) free(pAddresses);

    if (detectedFilter) {
        m_isConnected = true;
        return true;
    }

    return false;
}
