#include "RunAction.hh"

#include "EventAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "globals.hh"

#include <cstdlib>
#include <sstream>


namespace {

G4String GetOutputRootFileName(
    const G4Run* run
)
{
    /*
     * 環境変数が定義されている場合は、
     * そのファイル名を優先する。
     */
    const char* environmentOutput =
        std::getenv("G4_OUTPUT_ROOT");

    if (
        environmentOutput != nullptr &&
        environmentOutput[0] != '\0'
    ) {
        return G4String(environmentOutput);
    }


    std::stringstream runIdStream;

    runIdStream << run->GetRunID();

    return
        "../root/output"
        + runIdStream.str()
        + ".root";
}

}


RunAction::RunAction(
    EventAction* eventAction
)
{
    /*
     * masterと各workerのAnalysisManagerに
     * 同一構成のntupleを定義する。
     */
    fAnalysisOutput.Book();


    /*
     * workerの場合だけ、EventActionへ
     * AnalysisOutputの非所有ポインタを渡す。
     *
     * masterにはEventActionが存在しないため、
     * eventActionはnullptrとなる。
     */
    if (eventAction != nullptr) {
        eventAction->SetAnalysisOutput(
            &fAnalysisOutput
        );
    }
}


void RunAction::BeginOfRunAction(
    const G4Run* run
)
{
    if (run == nullptr) {
        return;
    }


    auto* analysisManager =
        G4AnalysisManager::Instance();

    analysisManager->OpenFile(
        GetOutputRootFileName(run)
    );
}


void RunAction::EndOfRunAction(
    const G4Run* run
)
{
    auto* analysisManager =
        G4AnalysisManager::Instance();

    analysisManager->Write();
    analysisManager->CloseFile();


    if (run != nullptr) {
        G4cout
            << "Finishing run "
            << run->GetRunID()
            << G4endl;
    }
}