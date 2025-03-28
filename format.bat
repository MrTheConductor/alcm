@echo off
setlocal

rem Set the paths to the subdirectories
set DIR1=\Library\HK32F030Mxx_Library_V1.1.6\HK32F030M_Project\inc
set DIR2=\Library\HK32F030Mxx_Library_V1.1.6\HK32F030M_Project\src
set DIR3=\Library\HK32F030Mxx_Library_V1.1.6\HK32F030M_Project\tests

rem Set the path to clang-format executable
set CLANG_FORMAT=clang-format

rem Function to format files in a directory
for %%D in ("%DIR1%" "%DIR2%" "%DIR3%") do (
    echo Processing directory %%D...
    for /R %%F in (%%D\*.c %%D\*.h) do (
        echo Formatting %%F...
        "%CLANG_FORMAT%" -i "%%F"
    )
)

echo All files processed.
endlocal