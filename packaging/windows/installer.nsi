; Hand-written in place of CPack's NSIS generator: CPack's CPACK_NSIS_*
; hooks only let you inject raw commands into a few fixed spots in its
; own template, not add new Pages, which is what a real "install for me
; / install for everyone / pick my own folder" + "add to PATH, and for
; whom" flow needs (MultiUser.nsh's install-mode page, and a custom
; nsDialogs page for the PATH choice).
;
; Expected to be invoked with these /D defines (see CMakeLists.txt):
;   NJAMX_VERSION, NJAMX_EXE, NJAMX_EXE_NAME, NJAMX_LICENSE,
;   NJAMX_NPCAP_EXE, NJAMX_OUT

!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\njamx"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${UNINSTKEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "CurrentUser"
!define MULTIUSER_INSTALLMODE_INSTDIR "njamx"
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI

!include "LogicLib.nsh"
!include "MultiUser.nsh"
!include "MUI2.nsh"
!include "WinMessages.nsh"

Name "njamx"
OutFile "${NJAMX_OUT}"

Var AddToPath
Var PathScope
Var ChkAddPath
Var RadioUser
Var RadioSystem

; --- Pages ---

!insertmacro MUI_PAGE_WELCOME
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_LICENSE "${NJAMX_LICENSE}"
!insertmacro MUI_PAGE_DIRECTORY
Page custom PathPageCreate PathPageLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Function .onInit
  !insertmacro MULTIUSER_INIT

  ; ShCtx/the remembered install mode both come from MULTIUSER_INIT above,
  ; so this checks whichever hive (HKCU/HKLM) the last install (if any)
  ; used.
  ReadRegStr $0 ShCtx "${UNINSTKEY}" "UninstallString"
  ${If} $0 != ""
    ReadRegStr $1 ShCtx "${UNINSTKEY}" "DisplayVersion"
    MessageBox MB_YESNO|MB_ICONQUESTION \
      "njamx $1 is already installed.$\r$\n$\r$\nReinstall / update it?" \
      IDYES already_installed_continue
    Abort
    already_installed_continue:
  ${EndIf}
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd

; --- Custom "add to PATH" page ---

Function PathPageCreate
  !insertmacro MUI_HEADER_TEXT "Add to PATH" "Choose whether njamx should be runnable from any terminal."

  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateCheckbox} 0 0 100% 12u "Add njamx's folder to PATH"
  Pop $ChkAddPath
  ${NSD_SetState} $ChkAddPath ${BST_CHECKED}

  ${NSD_CreateRadioButton} 10u 20u 100% 12u "My account only"
  Pop $RadioUser

  ${NSD_CreateRadioButton} 10u 35u 100% 12u "All users (system PATH)"
  Pop $RadioSystem

  ; Default the scope to match the install-mode page's choice, rather
  ; than always defaulting to "my account only".
  ${If} $MultiUser.InstallMode == "AllUsers"
    ${NSD_SetState} $RadioSystem ${BST_CHECKED}
  ${Else}
    ${NSD_SetState} $RadioUser ${BST_CHECKED}
    EnableWindow $RadioSystem 0
  ${EndIf}

  nsDialogs::Show
FunctionEnd

Function PathPageLeave
  ${NSD_GetState} $ChkAddPath $AddToPath

  ${NSD_GetState} $RadioSystem $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $PathScope "Machine"
  ${Else}
    StrCpy $PathScope "User"
  ${EndIf}
FunctionEnd

; --- Install ---

Section "Install"
  SetOutPath "$InstDir"
  File "${NJAMX_EXE}"
  File "${NJAMX_LICENSE}"
  File "update-path.ps1"

  ; Npcap always registers this uninstall entry (both 32- and 64-bit
  ; Windows redirect a 32-bit installer's HKLM reads here automatically,
  ; so no SetRegView dance is needed), regardless of which njamx install
  ; mode/hive was chosen above - Npcap itself always installs machine-wide.
  ReadRegStr $2 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\NpcapInst" "DisplayVersion"
  StrCpy $3 "1"
  ${If} $2 != ""
    MessageBox MB_YESNO|MB_ICONQUESTION \
      "Npcap $2 is already installed.$\r$\n$\r$\nReinstall it anyway?" \
      IDYES npcap_reinstall_yes
    StrCpy $3 "0"
    npcap_reinstall_yes:
  ${EndIf}

  ; Interactive on purpose - see CMakeLists.txt for why this isn't /S.
  ${If} $3 == "1"
    File "/oname=npcap-installer.exe" "${NJAMX_NPCAP_EXE}"
    ExecWait '"$InstDir\npcap-installer.exe"'
  ${EndIf}

  WriteUninstaller "$InstDir\Uninstall.exe"

  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayName" "njamx"
  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayVersion" "${NJAMX_VERSION}"
  WriteRegStr ShCtx "${UNINSTKEY}" "UninstallString" '"$InstDir\Uninstall.exe"'
  WriteRegStr ShCtx "${UNINSTKEY}" "InstallLocation" "$InstDir"
  WriteRegStr ShCtx "${UNINSTKEY}" "$MultiUser.InstallMode" "1"

  ${If} $AddToPath == ${BST_CHECKED}
    WriteRegStr ShCtx "${UNINSTKEY}" "PathScope" "$PathScope"
    nsExec::ExecToLog '"powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "$InstDir\update-path.ps1" -Dir "$InstDir" -Scope $PathScope -Action Add'
    System::Call 'user32::SendMessageTimeoutA(i -1, i ${WM_SETTINGCHANGE}, i 0, t "Environment", i 0, i 5000, *i .r0)'
  ${Else}
    WriteRegStr ShCtx "${UNINSTKEY}" "PathScope" "None"
  ${EndIf}
SectionEnd

; --- Uninstall ---

Section "Uninstall"
  ReadRegStr $0 ShCtx "${UNINSTKEY}" "PathScope"
  ${If} $0 != "None"
  ${AndIf} $0 != ""
    nsExec::ExecToLog '"powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "$InstDir\update-path.ps1" -Dir "$InstDir" -Scope $0 -Action Remove'
    System::Call 'user32::SendMessageTimeoutA(i -1, i ${WM_SETTINGCHANGE}, i 0, t "Environment", i 0, i 5000, *i .r0)'
  ${EndIf}

  Delete "$InstDir\${NJAMX_EXE_NAME}"
  Delete "$InstDir\LICENSE"
  Delete "$InstDir\update-path.ps1"
  Delete "$InstDir\npcap-installer.exe"
  Delete "$InstDir\Uninstall.exe"
  RMDir "$InstDir"

  DeleteRegKey ShCtx "${UNINSTKEY}"
SectionEnd
