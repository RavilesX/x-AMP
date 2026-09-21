; Inno Setup script for the x-AMP Windows installer.
;
; It packages the tree the CI stages -- the executable, the Qt runtime, the
; MinGW runtime and x-AMP's own plugins -- and nothing else. The staging is
; where the work is (see .github/actions/windows-build); by the time this runs
; the directory already starts on its own, so the installer's only job is to
; put it somewhere, make shortcuts, and be removable again.
;
; Built by the release workflow as:
;   iscc /DAppVersion=1.1.0 /DStageDir=<abs path to stage> packaging\windows\xamp.iss
;
; Nothing here is generated, so it can be run by hand against a local stage to
; try a change without waiting for CI.

#ifndef AppVersion
  #error AppVersion must be passed with /DAppVersion=x.y.z
#endif
#ifndef StageDir
  #define StageDir "..\..\stage"
#endif
; Where COPYING, README.md and the icons are read from. Separate from StageDir
; because the staged tree is an install prefix and holds neither.
#ifndef SourceRoot
  #define SourceRoot "..\.."
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

#define AppName      "x-AMP"
#define AppPublisher "Ricardo Aviles Sanders"
#define AppURL       "https://github.com/RavilesX/x-AMP"
#define AppExeName   "xamp.exe"

[Setup]
; A fixed GUID is what lets an install replace the one before it rather than
; piling up in Add/Remove Programs. It must never change again.
AppId={{8F3A6C21-7D4E-4B19-9A52-1C0E5D7B3F84}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
VersionInfoVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
; The licence x-AMP inherits from Qmmp. Shown because it is GPL-2+ and the
; user is entitled to read it before the files land.
LicenseFile={#SourceRoot}\COPYING
OutputDir={#OutputDir}
OutputBaseFilename=x-amp-{#AppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#SourceRoot}\src\app\images\ico\qmmp.ico
UninstallDisplayIcon={app}\bin\{#AppExeName}
UninstallDisplayName={#AppName} {#AppVersion}
; Per-user by default, so the wizard never raises a UAC prompt. Someone who
; runs it elevated gets a machine-wide install instead; "lowest" only sets
; what it asks for, not what it accepts.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; The build is MinGW x86_64 and there is no 32-bit one, so say so rather than
; letting it install somewhere it cannot run.
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
DisableProgramGroupPage=yes
ShowLanguageDialog=auto
; A running player holds its DLLs open, and an upgrade over the top would
; fail halfway and leave a mixed tree. This asks the Restart Manager to close
; it first; the player is not started again afterwards, since the wizard's
; last page offers that.
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"

[CustomMessages]
en.CreateDesktopIcon=Create a &desktop shortcut
es.CreateDesktopIcon=Crear un acceso directo en el &escritorio
en.AssociateFiles=Associate audio files with x-AMP
es.AssociateFiles=Asociar archivos de audio con x-AMP
en.AssociateNote=Only the formats x-AMP ships a decoder for.
es.AssociateNote=Solo los formatos para los que x-AMP trae decodificador.
en.LaunchApp=Open {#AppName} now
es.LaunchApp=Abrir {#AppName} ahora

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
; Unchecked on purpose: taking over someone's audio associations without
; being asked is the kind of thing a player gets uninstalled for.
Name: "associate"; Description: "{cm:AssociateFiles}"; GroupDescription: "{cm:AssociateNote}"; Flags: unchecked

[Files]
; The whole staged tree, minus what only a developer would want. The headers
; and import libraries are installed by CMake because the libraries are
; public; they are dead weight in a player.
Source: "{#StageDir}\bin\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\lib\qmmp-*"; DestDir: "{app}\lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\share\*"; DestDir: "{app}\share"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#SourceRoot}\COPYING"; DestDir: "{app}"; DestName: "COPYING.txt"; Flags: ignoreversion
Source: "{#SourceRoot}\README.md"; DestDir: "{app}"; DestName: "README.md"; Flags: ignoreversion
Source: "{#SourceRoot}\src\app\images\ico\qmmp_file.ico"; DestDir: "{app}"; DestName: "audio-file.ico"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\bin\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\bin\{#AppExeName}"; Tasks: desktopicon

[Registry]
; Written under the user's own classes, so the per-user install stays
; per-user. Registering the ProgID and the "Open with" entry is done whether
; or not the extensions are claimed, which is what lets x-AMP show up in the
; "Open with" list without becoming the default for anything.
Root: HKA; Subkey: "Software\Classes\x-AMP.AudioFile"; ValueType: string; ValueName: ""; ValueData: "Audio file"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\x-AMP.AudioFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\audio-file.ico"
Root: HKA; Subkey: "Software\Classes\x-AMP.AudioFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#AppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExeName}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#AppExeName}"" ""%1"""; Flags: uninsdeletekey

; Claimed only when the task is ticked. One line per extension rather than a
; loop: Inno has no loop here, and an explicit list is what can be checked
; against the decoders that are actually built.
Root: HKA; Subkey: "Software\Classes\.mp3";  ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.flac"; ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.ogg";  ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.oga";  ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.opus"; ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.m4a";  ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.aac";  ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.wav";  ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.wv";   ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.ape";  ValueType: string; ValueName: ""; ValueData: "x-AMP.AudioFile"; Tasks: associate; Flags: uninsdeletevalue

[Run]
Filename: "{app}\bin\{#AppExeName}"; Description: "{cm:LaunchApp}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; x-AMP keeps its settings under the user's profile; the installer never put
; them there, so they are left alone. What can go is anything the player
; wrote inside its own directory, which would otherwise leave the tree behind.
Type: filesandordirs; Name: "{app}\bin\platforms"
Type: dirifempty; Name: "{app}\bin"
Type: dirifempty; Name: "{app}\lib"
Type: dirifempty; Name: "{app}"
