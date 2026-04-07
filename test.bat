@echo off
chcp 65001 >nul

echo Начинаем проверку программы
echo Компиляция кода...
g++ -std=c++17 -O2 calculator.cpp -o calculator.exe
if %errorlevel% neq 0 (
    echo Ошибка компиляции. Проверьте исходный код.
    pause
    exit /b 1
)

echo Запуск тестов...
calculator.exe < test_advanced.txt > actual_output.txt

echo Сравнение результатов с эталоном...
fc /L actual_output.txt expected_output.txt >nul
if %errorlevel% equ 0 (
    echo Все тесты пройдены успешно.
) else (
    echo Обнаружены различия в выводе.
    fc /L actual_output.txt expected_output.txt
)

pause