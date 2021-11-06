@echo off
set OUTPUT=lzsa_tests_all.c
echo // Auto-generated with %0 > "%OUTPUT%"
for %%F in (*.txt, *.bin) do (
	..\tools\lzsa.exe -v -stats -f1 -r "%%F" "%%~nF.lzsa1"
	..\tools\xxd.exe -i "%%F" >> "%OUTPUT%"
	..\tools\xxd.exe -i "%%~nF.lzsa1" >> "%OUTPUT%"
)
