

















@echo off
Setlocal enabledelayedexpansion
@echo ********************************************************************************
@echo SDK BR56
@echo ********************************************************************************
@echo %date%


cd /d %~dp0

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf



set compress_tool=.\lz4_packet.exe


REM %OBJDUMP% -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .data_code %ELFFILE% data_code.bin
%OBJCOPY% -O binary -j .data_code_z %ELFFILE% data_code_z.bin
%OBJCOPY% -O binary -j .overlay_init %ELFFILE% init.bin
%OBJCOPY% -O binary -j .overlay_aec %ELFFILE% aec.bin
%OBJCOPY% -O binary -j .overlay_aac %ELFFILE% aac.bin

for /L %%i in (0,1,20) do (
            %OBJCOPY% -O binary -j .overlay_bank%%i %ELFFILE% bank%%i.bin
                set bankfiles=!bankfiles! bank%%i.bin 0xbbaa
        )


%compress_tool% -dict text.bin -input data_code_z.bin 0 init.bin 0 aec.bin 0 aac.bin 0 !bankfiles! -o compress.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
REM %OBJDUMP% -t %ELFFILE% > symbol_tbl.txt

copy /b text.bin + data.bin + data_code.bin + compress.bin app.bin

del !bankfiles! data_code_z.bin text.bin data.bin compress.bin


set TONE_EN_ENABLE=1







set TONE_ZH_ENABLE=0
del download\earphone\ALIGN_DIR\anc_ext.bin


call download/earphone/download.bat
