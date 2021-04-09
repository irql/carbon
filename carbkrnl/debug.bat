@echo off
set path=%path%;C:\Program Files (x86)\VMware\VMware Workstation;C:\Program Files\qemu
cd ..
vmrun -T ws stop "%cd%\vm\vmware\carbon_v2.vmx"
echo select vdisk file="%cd%\carbon_v2.vhd">dprt
echo attach vdisk>>dprt
diskpart /s %cd%\dprt
cd x64\Release
copy carbkrnl.sys T:\SYSTEM\CARBKRNL.SYS
copy pci.sys T:\SYSTEM\BOOT\PCI.SYS
copy pciide.sys T:\SYSTEM\BOOT\PCIIDE.SYS
copy partmgr.sys T:\SYSTEM\BOOT\PARTMGR.SYS
copy fat.sys T:\SYSTEM\BOOT\FAT.SYS
copy kdcom.sys T:\SYSTEM\BOOT\KDCOM.SYS
copy ahci.sys T:\SYSTEM\BOOT\AHCI.SYS
copy ntuser.sys T:\SYSTEM\NTUSER.SYS
copy dxgi.sys T:\SYSTEM\DXGI.SYS
copy vmsvga.sys T:\SYSTEM\VMSVGA.SYS
copy i8042.sys T:\SYSTEM\I8042.SYS
copy d3d.dll T:\SYSTEM\D3D.DLL
copy ntdll.dll T:\SYSTEM\NTDLL.DLL
copy user.dll T:\SYSTEM\USER.DLL
copy carbinit.exe T:\SYSTEM\CARBINIT.EXE
copy explorer.exe T:\SYSTEM\EXPLORER.EXE
copy vesa.sys T:\SYSTEM\VESA.SYS
copy ..\..\hi.txt T:\SYSTEM\HI.TXT
copy ..\..\ports\freetype\freetype.dll T:\SYSTEM\FREETYPE.DLL
copy "..\..\fonts\VICTOR MONO.TTF" "T:\SYSTEM\FONTS\VICTOR MONO.TTF"
copy ..\..\fonts\ARIAL.TTF T:\SYSTEM\FONTS\ARIAL.TTF
copy ..\..\fonts\MICROSS.TTF T:\SYSTEM\FONTS\MICROSS.TTF
copy ..\..\fonts\IBMVGA.BIN T:\SYSTEM\FONTS\IBMVGA.BIN
copy ..\..\carbkrnl\config\SYSTEM T:\SYSTEM\SYSTEM
cd ..\..\
echo select vdisk file="%cd%\carbon_v2.vhd">dprt
echo detach vdisk>>dprt
diskpart /s %cd%\dprt
del dprt
taskkill /f /im kdwin32.exe

!vmrun -T ws start "%cd%\vm\vmware\carbon_v2.vmx"
!.\x64\Release\kdwin32.exe
!pause
!vmrun -T ws stop "%cd%\vm\vmware\carbon_v2.vmx"

rem vmrun -T ws reset "%cd%\vm\vmware\carbon_v2.vmx"

qemu-system-x86_64 -s -d int -D Pog.TXT -m 1024 -smp 1 -no-reboot -no-shutdown -monitor stdio -drive id=pog,format=raw,media=disk,file=%cd%\carbon_v2.vhd,if=none -device ahci,id=ahci -device ide-drive,drive=pog,bus=ahci.0
rem qemu-system-x86_64 -s -m 1024 -smp 4 -no-reboot -no-shutdown -monitor stdio -vga vmware -drive format=raw,media=disk,file=%p%\carbon_v2.vhd -chardev pipe,id=pog,path=KdPipe -serial chardev:pog
pause