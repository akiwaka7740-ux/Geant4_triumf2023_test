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

    // ========================================================
    // Scinti側のカラム (ID: 0 〜 5)
    // ========================================================
    analysisManager->CreateNtupleDColumn("Scinti_Edep");      // [0]
    analysisManager->CreateNtupleDColumn("Scinti_Photons");   // [1]
    analysisManager->CreateNtupleDColumn("Scinti_X");         // [2]
    analysisManager->CreateNtupleDColumn("Scinti_Y");         // [3]
    analysisManager->CreateNtupleDColumn("Scinti_Z");         // [4]
    analysisManager->CreateNtupleIColumn("Scinti_nHits");     // [5]

    // ========================================================
    // PMT1 のカラム (ID: 6 ~ 10)　7だけPMT2
    // ========================================================
    analysisManager->CreateNtupleIColumn("PMT1_Photons");      // [6]
    analysisManager->CreateNtupleIColumn("PMT2_Photons");      // [7] 

    analysisManager->CreateNtupleDColumn("PMT1_HitTimes", fEventAction->GetHitTimeListRef(0)); // [8]
    analysisManager->CreateNtupleDColumn("PMT1_HitPos_X", fEventAction->GetHitPosXListRef(0)); // [9]
    analysisManager->CreateNtupleDColumn("PMT1_HitPos_Y", fEventAction->GetHitPosYListRef(0)); // [10]
    analysisManager->CreateNtupleDColumn("PMT1_HitPos_Z", fEventAction->GetHitPosZListRef(0)); // [11]

    // ========================================================
    // PMT2 のカラム (ID: 11 ~ 14)
    // ========================================================
    analysisManager->CreateNtupleDColumn("PMT2_HitTimes", fEventAction->GetHitTimeListRef(1)); // [12]
    analysisManager->CreateNtupleDColumn("PMT2_HitPos_X", fEventAction->GetHitPosXListRef(1)); // [13]
    analysisManager->CreateNtupleDColumn("PMT2_HitPos_Y", fEventAction->GetHitPosYListRef(1)); // [14]
    analysisManager->CreateNtupleDColumn("PMT2_HitPos_Z", fEventAction->GetHitPosZListRef(1)); // [15]


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

