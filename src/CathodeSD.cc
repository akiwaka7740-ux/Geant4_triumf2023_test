#include "G4OpticalPhoton.hh"
#include "G4EventManager.hh"
#include "G4OpBoundaryProcess.hh"



#include "CathodeSD.hh"
#include "EventAction.hh"
#include "AnalysisOutput.hh"




CathodeSD::CathodeSD(G4String name)
 : G4VSensitiveDetector(name),
   fPhotonArrivedCount{0, 0},
   fPhotonDetectedCount{0, 0}
{
}

void CathodeSD::Initialize(G4HCofThisEvent*) {
    fPhotonArrivedCount[0] = 0;
    fPhotonArrivedCount[1] = 0;
    fPhotonDetectedCount[0] = 0;
    fPhotonDetectedCount[1] = 0;
}


G4bool CathodeSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {

    //G4cout << "CathodeSD::ProcessHits" << G4endl;

    // 光学光子以外がカソードに迷い込んだら無視
    if (aStep->GetTrack()->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) {
        return false;
    }

    //G4cout << "good" << G4endl;

    // 「ステップのゴール地点がボリュームの境界」であり、かつ「隣のパーツ名が Cathode」であるか？
    if (aStep->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) {
        auto nextVol = aStep->GetPostStepPoint()->GetPhysicalVolume();
        if (nextVol && nextVol->GetName() == "Cathode") {

            G4int copyNo = aStep->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber();

            // copyNoが1 or 2　なので、0 or 1に変換する
            G4int pmtIndex = copyNo - 1; 

            if (pmtIndex < 0 || pmtIndex > 1) return false; // copyNoが0か1以外なら無視

            fPhotonArrivedCount[pmtIndex]++; 

            auto eventAction = static_cast<EventAction*>(G4EventManager::GetEventManager()->GetUserEventAction());
            eventAction->AddHitTime(pmtIndex, aStep->GetPostStepPoint()->GetGlobalTime());

            G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
            eventAction->AddHitPos(pmtIndex, pos.x(), pos.y(), pos.z());

            aStep->GetTrack()->SetTrackStatus(fStopAndKill); // 光子を停止させる

        }
    }
    
    return true;

}

void CathodeSD::EndOfEvent(G4HCofThisEvent*) {
    
    auto eventAction = static_cast<EventAction*>(G4EventManager::GetEventManager()->GetUserEventAction());

    if(!eventAction) return;

    auto output = eventAction->GetAnalysisOutput();
    if(!output) return;

    output->FillPMTPhotons(fPhotonArrivedCount[0], fPhotonArrivedCount[1]);
}