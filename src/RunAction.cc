#include <cstdlib>

#include "RunAction.hh"
#include "EventAction.hh"


namespace {
G4String GetOutputRootFileName(const G4Run* run)
{
    //環境変数が定義されている場合はそれを使用する
    const char* envOutput = std::getenv("G4_OUTPUT_ROOT");
    if (envOutput && envOutput[0] != '\0') {
        return G4String(envOutput);
    }

    std::stringstream strRunID;
    strRunID << run->GetRunID();
    return "../root/output" + strRunID.str() + ".root";
}

}


RunAction::RunAction(EventAction *eventAction, RunConfig* runConfig) : fEventAction(eventAction), fRunConfig(runConfig)
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
    analysisManager->OpenFile(GetOutputRootFileName(run));

}

void RunAction::EndOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    analysisManager->Write();

    analysisManager->CloseFile();

    G4int runID = run->GetRunID();

    G4cout << "Finishing run " << runID << G4endl;
}

