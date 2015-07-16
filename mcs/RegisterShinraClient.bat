:: @ECHO OFF

SET GAME_DIR=%~dp0
SET PLATFORM=x32
SET CONFIG=Release
SET PLATFORM_DIR=%GAME_DIR%Shinra\%PLATFORM%\%CONFIG%\
SET CLIENT_PATH=%PLATFORM_DIR%ShinraClient.exe

reg add HKCR\shinra /f /ve /t REG_SZ /d "URL:Shinra Protocol"
reg add HKCR\shinra /f /v "URL Protocol" /t REG_SZ /d ""
reg add HKCR\shinra\DefaultIcon /f /ve /t REG_SZ /d "%CLIENT_PATH%,1"  
reg add HKCR\shinra\shell\open\command /f /ve /t REG_SZ /d "\"%CLIENT_PATH%\" \"%%1\""
