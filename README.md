# Very light GLPI agent for Windows CE

The agent will try to retreive the following informations and send them back to
GLPI with FusionInventory plugin server:
 - device name
 - device hardware and model
 - device operating system version
 - device serial number
 - device MAC address
 - device IP

OCSInventory server should also be supported as far as UserAgent filtering on
server side is updated to accept `GLPI-Agent vX.Y`.

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

## Generated inventory samples
 * HTC Win-CE based Phone:
```
<?xml version="1.0" encoding="UTF-8"?>
<REQUEST>
  <CONTENT>
    <BIOS>
      <IMEI>357559019934433</IMEI>
      <SMANUFACTURER>HTC</SMANUFACTURER>
      <SMODEL>Touch Diamond P3700 PocketPC</SMODEL>
      <TYPE>Embedded Terminal</TYPE>
    </BIOS>
    <HARDWARE>
      <NAME>Touch_Diamond</NAME>
      <OSNAME>Windows CE OS</OSNAME>
      <OSVERSION>5.2.20764</OSVERSION>
      <UUID>a220b14b97d03347579cb4430611cbffe76e1c4c</UUID>
    </HARDWARE>
    <NETWORKS>
      <DESCRIPTION>TNETW12511</DESCRIPTION>
      <IPADDRESS>192.168.122.101</IPADDRESS>
      <IPMASK>255.255.255.0</IPMASK>
      <MACADDR>00:18:41:a3:66:f1</MACADDR>
      <MODEL>TNETW12511</MODEL>
      <TYPE>ethernet</TYPE>
    </NETWORKS>
    <VERSIONCLIENT>GLPI-Agent v1.2</VERSIONCLIENT>
  </CONTENT>
  <DEVICEID>Touch_Diamond-2016-07-07-18-32-18</DEVICEID>
  <QUERY>INVENTORY</QUERY>
</REQUEST>
```
 * Datalogic Skorpio X3:
```
<?xml version="1.0" encoding="UTF-8"?>
<REQUEST>
  <CONTENT>
    <ACCOUNTINFO>
      <KEYNAME>TAG</KEYNAME>
      <KEYVALUE>dev</KEYVALUE>
    </ACCOUNTINFO>
    <BIOS>
      <BVERSION>Firmware 1.90</BVERSION>
      <SMANUFACTURER>Datalogic</SMANUFACTURER>
      <SMODEL>SkorpioX3</SMODEL>
      <SSN>G14I70443</SSN>
      <TYPE>Embedded Terminal</TYPE>
    </BIOS>
    <HARDWARE>
      <NAME>SkorpioX3</NAME>
      <OSNAME>Windows CE OS</OSNAME>
      <OSVERSION>6.0.0</OSVERSION>
      <UUID>3b64278050af33031c6cc83668e664c6e9336333</UUID>
    </HARDWARE>
    <NETWORKS>
      <DESCRIPTION>SDCSD30AG1</DESCRIPTION>
      <IPADDRESS>192.168.122.62</IPADDRESS>
      <IPMASK>255.255.255.0</IPMASK>
      <MACADDR>00-17-23-ab-2d-cc</MACADDR>
      <MODEL>SDCSD30AG1</MODEL>
      <TYPE>ethernet</TYPE>
    </NETWORKS>
    <VERSIONCLIENT>GLPI-Agent v1.1</VERSIONCLIENT>
  </CONTENT>
  <DEVICEID>SkorpioX3-2016-05-23-10-27-54</DEVICEID>
  <QUERY>INVENTORY</QUERY>
</REQUEST>
```

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
