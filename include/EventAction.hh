#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

#include <memory>


class AnalysisConfig;
class AnalysisOutput;
class G4Event;


class EventAction : public G4UserEventAction {
public:
    /*
     * 光学光子記録モードと中性子履歴対象を
     * ROOT出力へ渡すため、AnalysisConfigを保持する。
     */
    explicit EventAction(
        std::shared_ptr<const AnalysisConfig>
            analysisConfig
    );

    ~EventAction() override = default;


    void BeginOfEventAction(
        const G4Event* event
    ) override;

    void EndOfEventAction(
        const G4Event* event
    ) override;


    /*
     * AnalysisOutputの実体はRunActionが所有する。
     *
     * EventActionは所有権を持たず、
     * ROOT出力時に使用するポインタだけを保持する。
     */
    void SetAnalysisOutput(
        AnalysisOutput* analysisOutput
    )
    {
        fAnalysisOutput = analysisOutput;
    }


private:
    /*
     * 光学光子記録モードと、
     * 中性子履歴の対象検出器を取得する。
     */
    std::shared_ptr<const AnalysisConfig>
        fAnalysisConfig;


    /*
     * AnalysisOutputの非所有ポインタ。
     *
     * RunActionのコンストラクタから設定される。
     */
    AnalysisOutput* fAnalysisOutput = nullptr;


    /*
     * G4HCofThisEventからHit Collectionを取得するためのID。
     *
     * Collection IDの検索は比較的コストがあるため、
     * 最初のイベントで一度だけ取得してキャッシュする。
     *
     * EventActionはワーカースレッドごとに作られるので、
     * 各インスタンスがそれぞれIDを保持する。
     */
    G4int fScintillatorHitsCollectionId = -1;
    G4int fPhotocathodeHitsCollectionId = -1;
};

#endif