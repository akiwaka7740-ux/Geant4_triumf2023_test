#include "G4OpticalPhoton.hh"
#include "CathodeSD.hh"
#include "EventAction.hh"

CathodeSD::CathodeSD(G4String name)
 : G4VSensitiveDetector(name),
   fPhotonCount(0)
{
}

void CathodeSD::Initialize(G4HCofThisEvent*) {
    fPhotonCount = 0;
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
            fPhotonCount++; // ←ここでカウント！

            auto eventAction = static_cast<EventAction*>(G4EventManager::GetEventManager()->GetUserEventAction());
            eventAction->AddHitTime(aStep->GetPostStepPoint()->GetGlobalTime());

            G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
            eventAction->AddHitPos(pos.x(), pos.y(), pos.z());

        }
    }
    return true;


    /*
    fPhotonCount++;

    //明示的な型変換
    auto eventAction = static_cast<EventAction*>(G4EventManager::GetEventManager()->GetUserEventAction());
    eventAction->AddHitTime(aStep->GetPostStepPoint()->GetGlobalTime());

    G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
    eventAction->AddHitPos(pos.x(), pos.y(), pos.z());
    */

    return true;
}

void CathodeSD::EndOfEvent(G4HCofThisEvent*) {
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillNtupleIColumn(5, fPhotonCount);
    // ※ベクターカラムは FillNtuple ではなく、作成時にポインタ紐づけをする(EventActionが管理する）
}