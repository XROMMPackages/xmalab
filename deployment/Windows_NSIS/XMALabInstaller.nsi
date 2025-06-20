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
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$SMPROGRAMS\XMALab.lnk"
  RMDir /r "$INSTDIR"
SectionEnd