Name "WendzelNNTPd"

OutFile "WendzelNNTPd Setup.exe"

InstallDir $PROGRAMFILES\WendzelNNTPd

LicenseText "Press Page Down to see the rest of the agreement. If you accept the terms of the agreement, click I Agree to continue. You must accept the agreement to install WendzelNNTPd."
LicenseData "LICENSE"

Page license
Page directory
Page instfiles

Section ""

  SetOutPath $INSTDIR

  File bin\wendzelnntpADM.exe
  File bin\wendzelnntpD.exe
  File gui\src\release\wendzelnntpGUI.exe
  File bin\pthreadGC2.dll
  File bin\rx.dll
  File bin\mingwm10.dll
  File bin\QtCore4.dll
  File bin\QtGui4.dll
  File log.txt
  File start_daemon.bat
  File CONTRIBUTE
  File HISTORY
  File FAQ.txt
  File LICENSE.txt
  File README_win32.txt
  File wendzelnntpd.conf
  
;IfFileExists "$INSTDIR\usenet.db" LeaveUsenetDB
  File usenet.db
;LeaveUsenetDB:

SectionEnd
