[Files]
Source: "{code:GetSourceDrive}TonicTrouble.exe"; DestDir: "{app}"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}videos\EDPROL.AVI"; DestDir: "{app}\videos"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}DLL\*"; DestDir: "{app}\DLL"; Flags: external ignoreversion createallsubdirs recursesubdirs; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}Demos\*"; DestDir: "{app}\Demos"; Flags: external ignoreversion createallsubdirs recursesubdirs; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\OPTIONS\*"; DestDir: "{app}\GAMEDATA\OPTIONS"; Flags: external ignoreversion createallsubdirs recursesubdirs; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\WORLD\LEVELS\*"; DestDir: "{app}\GAMEDATA\WORLD\LEVELS"; Flags: external ignoreversion createallsubdirs recursesubdirs; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\WORLD\SOUND\*"; DestDir: "{app}\GAMEDATA\WORLD\SOUND"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\WORLD\SOUND\English\*"; DestDir: "{app}\GAMEDATA\WORLD\SOUND"; Flags: external ignoreversion skipifsourcedoesntexist; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\WORLD\SOUND\French\*"; DestDir: "{app}\GAMEDATA\WORLD\SOUND"; Flags: external ignoreversion skipifsourcedoesntexist; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\WORLD\SOUND\German\*"; DestDir: "{app}\GAMEDATA\WORLD\SOUND"; Flags: external ignoreversion skipifsourcedoesntexist; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\WORLD\SOUND\Italian\*"; DestDir: "{app}\GAMEDATA\WORLD\SOUND"; Flags: external ignoreversion skipifsourcedoesntexist; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\WORLD\SOUND\Spanish\*"; DestDir: "{app}\GAMEDATA\WORLD\SOUND"; Flags: external ignoreversion skipifsourcedoesntexist; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\Camera.cst"; DestDir: "{app}\GAMEDATA"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\fix_bnk.bin"; DestDir: "{app}\GAMEDATA"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\game.bpg"; DestDir: "{app}\GAMEDATA"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\game.dsb"; DestDir: "{app}\GAMEDATA"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\Textures.cnt"; DestDir: "{app}\GAMEDATA"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\tt1.cnt"; DestDir: "{app}\GAMEDATA"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;
Source: "{code:GetSourceDrive}GAMEDATA\Vignette.cnt"; DestDir: "{app}\GAMEDATA"; Flags: external ignoreversion; Check: not IsAlreadyInstalled;

[Icons]
Name: "{group}\Tonic Trouble"; Filename: "{app}\TonicTrouble.exe"; WorkingDir: "{app}"
Name: "{group}\Tonic Trouble - Configuration"; Filename: "{app}\ttconfig.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall Tonic Trouble (+Fix)"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: files; Name: "{app}\*"

[Code]
function IsAlreadyInstalled: Boolean;
begin
  Result:=RegValueExists(HKEY_AUTO,'Software\Microsoft\Windows\CurrentVersion\Uninstall\Tonic Trouble_is1', 'DisplayVersion');
end;

// Pascal script from Bgbennyboy to pull files off a CD, greatly trimmed up 
// and modified to support ANSI and Unicode Inno Setup by Triangle717.
var
	SourceDrive: string;
  
#include "FindDisc.iss"

function GetSourceDrive(Param: String): String;
begin
	Result:=SourceDrive;
end;

procedure InitializeWizard();
begin
  if not IsAlreadyInstalled() then
    SourceDrive:=GetSourceCdDrive()
  else
    SourceDrive:='';
end;
