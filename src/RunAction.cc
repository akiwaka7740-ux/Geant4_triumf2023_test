#include "RunAction.hh"

RunAction::RunAction(EventAction *eventAction) : fEventAction(eventAction)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetNtupleMerging(true);

    //旧型
    /*
    analysisManager->CreateNtuple("neutron", "neutron");
    analysisManager->CreateNtupleSColumn("fDetectorName");
    analysisManager->CreateNtupleDColumn("fEnergyDeposit");
    analysisManager->CreateNtupleDColumn("fIncidentEnergy");
    analysisManager->CreateNtupleDColumn("fHitTime");
    analysisManager->CreateNtupleDColumn("fHitTimeCapture");
    analysisManager->CreateNtupleDColumn("fPosX");
    analysisManager->CreateNtupleDColumn("fPosY");
    analysisManager->CreateNtupleDColumn("fPosZ");
    analysisManager->CreateNtupleIColumn("fHitCounter");
    analysisManager->FinishNtuple(0);
    */

    analysisManager->CreateNtuple("tree", "tree");

    // Scinti側のカラム (ID: 0 〜 4)
    analysisManager->CreateNtupleDColumn("Scinti_Edep");      // [0]
    analysisManager->CreateNtupleDColumn("Scinti_X");         // [1]
    analysisManager->CreateNtupleDColumn("Scinti_Y");         // [2]
    analysisManager->CreateNtupleDColumn("Scinti_Z");         // [3]
    analysisManager->CreateNtupleIColumn("Scinti_nHits");     // [4]

    // Cathode側のカラム (ID: 5 ~ 9)
    analysisManager->CreateNtupleIColumn("PMT_nPhotons");     // [5]
    analysisManager->CreateNtupleDColumn("PMT_HitTimes",fEventAction->GetHitTimeListRef());// [6]
    analysisManager->CreateNtupleDColumn("PMT_HitPos_X",fEventAction->GetHitPosXListRef());// [7]
    analysisManager->CreateNtupleDColumn("PMT_HitPos_Y",fEventAction->GetHitPosYListRef());// [8]
    analysisManager->CreateNtupleDColumn("PMT_HitPos_Z",fEventAction->GetHitPosZListRef());// [9]


}

RunAction::~RunAction()
{
}

void RunAction::BeginOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    G4int runID = run->GetRunID();

    std::stringstream strRunID;
    strRunID << runID;

    analysisManager->OpenFile("../root/output" + strRunID.str() + ".root");
}

void RunAction::EndOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    analysisManager->Write();

    analysisManager->CloseFile();

    G4int runID = run->GetRunID();

    G4cout << "Finishing run " << runID << G4endl;
}

