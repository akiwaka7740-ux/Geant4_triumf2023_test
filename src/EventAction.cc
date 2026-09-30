#include "EventAction.hh"

#include "RunAction.hh"
#include "AnalysisConfig.hh"
#include "AnalysisOutput.hh"
#include "DetectorChannelKey.hh"
#include "PhotocathodeHit.hh"
#include "ScintillatorHit.hh"

#include "G4Event.hh"
#include "G4Exception.hh"
#include "G4HCofThisEvent.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"

#include <map>
#include <utility>


namespace {

/*
 * DetectorConstructionで設定しているSD名と、
 * 各SDが登録しているHit Collection名を結合した名前。
 *
 * 形式:
 *   SensitiveDetectorName/CollectionName
 */
const G4String
    kScintillatorHitsCollectionFullName =
        "ScintillatorSD/ScintillatorHits";

const G4String
    kPhotocathodeHitsCollectionFullName =
        "PhotocathodeSD/PhotocathodeHits";


struct PmtTotals {
    G4int sumArrivedPhotons = 0;
    G4int sumDetectedPhotons = 0;
};

}


EventAction::EventAction(
    std::shared_ptr<const AnalysisConfig>
        analysisConfig
)
    : fAnalysisConfig(
          std::move(analysisConfig)
      )
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "EventAction::EventAction",
            "EventAction001",
            FatalException,
            "AnalysisConfig is null."
        );
    }
}


void EventAction::BeginOfEventAction(
    const G4Event*
)
{
    /*
     * Collection IDは最初のイベントで一度だけ検索し、
     * 以降のイベントでは保存済みのIDを使用する。
     */
    if (
        fScintillatorHitsCollectionId >= 0 &&
        fPhotocathodeHitsCollectionId >= 0
    ) {
        return;
    }


    auto* sdManager =
        G4SDManager::GetSDMpointer();

    if (sdManager == nullptr) {
        G4Exception(
            "EventAction::BeginOfEventAction",
            "EventAction002",
            FatalException,
            "G4SDManager is null."
        );

        return;
    }


    if (fScintillatorHitsCollectionId < 0) {
        fScintillatorHitsCollectionId =
            sdManager->GetCollectionID(
                kScintillatorHitsCollectionFullName
            );
    }

    if (fPhotocathodeHitsCollectionId < 0) {
        fPhotocathodeHitsCollectionId =
            sdManager->GetCollectionID(
                kPhotocathodeHitsCollectionFullName
            );
    }


    if (
        fScintillatorHitsCollectionId < 0 ||
        fPhotocathodeHitsCollectionId < 0
    ) {
        G4Exception(
            "EventAction::BeginOfEventAction",
            "EventAction003",
            FatalException,
            "Required Hit Collection ID "
            "could not be found."
        );
    }
}


