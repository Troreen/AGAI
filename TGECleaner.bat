@echo off

set OUTDIR=SOLUTION

robocopy "Bin" "%OUTDIR%\exe" /E /XF "*.idb" "*.lib" "*.pdb"

robocopy "EngineAssets" "%OUTDIR%\exe\EngineAssets" /E
powershell -Command "(Get-Content %OUTDIR%\exe\settings\Game.json) -replace '\.\./(?=EngineAssets)','' | Set-Content %OUTDIR%\exe\settings\Game.json"

robocopy "Bin" "%OUTDIR%\source\Bin" /E /XF "*.idb" "*.lib" "*.pdb" "*.exe"

robocopy . "%OUTDIR%\source" /E /XF "%~nx0" /XD "%CD%\Lib" "%CD%\Bin" "%CD%\Doc" "%CD%\Temp" "%CD%\.vs" "%CD%\%OUTDIR%" "%CD%\Source\Tutorials"
