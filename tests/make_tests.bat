@echo off

set INPUT=lzsa_test_*.plain
set OUTPUT=tests_data.c
set OUTPUT_TMP=%OUTPUT%.tmp

echo // Auto-generated with %0 on %DATE% %TIME% > "%OUTPUT_TMP%"

for %%F in (%INPUT%) do (
	echo /******************************************************************************/ >> "%OUTPUT_TMP%"
	
	rem Compress input file to raw blocks in both LZSA1 and LZSA2 formats.
	..\tools\lzsa.exe -v -stats -f1 -r "%%F" "%%~nF.lzsa1"
	..\tools\lzsa.exe -v -stats -f2 -r "%%F" "%%~nF.lzsa2"
	
	rem Format input and compressed data files as C-style hex arrays and append to output.
	..\tools\xxd.exe -i "%%F" >> "%OUTPUT_TMP%"
	..\tools\xxd.exe -i "%%~nF.lzsa1" >> "%OUTPUT_TMP%"
	..\tools\xxd.exe -i "%%~nF.lzsa2" >> "%OUTPUT_TMP%"
)

rem Munge temp output file with AWK script into final output. Delete temp file.
..\tools\gawk-3.1.6-1-bin\bin\gawk.exe -f modify_tests.awk "%OUTPUT_TMP%" > "%OUTPUT%"
del "%OUTPUT_TMP%"
