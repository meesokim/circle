z80asm\z80asm -fb -l -sa spc-1000_all.asm 
z80asm\z80asm -fb -l -sa spc-1000_ipl.asm
copy /b spc-1000_all.bin+spc-1000_ipl.bin spcall.rom
copy spcall.rom ..\Debug
c:\mingw64\mingw32\msys\bin\zip c:Retro\mame\roms\spc1000.zip spcall.rom 