@echo off

set EXE="C:\Program Files (x86)\Microsoft Device Emulator\1.0\DeviceEmulator.exe"

C:
cd "\Program Files (x86)\Windows Mobile 6 SDK\PocketPC\Deviceemulation\0409"

rem ARMv4 / ARMv5
set CPU=ARMv5

set BIN=PPC_USA_GSM_VGA_VR.BIN
set XML=..\Pocket_PC_Phone_VGA\Pocket_PC_PE_VGA.xml
set MEM=128
set SHARE=C:\
set VMNAME=TEST
set VMID={7bd3f032-7ecb-4f3f-a8e2-376051dc0890}

set OPTIONS=/c /memsize %MEM% /cpucore %CPU%
set COMMONS=/sharedfolder %SHARE% /vmname %VMNAME% /vmid %VMID% /skin %XML%

%EXE% %OPTIONS% %COMMONS% %BIN%  

pause
