; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Coolkey
AppId=Coolkey
AppVerName=Coolkey 1.0.0-2
AppPublisher=Red Hat
CreateAppDir=true
Compression=lzma
SolidCompression=true
MinVersion=0,5.0.2195
ShowLanguageDialog=yes
OutputBaseFilename=CoolkeySetup-1.0.0-2.win64.x64
DefaultDirName={pf}\Red Hat\Coolkey
DisableProgramGroupPage=false
DefaultGroupName=Red Hat
SetupIconFile=..\src\app\xpcom\tray\esc.ico
UninstallDisplayIcon={app}\esc.ico
WizardSmallImageFile=..\src\app\xpcom\tray\esc.bmp
AllowNoIcons=yes
LicenseFile=esc-license.txt
InfoBeforeFile=coolkey-64-info-before.txt
PrivilegesRequired=admin
VersionInfoVersion=1.0.0.2
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
;Source: BUILD\regcerts.exe; DestDir: {app}
Source: ..\src\app\xpcom\tray\esc.ico; DestDir: {app}
Source: BUILD\clkcsp.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\cspres.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\clkcsp.sig; DestDir: {sys}
Source: BUILD\coolkeypk11.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\zlibwapi.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\libckyapplet-1.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\vcredist_x64.exe; DestDir: "{tmp}"


; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Run]


Filename: {tmp}\vcredist_x64.exe;  Parameters: "/q:a"; Flags: skipifdoesntexist; StatusMsg: "Installing Microsoft Visual C++ Redistributable Package"
Filename: {sys}\regsvr32 ; Parameters: "/s {sys}\clkcsp.dll" 

[UninstallRun]
Filename: {sys}\regsvr32 ; Parameters: "/u /s {sys}\clkcsp.dll"
[_ISTool]
OutputExeFilename=BUILD\coolkey-setup.exe
UseAbsolutePaths=false
LogFile=inst.log
LogFileAppend=false
[Registry]
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Defaults\Provider\CoolKey PKCS #11 CSP; ValueType: string; ValueName: PKCS11Module; ValueData: coolkeypk11.dll; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Axalto Developer; ValueType: binary; ValueName: ATRMask; ValueData: ff ff ff ff ff ff ff ff 00 00; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Axalto Developer; ValueType: string; ValueName: Crypto Provider; ValueData: CoolKey PKCS #11 CSP
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Axalto Developer; ValueType: binary; ValueName: ATR; ValueData: 3b 75 94 00 00 62 02 02 00 00

; Now register the Gemalto 64K V2
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Gemalto 64K V2; ValueType: binary; ValueName: ATRMask; ValueData: ff ff 00 ff 00 ff ff ff 00 00; Flags: uninsdeletekey

Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Gemalto 64K V2; ValueType: string; ValueName: Crypto Provider; ValueData: CoolKey PKCS #11 CSP

Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Gemalto 64K V2; ValueType: binary; ValueName: ATR; ValueData: 3b 95 00 40 00 ae 01 03 00 00

; Now register the Safenet 330J 
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Safenet 330J; ValueType: binary; ValueName: ATRMask; ValueData: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 00 00; Flags: uninsdeletekey

Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Safenet 330J; ValueType: string; ValueName: Crypto Provider; ValueData: CoolKey PKCS #11 CSP

Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Safenet 330J; ValueType: binary; ValueName: ATR; ValueData: 3b ec 00 ff 81 31 fe 45 a0 00 00 00 56 33 33 30 4a 33 06 00 00


Root: HKLM; Subkey: Software\
; Turn off the "pick a cert" dialog box
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Internet Settings\Zones\3; ValueType: dword; ValueName: 1A04; ValueData: 0
; Enable TLS 1.0
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Internet Settings; ValueType: dword; ValueName: SecureProtocols; ValueData: 168
