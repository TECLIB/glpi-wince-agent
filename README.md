# Very light GLPI agent for Windows CE

The agent will try to retreive the following informations and send them back to
GLPI with FusionInventory plugin server:
 - device name
 - device hardware and model
 - device operating system version
 - device serial number
 - device MAC address
 - device IP

## Building the agent
To build the agent you will need a full cross-building toolchain for wince. We
used cegcc/mingw32ce based on GCC v4.6.3 and published by [Max Kerllermann](https://github.com/MaxKellermann) for
the [XCSoar project](https://github.com/XCSoar/XCSoar) needs.

The toolchain is available from: http://max.kellermann.name/projects/cegcc/

To build the agent, you need the following tool in your environment:
 - arm-mingw32ce-gcc
 - arm-mingw32ce-cpp
 - arm-mingw32ce-windres
 - arm-mingw32ce-strip

## Building the cab installer
Before building the cab installer, you may want to update the src/Makefile.local
with your GLPI server URL so it is defined during installation.

To build the installer, just start in 
  make cab

This will also download and install lcab and cabwiz tools from launchpad and github.

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
 - [lcab](https://launchpad.net/lcab)
 - [cabwiz](https://github.com/Turbo87/cabwiz)
