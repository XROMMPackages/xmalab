; Ensure 64-bit installation
!include "x64.nsh"

; Single source of truth for the version shown by the installer. Keep in step with
; PROJECT_VERSION / PROJECT_BETA_VERSION in CMakeLists.txt and the release tag.
!define VERSION "3.0.0-beta4"
!define VERSION_NUMERIC "3.0.0.4" ; major.minor.patch.beta, required X.X.X.X form

OutFile "XMALab-v${VERSION}.Windows.x64.Setup.exe"

; Version resource of the setup executable itself (Properties > Details)
VIProductVersion "${VERSION_NUMERIC}"
VIAddVersionKey "ProductName" "XMALab"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "XMALab ${VERSION} installer"
VIAddVersionKey "CompanyName" "XROMM"
VIAddVersionKey "LegalCopyright" "Copyright (c) 2015, Brown University. GPL v3."
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
    File /r "D:\XMALab code\xmalab\build\windows\bin\Release\*"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    CreateShortcut "$SMPROGRAMS\XMALab.lnk" "$INSTDIR\XMALab.exe"
    ; Register in Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayName" "XMALab"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayIcon" "$INSTDIR\XMALab.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "Publisher" "XROMM"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XMALab" "DisplayVersion" "${VERSION}"
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