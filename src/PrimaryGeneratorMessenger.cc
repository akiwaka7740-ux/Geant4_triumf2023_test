#include "PrimaryGeneratorMessenger.hh"
#include "PrimaryGenerator.hh"
#include "G4UIdirectory.hh"       // 【修正】インクルードを追加
#include "G4UIcmdWithAString.hh"  // 【修正】インクルードを追加

// 【修正】クラス名を Messanger から Messenger に統一
PrimaryGeneratorMessenger::PrimaryGeneratorMessenger(PrimaryGenerator *action) : fAction(action)
{
    // コマンドディレクトリの作成
    fDir = new G4UIdirectory("/mygen/"); 
    fDir->SetGuidance("Primary generator control commands.");

    // コマンドの作成
    fSourceCmd = new G4UIcmdWithAString("/mygen/sourceType", this); 
    // 指定したコマンドが入力されたときは、このクラス自身に通知されるようにする
    fSourceCmd->SetGuidance("Select soruce type: neutron, 137Cs, 90Sr");
    fSourceCmd->SetCandidates("neutron 137Cs 90Sr");

    fSourceCmd->SetParameterName("type",false);
    fSourceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

PrimaryGeneratorMessenger::~PrimaryGeneratorMessenger()
{
    delete fSourceCmd;
    delete fDir;
}

void PrimaryGeneratorMessenger::SetNewValue(G4UIcommand *command, G4String newValue)
{
    if (command == fSourceCmd) {
        fAction->SetSourceType(newValue);
    }
}