void EventAction::EndOfEventAction(
    const G4Event* event
)
{
    if (
        event == nullptr ||
        fAnalysisOutput == nullptr
    ) {
        G4Exception(
            "EventAction::EndOfEventAction",
            "EventAction011",
            FatalException,
            "Event or AnalysisOutput is null."
        );
        return;
    }


    /*
     * 通常はBeginOfEventActionで取得済みだが、
     * 念のため未取得の場合には再取得を試みる。
     */
    if (
        fScintillatorHitsCollectionId < 0 ||
        fPhotocathodeHitsCollectionId < 0
    ) {
        BeginOfEventAction(event);
    }


    auto* hitCollections =
        event->GetHCofThisEvent();

    if (hitCollections == nullptr) {
        G4Exception(
            "EventAction::EndOfEventAction",
            "EventAction004",
            FatalException,
            "G4HCofThisEvent is null."
        );

        return;
    }


    auto* scintillatorHits =
        dynamic_cast<ScintillatorHitsCollection*>(
            hitCollections->GetHC(
                fScintillatorHitsCollectionId
            )
        );

    auto* photocathodeHits =
        dynamic_cast<PhotocathodeHitsCollection*>(
            hitCollections->GetHC(
                fPhotocathodeHitsCollectionId
            )
        );


    if (scintillatorHits == nullptr) {
        G4Exception(
            "EventAction::EndOfEventAction",
            "EventAction005",
            FatalException,
            "ScintillatorHitsCollection was not found."
        );

        return;
    }

    if (photocathodeHits == nullptr) {
        G4Exception(
            "EventAction::EndOfEventAction",
            "EventAction006",
            FatalException,
            "PhotocathodeHitsCollection was not found."
        );

        return;
    }

    // EventSelectionModeに応じて出力を変更
    if (fRunAction == nullptr) {
        G4Exception(
            "EventAction::EndOfEventAction",
            "EventAction009",
            FatalException,
            "RunAction is null."
        );
        return;
    }

    const auto selectionMode =
        fAnalysisConfig->GetEventSelectionMode();

    G4bool selected = false;

    switch (selectionMode) {
    case EventSelectionMode::All:
        selected = true;
        break;

    case EventSelectionMode::ScintiEdep:
    case EventSelectionMode::NeutronCapture:
        for (std::size_t index = 0;
            index < scintillatorHits->entries();
            ++index) {
            const auto* hit = (*scintillatorHits)[index];

            if (hit == nullptr) {
                continue;
            }

            if (selectionMode == EventSelectionMode::ScintiEdep &&
                hit->GetTotalEdep() > 0.0) {
                selected = true;
                break;
            }

            if (selectionMode == EventSelectionMode::NeutronCapture &&
                hit->HasNeutronCapture()) {
                selected = true;
                break;
            }
        }
        break;

    default:
        G4Exception(
            "EventAction::EndOfEventAction",
            "EventAction010",
            FatalException,
            "Unknown event selection mode."
        );
        return;
    }

    if (!selected) {
        return;
    }

    if (photocathodeHits->entries() == 0) {
        G4Exception(
            "EventAction::EndOfEventAction",
            "EventAction012",
            FatalException,
            "Selected event has no PMT channel hits."
        );
        return;
    }

    fRunAction->CountSelectedEvent();


    /*
     * DetectorKeyからScintillatorHitを検索するための
     * 非所有ポインタmapを作成する。
     *
     * Hitの所有権はG4HCofThisEventが持っている。
     */
    std::map<
        DetectorKey,
        const ScintillatorHit*
    > scintillatorHitsByDetector;


    for (
        std::size_t index = 0;
        index < scintillatorHits->entries();
        ++index
    ) {
        const auto* hit =
            (*scintillatorHits)[index];

        if (hit == nullptr) {
            continue;
        }

        const auto result =
            scintillatorHitsByDetector.emplace(
                hit->GetDetectorKey(),
                hit
            );

        if (!result.second) {
            G4Exception(
                "EventAction::EndOfEventAction",
                "EventAction007",
                JustWarning,
                "Multiple ScintillatorHit objects "
                "have the same DetectorKey. "
                "The first Hit will be used."
            );
        }
    }


    /*
     * 検出器単位で、全PMTの到達数と検出数を合計する。
     */
    std::map<DetectorKey, PmtTotals>
        totalsByDetector;


    for (
        std::size_t index = 0;
        index < photocathodeHits->entries();
        ++index
    ) {
        const auto* hit =
            (*photocathodeHits)[index];

        if (hit == nullptr) {
            continue;
        }

        const auto& channelKey =
            hit->GetChannelKey();

        auto& totals =
            totalsByDetector[
                channelKey.detector
            ];

        totals.sumArrivedPhotons +=
            hit->GetArrivedPhotons();

        totals.sumDetectedPhotons +=
            hit->GetDetectedPhotons();
    }


    const G4int eventId =
        event->GetEventID();

    G4int runId = -1;

    const auto* runManager =
        G4RunManager::GetRunManager();

    if (runManager != nullptr) {
        const auto* currentRun =
            runManager->GetCurrentRun();

        if (currentRun != nullptr) {
            runId =
                currentRun->GetRunID();
        }
    }


    /*
     * 1 PMTチャンネルにつきROOTへ1行出力する。
     */
    for (
        std::size_t index = 0;
        index < photocathodeHits->entries();
        ++index
    ) {
        const auto* photocathodeHit =
            (*photocathodeHits)[index];

        if (photocathodeHit == nullptr) {
            G4Exception(
                "EventAction::EndOfEventAction",
                "EventAction013",
                FatalException,
                "PhotocathodeHitsCollection contains a null hit."
            );
            return;
        }


        const auto& channelKey =
            photocathodeHit->GetChannelKey();


        /*
         * 対応するScintillatorHitがない場合は
         * nullptrのままAnalysisOutputへ渡す。
         */
        const ScintillatorHit*
            scintillatorHit = nullptr;

        const auto scintillatorIterator =
            scintillatorHitsByDetector.find(
                channelKey.detector
            );

        if (
            scintillatorIterator !=
            scintillatorHitsByDetector.end()
        ) {
            scintillatorHit =
                scintillatorIterator->second;
        }


        const auto totalsIterator =
            totalsByDetector.find(
                channelKey.detector
            );

        if (
            totalsIterator ==
            totalsByDetector.end()
        ) {
            G4Exception(
                "EventAction::EndOfEventAction",
                "EventAction008",
                FatalException,
                "PMT totals were not found for "
                "the current detector."
            );

            return;
        }

        const auto& totals =
            totalsIterator->second;


        G4double arrivalEfficiency = 0.0;
        G4double detectionEfficiency = 0.0;

        G4double sumArrivalEfficiency = 0.0;
        G4double sumDetectionEfficiency = 0.0;


        /*
         * 生成光子数が0の場合は除算せず、
         * 効率を0.0として出力する。
         */
        if (scintillatorHit != nullptr) {
            const G4int generatedPhotons =
                scintillatorHit
                    ->GetGeneratedPhotons();

            if (generatedPhotons > 0) {
                arrivalEfficiency =
                    static_cast<G4double>(
                        photocathodeHit
                            ->GetArrivedPhotons()
                    )
                    / generatedPhotons;

                detectionEfficiency =
                    static_cast<G4double>(
                        photocathodeHit
                            ->GetDetectedPhotons()
                    )
                    / generatedPhotons;

                sumArrivalEfficiency =
                    static_cast<G4double>(
                        totals.sumArrivedPhotons
                    )
                    / generatedPhotons;

                sumDetectionEfficiency =
                    static_cast<G4double>(
                        totals.sumDetectedPhotons
                    )
                    / generatedPhotons;
            }
        }


        fAnalysisOutput->FillChannelRow(
            runId,
            eventId,
            channelKey,
            scintillatorHit,
            *photocathodeHit,
            fAnalysisConfig
                ->GetOpticalRecordingMode(),
            fAnalysisConfig
                ->GetNeutronHistoryTarget(),
            totals.sumArrivedPhotons,
            totals.sumDetectedPhotons,
            arrivalEfficiency,
            detectionEfficiency,
            sumArrivalEfficiency,
            sumDetectionEfficiency
        );
    }
}