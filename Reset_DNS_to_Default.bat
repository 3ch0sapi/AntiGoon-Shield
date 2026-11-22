@echo off
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    powershell -Command "Start-Process cmd -ArgumentList '/c \"%~fn0\"' -Verb RunAs"
    exit /b
)

echo Resetting all network adapters to Default (DHCP)...
netsh interface ipv4 set dnsservers name="Ethernet" source=dhcp
netsh interface ipv6 set dnsservers name="Ethernet" source=dhcp
netsh interface ipv4 set dnsservers name="Wi-Fi" source=dhcp
netsh interface ipv6 set dnsservers name="Wi-Fi" source=dhcp

ipconfig /flushdns
ipconfig /registerdns
nbtstat -R

echo.
echo ========================================================
echo  [SUCCESS] All network adapters reset to DHCP (Default)!
echo ========================================================
timeout /t 3
