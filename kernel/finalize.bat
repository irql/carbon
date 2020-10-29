

@echo off

cd ..

echo select vdisk file="%cd%\carbon_os.vhd">dp
echo attach vdisk>>dp
diskpart /s %cd%\dp

cd x64\Release

copy *.sys D:\system\*.sys
copy *.dll D:\system\*.dll
copy *.exe D:\system\*.exe
copy .\symbols\*.pdb D:\system\symbols\*.pdb
copy ..\..\assets\system\*.* D:\system\*.*

cd ..
cd ..

echo select vdisk file="%cd%\carbon_os.vhd">dp
echo detach vdisk>>dp
diskpart /s %cd%\dp

del dp
rem qemu-system-x86_64 -s -m 1024 -cpu core2duo -smp 2 -no-reboot -no-shutdown -monitor stdio -vga vmware -accel hax -serial pipe:KdPipe -drive format=raw,media=disk,file="%cd%\carbon_os.vhd"
rem pause

