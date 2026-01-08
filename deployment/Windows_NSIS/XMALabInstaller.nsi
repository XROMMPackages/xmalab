; Ensure 64-bit installation
!include "x64.nsh"

OutFile "XMALab-Setup.exe"
InstallDir "$PROGRAMFILES64\XMALab"
InstallDirRegKey HKLM "Software\XMALab" "Install_Dir"
RequestExecutionLevel admin

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Install"
  ; Check if running on 64-bit Windows
  ${If} ${RunningX64}
    SetOutPath $INSTDIR
    ; Recursively include everything from the Release folder
    File /r "..\..\build\bin\Release\*"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    CreateShortcut "$SMPROGRAMS\XMALab.lnk" "$INSTDIR\XMALab.exe"
    ; Register in Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayName" "XMALab"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayIcon" "$INSTDIR\XMALab.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "Publisher" "XROMM"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayVersion" "2.2.4"
  ${Else}
    MessageBox MB_OK "This application requires a 64-bit version of Windows."
    Abort
  ${EndIf}
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$SMPROGRAMS\XMALab.lnk"
  RMDir /r "$INSTDIR"
  ; Remove from Add/Remove Programs
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab"
SectionEnd