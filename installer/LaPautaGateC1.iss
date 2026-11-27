; Instalador de La Pauta Gate C1 (VST3, Windows 64 bits) - Inno Setup 6

#define MyAppName      "La Pauta Gate C1"
#define MyAppVersion   "1.0.0"
#define MyAppPublisher "La Pauta"
#define VstBundle      "La Pauta Gate C1.vst3"

; Carpeta donde CMake deja el plugin compilado (relativa a este archivo)
#ifndef BuildDir
  #define BuildDir "..\build\LaPautaGateC1_artefacts\Release\VST3"
#endif

[Setup]
AppId={{8F3A6C52-1B7D-4E39-A4C8-2D5E9B71F640}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
; Esta carpeta solo guarda el desinstalador. El plugin va a la carpeta VST3 comun.
DefaultDirName={autopf}\La Pauta\Gate C1
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=output
OutputBaseFilename=LaPauta_GateC1_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName={#MyAppName} (VST3)

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "{#BuildDir}\{#VstBundle}\*"; DestDir: "{commoncf64}\VST3\{#VstBundle}"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#VstBundle}"
