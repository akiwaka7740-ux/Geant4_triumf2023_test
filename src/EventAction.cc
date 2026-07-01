#include "EventAction.hh"
#include "G4AnalysisManager.hh"

void EventAction::BeginOfEventAction(const G4Event*) {
    for (int i = 0; i < 2; i++) {
        fHitTimeList[i].clear();
        fHitPosXList[i].clear();
        fHitPosYList[i].clear();
        fHitPosZList[i].clear();
    }
}

void EventAction::EndOfEventAction(const G4Event*) {
    //Event毎にデータを確定させる
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->AddNtupleRow();
}