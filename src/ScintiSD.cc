#include "ScintiSD.hh"
#include "G4OpticalPhoton.hh"


ScintiSD::ScintiSD(G4String name)
 : G4VSensitiveDetector(name),
   fTotalEdep(0.0),
   fGenerateedPhotons(0.0),
   fFirstHitPos(0, 0, 0),
   fHitCount(0)
{
}



void ScintiSD::Initialize(G4HCofThisEvent*) {
    fTotalEdep = 0.0;
    fGenerateedPhotons = 0.0;
    fFirstHitPos = G4ThreeVector(0,0,0);
    fHitCount = 0;
}

G4bool ScintiSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
    // 1. エネルギー付与を加算
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep == 0.) return false;

    fTotalEdep += edep;

    // 2. 光子発生数を加算
    auto secondaries = aStep->GetSecondaryInCurrentStep(); //実際にはG4TrackVector型のポインタが返ってくる
    if (secondaries) {
        for (auto track : *secondaries) { //配列であるsecondariesをfor文で回すときの書き方。autoで型を自動推論させる
            if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()){
                fGenerateedPhotons++;
            } 
        }
    }

    // 3.「最初の1発目」の座標だけをキープする
    if (fHitCount == 0) {
        fFirstHitPos = aStep->GetPostStepPoint()->GetPosition();
    }
    fHitCount++;
    return true;
}

void ScintiSD::EndOfEvent(G4HCofThisEvent*) {
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillNtupleDColumn(0, fTotalEdep);
    analysisManager->FillNtupleDColumn(1, fGenerateedPhotons);
    analysisManager->FillNtupleDColumn(2, fFirstHitPos.x());
    analysisManager->FillNtupleDColumn(3, fFirstHitPos.y());
    analysisManager->FillNtupleDColumn(4, fFirstHitPos.z());
    analysisManager->FillNtupleIColumn(5, fHitCount);
}