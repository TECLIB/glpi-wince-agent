# Very light GLPI agent for Windows CE

It will try to retreive the following informations and send them back to GLPI
server:
 - device name
 - device serial number
 - device MAC address
 - device IP

## Test with Wine
As explain on [MSDN](https://msdn.microsoft.com/en-us/library/aa462416.aspx), Microsoft
provides a WinCE emulator on which we can test the software. It can be started using Wine.

1. Install [Microsoft Device Emulator 3.0 -- Standalone Release](https://www.microsoft.com/en-us/download/details.aspx?id=5352)
   - SHA1: `a0de78a04e0af037027512f9c89c4a4d18896132  vs_emulator.exe`
   - Run: `wine vs_emulator.exe`

1. Install [Windows Mobile 6 Professional Images](https://www.microsoft.com/en-us/download/details.aspx?id=7974)
   - SHA1: `1dcc6a95e949ae776143357d197d1a7a6e137539  Windows Mobile 6 Professional Images (USA).msi`
   - Run: `wine msiexec /i "Windows Mobile 6 Professional Images (USA).msi"`

1. Start emulator launcher test script:
   - Run: `wineconsole --backend=curses test/emulator.bat`

## Other resources
 - [Shared Source Microsoft Device Emulator 1.0 Release](https://www.microsoft.com/en-us/download/details.aspx?id=10865)
