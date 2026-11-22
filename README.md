[README.md](https://github.com/user-attachments/files/35238254/README.md)
# Anti-Goon | VPN-Style DNS Adult & Porn Blocker

[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011-blue?logo=windows)](https://github.com)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg?logo=c%2B%2B)](https://github.com)
[![Binary Size](https://img.shields.io/badge/Size-~360%20KB-success.svg)](https://github.com)
[![Built with AI](https://img.shields.io/badge/Built%20with-100%25%20AI-blueviolet.svg)](https://github.com)

A lightweight, native Windows C++ desktop application built with a modern **VPN client interface** (similar to Cloudflare WARP and Mullvad). It blocks adult websites, pornography, and malicious domains system-wide at the DNS level with zero background CPU or RAM overhead.

> 🤖 **Built 100% with Artificial Intelligence:**  
> This entire project — including the native Win32/GDI+ user interface, asynchronous DNS switching engine, icon generator, and build workflows — was designed and coded collaboratively using AI.

---

## ✨ Features

- **Single-Click VPN Experience:** Simply click **CONNECT** to shield your entire computer, and **DISCONNECT** to revert seamlessly back to your default network settings.
- **100% Portable & Lightweight:** Compiled statically (`/MT`) into a single **~360 KB executable**. Zero external dependencies (no Visual C++ redistributable, .NET, or Python required).
- **Zero Resource Consumption:** Runs through your native Windows network stack. No heavy local proxy processes, consuming 0% CPU and ~10 MB RAM.
- **Universal Compatibility:** Uses interface indexes and native Windows APIs to configure adapters regardless of system language (Czech, English, German, etc.) across Ethernet and Wi-Fi.
- **Fail-Safe DNS Restoration:** Automatically remembers your router's default DHCP configuration and restores it cleanly upon disconnect or exit.
- **System Tray Integration:** Closes quietly to the notification tray. Right-click the tray icon to quickly toggle protection, flush DNS cache, or exit.
- **Single-Instance Enforcement:** Clicking the desktop icon while running simply brings the existing window to the front without spawning duplicate processes.

---

## 🛡️ Available DNS Filters

Switch between 3 pre-configured family-safe DNS providers:

| Provider | Primary IPv4 | Secondary IPv4 | Highlights |
| :--- | :--- | :--- | :--- |
| **Cloudflare Family** *(Default)* | `1.1.1.3` | `1.0.0.3` | Ultra-fast response times, blocks pornographic content and known malware |
| **CleanBrowsing Family** | `185.228.168.168` | `185.228.169.168` | Strict adult domain filtering + automatic SafeSearch enforcement |
| **AdGuard Family** | `94.140.14.15` | `94.140.15.16` | Blocks adult websites, explicit advertisements, and tracking domains |

---

## 🚀 Quick Start

1. Download **`AntiGoon.exe`** from the [Releases](../../releases) tab.
2. Run the application and accept the Windows UAC Administrator prompt (required by Windows to adjust network interface DNS servers).
3. Click **CONNECT (ACTIVATE)**. The status will glow green (**PROTECTED**).
4. When done, click **DISCONNECT** to return to your normal ISP DNS.

*Note: If your browser was already open during connection or disconnection, refresh your tabs or open an Incognito window to clear the browser's internal socket cache.*

---

## 🔒 Privacy & Security

- **No Telemetry / No Tracking:** This tool does not log, store, or transmit any user data.
- **No Third-Party Servers:** Your web traffic is never routed through custom VPN servers; it only points your local DNS resolver to official public safety resolvers (Cloudflare, CleanBrowsing, AdGuard).
- **Open Source:** All C++ source code is visible in this repository for full auditability.

---

## 🛠️ Building from Source

To compile `AntiGoon.exe` locally:

1. Clone this repository:
   ```cmd
   git clone https://github.com/YOUR_USERNAME/AntiGoon.git
   cd AntiGoon
   ```
2. Run the build script:
   ```cmd
   build.bat
   ```
   The script auto-detects your Visual Studio C++ compiler, compiles the icon and resource manifest, and outputs the standalone `AntiGoon.exe`.

---

## 📄 License

This project is open-source software licensed under the MIT License.
