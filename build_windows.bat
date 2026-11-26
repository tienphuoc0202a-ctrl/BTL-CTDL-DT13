@echo off
chcp 65001 > nul
echo Dang bien dich He thong Print Spooler (DT13 - Ho Tien Phuoc)...
g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp -o print_spooler.exe
if %errorlevel% neq 0 (
    echo [LOI] Bien dich that bai! Kiem tra lai MinGW/g++.
    pause
    exit /b 1
)
echo [THANH CONG] Da tao file print_spooler.exe
echo Dang chay chuong trinh...
print_spooler.exe
pause
