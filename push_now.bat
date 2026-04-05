@echo off
cd /d "C:\Users\sammi\OneDrive\Documents\PlatformIO\Projects\Care Cane"
git add .
git commit -m "Updated fall detection with 3s settling period and 5s avg delta stillness check"
git push origin main
echo.
echo Push complete!
pause
