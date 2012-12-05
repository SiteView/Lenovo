!include "MUI2.nsh"
!define MUI_ABORTWARNING

#!define MUI_FINISHPAGE_RUN "$INSTDIR\lwrma.exe"


!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "English"

!define APPNAME "Lenovo Wireless Router Manage Assistant"
!define APPNAME_KEY "lwrma"
!define APPVERSION "1.0.169.17"

LangString u_AppName ${LANG_SIMPCHINESE} "联想无线路由管理助手"
LangString u_AppName ${LANG_ENGLISH} "Lenovo Wireless Router Manage Assistant"

LangString u_SecReq ${LANG_SIMPCHINESE} "无线路由管理助手(必需)"
LangString u_SecReq ${LANG_ENGLISH} "Wireless Router Manage Assistant (required)"

#LangString u_SecHelp ${LANG_SIMPCHINESE} "帮助文档"
#LangString u_SecHelp ${LANG_ENGLISH} "Help File"

LangString u_SecStartMenu ${LANG_SIMPCHINESE} "开始菜单快捷方式"
LangString u_SecStartMenu ${LANG_ENGLISH} "Start Menu Shortcuts"

LangString u_SecDesktopIcon ${LANG_SIMPCHINESE} "桌面快捷方式"
LangString u_SecDesktopIcon ${LANG_ENGLISH} "Desktop Shortcuts"

Name $(u_AppName)
OutFile "lwrma_setup_v${APPVERSION}.exe"
InstallDir "$PROGRAMFILES\Lenovo\Wireless Router Manage Assistant"
RequestExecutionLevel admin
XPStyle on
BrandingText "Lenovo"

InstallDirRegKey HKLM "Software\Lenovo\Wireless Router Manage Assistant" "InstallDir"

#Page components
#Page directory
#Page instfiles

#UninstPage uninstConfirm
#UninstPage instfiles

Section $(u_SecReq)
	SectionIn RO
	SetOutPath "$TEMP"
	File ISHelper.dll
	System::Call "ISHelper::ISCloseAll()"
	Delete ISHelper.dll

	SetOutPath "$INSTDIR"
	File /r ${INPUTDIR}\*
	WriteRegStr HKLM "Software\Lenovo\Wireless Router Manage Assistant" "InstallDir" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}" "DisplayName" "${APPNAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}" "DisplayVersion" "${APPVERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}" "Publisher" "Lenovo"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}" "NoRepair" 1
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "lwrma_autostart" "$INSTDIR\lwrma.exe -autostart"
	WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd

Section $(u_SecStartMenu)
	SetShellVarContext all
	CreateDirectory "$SMPROGRAMS\Lenovo\Wireless Router Manage Assistant"
	CreateShortCut "$SMPROGRAMS\Lenovo\Wireless Router Manage Assistant\Uninstall.lnk" "$INSTDIR\uninst.exe"
	CreateShortCut "$SMPROGRAMS\Lenovo\Wireless Router Manage Assistant\联想无线路由管理助手.lnk" "$INSTDIR\lwrma.exe"
	CreateShortCut "$SMPROGRAMS\Lenovo\Wireless Router Manage Assistant\帮助.lnk" "$INSTDIR\help.pdf"
SectionEnd

Section $(u_SecDesktopIcon)
	SetShellVarContext all
	CreateShortCut "$DESKTOP\联想无线路由管理助手.lnk" "$INSTDIR\lwrma.exe"
SectionEnd

Section "Uninstall"
	SetOutPath "$TEMP"
	File ISHelper.dll
	System::Call "ISHelper::ISCloseAll()"
	Delete ISHelper.dll
	SetShellVarContext all
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME_KEY}"
	DeleteRegKey HKLM "Software\Lenovo\Wireless Router Manage Assistant"
	DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "lwrma_autostart"
	Delete "$DESKTOP\联想无线路由管理助手.lnk"
	RMDir /r "$SMPROGRAMS\Lenovo\Wireless Router Manage Assistant"
	RMDir /r "$INSTDIR"
SectionEnd
