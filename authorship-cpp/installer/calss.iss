; ─────────────────────────────────────────────────────────────
; CALSS installer (Inno Setup 6.4 or later).
;
; Built by make_installer.bat (which runs compile.bat's output checks
; first). Output: release\CALSS_Setup_<version>.exe
;
; Version, product name and description are read back out of
; authorship.exe's version resource, so include\version.h stays the only
; place the version is written.
;
; Layout and policy (details in docs\RELEASE_READINESS.md §7):
;   - Default: all users, {autopf}\CALSS (Program Files), needs admin
;     once to install. "Only for me" (no admin) is offered too and
;     installs to %LOCALAPPDATA%\Programs\CALSS.
;   - CALSS itself never needs admin (asInvoker manifest).
;   - All-users install also prepares %PROGRAMDATA%\CALSS\audit.log so
;     every Windows user can APPEND to it (and nothing more) — see
;     PrepareAuditLog below.
;   - Uninstall removes the program, its shortcuts and the Google
;     Classroom working copies in %LOCALAPPDATA%\CALSS\Classroom (of the
;     Windows account running the uninstaller). Reports, imports, the
;     audit log, other per-user settings and saved sign-ins (Credential
;     Manager) are kept.
; ─────────────────────────────────────────────────────────────

#define Root        ".."
#define AppExe      "authorship.exe"
#define AppVersion  GetStringFileInfo(Root + "\" + AppExe, "FileVersion")
#define AppDesc     GetStringFileInfo(Root + "\" + AppExe, "FileDescription")
#define AppPublisher GetStringFileInfo(Root + "\" + AppExe, "CompanyName")

#if AppVersion == ""
  #error authorship.exe has no version resource - run compile.bat first
#endif

[Setup]
; AppId identifies CALSS for upgrades and uninstall. Never change it.
AppId={{29DB3642-8C7F-4F12-B8AA-EF8226D1304B}
AppName=CALSS
AppVersion={#AppVersion}
AppVerName=CALSS {#AppVersion}
AppPublisher={#AppPublisher}
AppComments={#AppDesc}
VersionInfoVersion={#AppVersion}.0
VersionInfoProductName=CALSS
VersionInfoDescription=CALSS {#AppVersion} Setup
VersionInfoCompany={#AppPublisher}

DefaultDirName={autopf}\CALSS
DefaultGroupName=CALSS
DisableProgramGroupPage=yes
DisableWelcomePage=no
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

OutputDir={#Root}\release
OutputBaseFilename=CALSS_Setup_{#AppVersion}
SetupIconFile={#Root}\ulccslogo.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName=CALSS {#AppVersion}
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
; Close a running CALSS before replacing or removing its files.
CloseApplications=yes
RestartApplications=no
; {localappdata} is used on purpose in an all-users install: the
; uninstaller deletes the Classroom working folder of the account that
; runs it (see [UninstallDelete]).
UsedUserAreasWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; Exactly the runtime files (same set as make_release.bat's package,
; minus the tester-only text files).
Source: "{#Root}\{#AppExe}";                          DestDir: "{app}"; Flags: ignoreversion
Source: "{#Root}\ulccslogo.png";                      DestDir: "{app}"; Flags: ignoreversion
Source: "{#Root}\ulcclogoloadingscreen.png";          DestDir: "{app}"; Flags: ignoreversion
; The app's OAuth CLIENT id for Classroom sync (not a personal secret,
; not a sign-in token; see RELEASE_READINESS §1).
Source: "{#Root}\calss_gc_embedded_credentials.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Root}\assets\help\calss_help_*.png";       DestDir: "{app}\assets\help"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\CALSS"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"; Comment: "{#AppDesc}"
Name: "{autodesktop}\CALSS";  Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"; Comment: "{#AppDesc}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,CALSS}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Google Classroom working copies (downloaded submissions + their
; calss_sync_manifest.jsonl) are application-managed and go away with
; the app. ONLY this subfolder: %LOCALAPPDATA%\CALSS itself also holds
; settings and is left alone.
Type: filesandordirs; Name: "{localappdata}\CALSS\Classroom"

[Code]
// %PROGRAMDATA%\CALSS\audit.log on a shared PC.
// CALSS creates the file on first use, and the creator becomes its
// owner; other users then inherit only Read from ProgramData and their
// entries are dropped. CALSS opens the log append-only
// (FILE_APPEND_DATA), so the installer pre-creates it and grants the
// Users group (by SID, so it works on non-English Windows) exactly
// Read + Append Data on that one file. No user can overwrite, truncate,
// delete or re-permission it; the folder keeps ProgramData's defaults.
// Runs only for an all-users (admin) install.
procedure PrepareAuditLog();
var
  Dir, LogFile: String;
  RC: Integer;
begin
  Dir := ExpandConstant('{commonappdata}\CALSS');
  LogFile := Dir + '\audit.log';
  if not ForceDirectories(Dir) then begin
    Log('Could not create ' + Dir);
    Exit;
  end;
  if not FileExists(LogFile) then
    SaveStringToFile(LogFile, '', False);
  if Exec(ExpandConstant('{sys}\icacls.exe'),
          '"' + LogFile + '" /grant *S-1-5-32-545:(R,AD)',
          '', SW_HIDE, ewWaitUntilTerminated, RC) then
    Log('icacls audit.log exit code ' + IntToStr(RC))
  else
    Log('icacls could not be started');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and IsAdminInstallMode then
    PrepareAuditLog();
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    SuppressibleMsgBox(
      'CALSS has been removed.' + #13#10 + #13#10 +
      'Deleted: the Google Classroom submissions CALSS downloaded for this' + #13#10 +
      'Windows account (' + ExpandConstant('{localappdata}') + '\CALSS\Classroom).' + #13#10 + #13#10 +
      'Kept on purpose (delete them yourself if you no longer need them):' + #13#10 +
      '  - Documents\CALSS Reports  (similarity reports)' + #13#10 +
      '  - Documents\CALSS Imports  (files you imported)' + #13#10 +
      '  - ' + ExpandConstant('{commonappdata}') + '\CALSS\audit.log  (audit record)' + #13#10 +
      '  - Your saved Gemini key and Google sign-in (Windows Credential Manager,' + #13#10 +
      '    entries starting with "CALSS/").',
      mbInformation, MB_OK, IDOK);
end;
