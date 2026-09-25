#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "AnalysisOutput.hh"

#include "G4UserRunAction.hh"


class EventAction;
class G4Run;


class RunAction final : public G4UserRunAction {
public:
    /*
     * workerではEventActionを渡す。
     * masterではEventActionが存在しないためnullptrを渡す。
     */
    explicit RunAction(
        EventAction* eventAction
    );

    ~RunAction() override = default;


    void BeginOfRunAction(
        const G4Run* run
    ) override;

    void EndOfRunAction(
        const G4Run* run
    ) override;


private:
    /*
     * AnalysisOutputの実体はRunActionが所有する。
     *
     * workerでは、この実体への非所有ポインタを
     * EventActionへ渡す。
     *
     * masterではROOT ntupleの定義と
     * マージ処理のために使用する。
     */
    AnalysisOutput fAnalysisOutput;
};

#endif