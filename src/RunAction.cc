#include "RunAction.hh"
#include "EventAction.hh"

RunAction::RunAction(EventAction *eventAction) : fEventAction(eventAction)
{

    fAnalysisOutput.Book(fEventAction);

    //EventActionを介して、CathodeSDやScintiSDはAnalysisOutputを参照できるようにする
    fEventAction->SetAnalysisOutput(&fAnalysisOutput);
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

