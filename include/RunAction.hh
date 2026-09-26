#ifndef RUNACTION_HH
#define RUNACTION_HH

#include <memory>
#include <optional>

#include "AnalysisOutput.hh"
#include "RunConditions.hh"

#include "G4UserRunAction.hh"
#include "globals.hh"

class EventAction;
class G4Run;
struct OutputConfig;

class RunAction final : public G4UserRunAction
{
public:
    // master では eventAction に nullptr を渡す
    RunAction(
        EventAction* eventAction,
        std::shared_ptr<const OutputConfig> outputConfig
    );

    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

private:
    AnalysisOutput fAnalysisOutput;

    // 実行全体で固定する出力設定
    std::shared_ptr<const OutputConfig> fOutputConfig;

    // 現在の Run で採用した条件
    std::optional<RunConditions> fRunConditions;

    // 現在の Run の ROOT 出力パス
    G4String fOutputRootFileName;
};

#endif