@ECHO OFF
echo Subst X Drive to network location. If this fails, check VPN/GSA Connection.
subst X: /D
subst X: \\shakti.svceng.com\lab\validation\projects
cd /D X:\kingsgate\tools\T32
rem
echo Starting TRACE32 Arm32 SCP Die0 JTAG 1
start X:\kingsgate\tools\T32\bin\windows64\t32marm.exe -c X:\kingsgate\tools\Kingsgate_TRACE32\Start\JTAG\configJTAG_SCP_Die0.t32 -s X:\kingsgate\tools\Kingsgate_TRACE32\Start\JTAG\work-settings_JTAG_DUAL_DIE.cmm &
timeout 5
rem
echo Starting TRACE32 Arm64 AP Die0 JTAG 4
start X:\kingsgate\tools\T32\bin\windows64\t32marm.exe -c X:\kingsgate\tools\Kingsgate_TRACE32\Start\JTAG\configJTAG_AP_Die0.t32 -s X:\kingsgate\tools\Kingsgate_TRACE32\Start\JTAG\work-settings_JTAG_DUAL_DIE.cmm &
timeout 3
rem
