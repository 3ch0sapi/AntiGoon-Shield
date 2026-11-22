#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>

struct DnsProvider {
    std::wstring name;
    std::wstring shortDesc;
    std::wstring ipv4Primary;
    std::wstring ipv4Secondary;
    std::wstring ipv6Primary;
    std::wstring ipv6Secondary;
};

struct AdapterDnsBackup {
    DWORD ifIndex = 0;
    std::wstring adapterName;
    std::wstring adapterDesc;
    bool wasDhcp = true;
    std::vector<std::wstring> originalIpv4;
    std::vector<std::wstring> originalIpv6;
};

class DnsManager {
public:
    static DnsManager& Instance();

    bool IsRunningAsAdmin();
    std::vector<DnsProvider> GetAvailableProviders();
    std::vector<std::wstring> GetActiveAdapters();

    void SetCurrentProviderIndex(int idx) {
        if (idx >= 0 && idx < (int)m_providers.size()) {
            m_currentProviderIndex = idx;
        }
    }

    void ConnectAsync(int providerIndex, HWND notifyWnd = NULL, UINT notifyMsg = 0);
    void DisconnectAsync(HWND notifyWnd = NULL, UINT notifyMsg = 0);
    void ResetAllToDhcpAsync(HWND notifyWnd = NULL, UINT notifyMsg = 0);

    bool Connect(int providerIndex);
    bool Disconnect();
    bool ResetAllToDhcp();
    bool FlushDnsCache();

    bool IsConnected() const { return m_isConnected; }
    bool IsBusy() const { return m_isBusy; }
    int GetCurrentProviderIndex() const { return m_currentProviderIndex; }
    std::wstring GetActiveAdapterSummary();
    std::wstring GetActiveDnsDisplay();

    bool CheckAndRestorePreviousStateIfAny();
    bool IsKnownFilterDns(const std::wstring& ip);

private:
    DnsManager();
    ~DnsManager();

    bool SaveBackupState();
    bool LoadBackupState();
    void DeleteBackupStateFile();
    bool RunHiddenProcess(const std::wstring& cmd);

    std::atomic<bool> m_isConnected;
    std::atomic<bool> m_isBusy;
    int m_currentProviderIndex;
    std::vector<DnsProvider> m_providers;
    std::vector<AdapterDnsBackup> m_backups;
    std::wstring m_backupFilePath;
};
