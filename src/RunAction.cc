#include "RunAction.hh"

RunAction::RunAction()
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetNtupleMerging(true);

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

