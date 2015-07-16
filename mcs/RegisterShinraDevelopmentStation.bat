:: @ECHO OFF

SET CURDIR=%~dp0
SET SDSPATH=%CURDIR%ShinraDevelopmentStation.exe

reg add HKCR\.shinra /f /ve /t REG_SZ /d "shinra_auto_file"
reg add HKCR\shinra_auto_file /f /ve /t REG_SZ /d "Shinra Project"
reg add HKCR\shinra_auto_file\DefaultIcon /f /ve /t REG_SZ /d "%SDSPATH%,1"  
reg add HKCR\shinra_auto_file\shell\open\command /f /ve /t REG_SZ /d "\"%SDSPATH%\" \"%%1\""
reg add HKCR\ShinraDevelopmentStation.exe\shell\open\command /f /ve /t REG_SZ /d "\"%SDSPATH%\" \"%%1\""
