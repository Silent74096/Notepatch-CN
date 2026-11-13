#define MyAppName "Notepatch 汉化增强版"
#define MyAppVersion "1.0.6"
#define MyAppPublisher "Silent74096"
#define MyAppURL "https://github.com/Silent74096/Notepatch-CN"
#define MyAppExeName "Notepatch.exe"

[Setup]
AppId={{8F3A2C1E-9B4D-4E6F-A5C7-1D2E3F4A5B6C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\Notepatch
DisableProgramGroupPage=yes
OutputDir=.
OutputBaseFilename=Notepatch-Setup-1.0.6
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
SetupIconFile=Notepatch.ico
UninstallDisplayIcon={app}\Notepatch.exe
VersionInfoVersion=1.0.6.0
MinVersion=10.0

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"

[Files]
Source: "Notepatch.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Notepatch"; Filename: "{app}\Notepatch.exe"

[Registry]
; IFEO 重定向：打开 notepad.exe 时改为启动 Notepatch；卸载时自动移除
Root: HKLM; Subkey: "Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe"; ValueType: string; ValueName: "Debugger"; ValueData: """{app}\Notepatch.exe"""; Flags: uninsdeletevalue
; Windows 11 兼容：删除 UseFilter 过滤器（存在时商店版记事本会绕过重定向）
Root: HKLM; Subkey: "Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe"; ValueType: none; ValueName: "UseFilter"; Flags: deletevalue uninsdeletevalue

[Code]
function IsStoreNotepadInstalled: Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec('powershell.exe',
    '-NoProfile -Command "if (Get-AppxPackage -Name Microsoft.WindowsNotepad -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }"',
    '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    if IsStoreNotepadInstalled then
    begin
      if MsgBox('检测到系统安装了商店版新记事本（Windows 11 自带）。' + #13#10 +
                '它会拦截记事本的启动，导致 Notepatch 无法生效。' + #13#10#13#10 +
                '是否现在卸载它？以后可随时在应用商店搜索 "Windows 记事本" 重装。',
                mbConfirmation, MB_YESNO) = IDYES then
      begin
        Exec('powershell.exe',
          '-NoProfile -Command "Get-AppxPackage Microsoft.WindowsNotepad | Remove-AppxPackage"',
          '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
      end
      else
      begin
        MsgBox('已保留商店版。注意：双击 .txt 和开始菜单的"记事本"仍会打开商店版。' + #13#10 +
               '如需让 Notepatch 接管 .txt，请到 设置 - 应用 - 默认应用 中手动更改。',
               mbInformation, MB_OK);
      end;
    end;
  end;
end;
