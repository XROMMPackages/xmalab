OutFile "XMALab-Setup.exe"
InstallDir "$PROGRAMFILES\XMALab"
InstallDirRegKey HKLM "Software\XMALab" "Install_Dir"
RequestExecutionLevel admin

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Install"
  SetOutPath $INSTDIR
  ; Recursively include everything from the Release folder
  File /r "..\..\build\bin\Release\*"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\XMALab.lnk" "$INSTDIR\XMALab.exe"
  ; Register in Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayName" "XMALab"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayIcon" "$INSTDIR\XMALab.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "Publisher" "Peter Falkingham"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayVersion" "2.2.0"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$SMPROGRAMS\XMALab.lnk"
  RMDir /r "$INSTDIR"
  ; Remove from Add/Remove Programs
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab"
SectionEnd