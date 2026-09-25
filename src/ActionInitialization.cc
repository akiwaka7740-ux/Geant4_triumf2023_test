#include "ActionInitialization.hh"

#include "AnalysisConfig.hh"
#include "EventAction.hh"
#include "PrimaryGenerator.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"
#include "TrackingAction.hh"

#include "G4Exception.hh"

#include <utility>


ActionInitialization::ActionInitialization(
    std::shared_ptr<const AnalysisConfig>
        analysisConfig
)
    : fRunConfig(new RunConfig()),
      fAnalysisConfig(
          std::move(analysisConfig)
      )
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "ActionInitialization::"
            "ActionInitialization",
            "ActionInitialization001",
            FatalException,
            "AnalysisConfig is null."
        );
    }
}


ActionInitialization::~ActionInitialization()
{
    delete fRunConfig;
}


void ActionInitialization::BuildForMaster() const
{
    /*
     * masterスレッドではイベントを処理しないため、
     * EventAction、TrackingAction、
     * SteppingActionは生成しない。
     *
     * RunActionはROOT ntupleの定義と、
     * worker出力のマージに必要。
     */
    auto* runAction =
        new RunAction(nullptr);

    SetUserAction(runAction);
}


void ActionInitialization::Build() const
{
    /*
     * workerスレッド用EventAction。
     *
     * 光学光子記録モードと中性子履歴対象を
     * ROOT出力へ渡すため、AnalysisConfigを共有する。
     */
    auto* eventAction =
        new EventAction(fAnalysisConfig);

    SetUserAction(eventAction);


    /*
     * 一次粒子生成。
     */
    auto* generator =
        new PrimaryGenerator(fRunConfig);

    SetUserAction(generator);


    /*
     * worker用AnalysisOutputを所有し、
     * EventActionへ接続する。
     */
    auto* runAction =
        new RunAction(eventAction);

    SetUserAction(runAction);


    /*
     * Track単位の情報管理と、
     * 二次中性子への履歴継承を担当する。
     */
    auto* trackingAction =
        new TrackingAction(
            fAnalysisConfig
        );

    SetUserAction(trackingAction);


    /*
     * Step単位で中性子処理と
     * 光学光子処理を振り分ける。
     */
    auto* steppingAction =
        new SteppingAction(
            fAnalysisConfig
        );

    SetUserAction(steppingAction);
}