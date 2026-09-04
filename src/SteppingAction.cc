#include "SteppingAction.hh"
#include "CathodeSD.hh"

#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"
#include "G4LogicalVolume.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "G4VProcess.hh"
#include "G4VPhysicalVolume.hh"

SteppingAction::SteppingAction()
 : G4UserSteppingAction()
{
}

void SteppingAction::UserSteppingAction(const G4Step* step) {

    if(step == nullptr){
        return ;
    }

    G4Track* track = step->GetTrack();

    //光学光子であるか判定
    if(track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()){
        return;
    }
    
    //初回のみ判定　
    if (fBoundary == nullptr){
        auto* processManager = track->GetDefinition()->GetProcessManager();

        if (processManager == nullptr){
            return;
        }

        auto* processList = processManager->GetProcessList();

        const G4int processCount = processManager->GetProcessListLength();

        //全プロセスから境界判定のプロセスのみ引き抜く
        for (G4int i = 0; i < processCount; i++){
            auto* process = (*processList)[i];

            if (process != nullptr && process->GetProcessName() == "OpBoundary"){
                fBoundary = dynamic_cast<G4OpBoundaryProcess*>(process);
                //境界判定が見つかれば終了
                break;
            }
        }
    }

    if (fBoundary == nullptr){
        return;
    }

    const auto* postPoint = step->GetPostStepPoint();

    // boundary statusは境界stepでのみ有効
    if (postPoint->GetStepStatus()!= fGeomBoundary) {
        return;
    }

    // 現在EFFICIENCYを持つoptical surfaceはCathodeSurfaceのみ
    if (fBoundary->GetStatus() != Detection) {
        return;
    }

    auto* postVolume = postPoint->GetPhysicalVolume();

    if (postVolume == nullptr) {
        return;
    }

    auto* sensitiveDetector = postVolume->GetLogicalVolume()->GetSensitiveDetector();

    auto* cathodeSD = dynamic_cast<CathodeSD*>(sensitiveDetector);

    if (cathodeSD == nullptr){
        return;
    }

    cathodeSD->ProcessBoundaryHit(step);
}