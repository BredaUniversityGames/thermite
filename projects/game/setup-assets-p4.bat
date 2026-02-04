@echo off
:: =============================================================================
:: P4 Assets Workspace Setup
:: =============================================================================
:: Generated with AI assistance
:: First prompt: "is there a script or something that somebody could run to 
::               make it work? they all have their own path folder and 
::               workspace view, I want something that would work for the 
::               whole team"
:: Date: February 4, 2026
:: Model: Claude Opus 4.5
:: =============================================================================

powershell -ExecutionPolicy Bypass -Command ^
  "$ws = (p4 info | Select-String 'Client name').ToString().Split(':')[1].Trim(); " ^
  "$spec = p4 client -o | ForEach-Object { if ($_ -match '^View:') { \"View:`n`t//Y2526-Y3-CustomTech/Thermite/projects/game/assets/... //$ws/assets/...\" } elseif ($_ -notmatch '^\t//') { $_ } }; " ^
  "$spec | p4 client -i; " ^
  "p4 sync"

echo.
echo Done!
pause