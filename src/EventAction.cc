#include "EventAction.hh"
#include "G4AnalysisManager.hh"

void EventAction::BeginOfEventAction(const G4Event*) {
    fHitTimeList.clear();
    fHitPosXList.clear();
    fHitPosYList.clear();
    fHitPosZList.clear();
}

void EventAction::EndOfEventAction(const G4Event*) {
    //Event毎にデータを確定させる
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->AddNtupleRow();
}