#include "ScintillatorSD.hh"

#include "AnalysisConfig.hh"
#include "NeutronTrackInfo.hh"

#include "G4AffineTransform.hh"
#include "G4Alpha.hh"
#include "G4EmSaturation.hh"
#include "G4Exception.hh"
#include "G4HCofThisEvent.hh"
#include "G4HadronicProcessType.hh"
#include "G4LossTableManager.hh"
#include "G4Neutron.hh"
#include "G4OpticalPhoton.hh"
#include "G4ProcessType.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4Triton.hh"
#include "G4VProcess.hh"
#include "G4VTouchable.hh"

#include <utility>


namespace {
    /*
     * depth=0 : シンチレータ内部volume
     * depth=1 : LiGlassまたはUROKO本体
     */
    constexpr G4int kDetectorDepth = 1;
}


// EventActionが後から取得するために必要
const G4String
ScintillatorSD::kHitsCollectionName =
    "ScintillatorHits";

// 光学光子のon/off を取得 → generatedPhotonsを数えるか否か
ScintillatorSD::ScintillatorSD(
    const G4String& name,
    std::shared_ptr<const AnalysisConfig> config
)
    : G4VSensitiveDetector(name),
      fAnalysisConfig(std::move(config))
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "ScintillatorSD::ScintillatorSD",
            "ScintillatorSD001",
            FatalException,
            "AnalysisConfig is null."
        );
    }

    collectionName.insert(
        kHitsCollectionName
    );
}


void ScintillatorSD::Initialize(
    G4HCofThisEvent* hitCollectionOfEvent
)
{
    fHitsByDetector.clear();

    // イベントごとにcollectionを生成
    fHitsCollection =
        new ScintillatorHitsCollection(
            SensitiveDetectorName,
            collectionName[0]
        );

    if (fHitsCollectionId < 0) {
        fHitsCollectionId =
            G4SDManager::GetSDMpointer()
                ->GetCollectionID(
                    fHitsCollection
                );
    }

    if (hitCollectionOfEvent == nullptr) {
        G4Exception(
            "ScintillatorSD::Initialize",
            "ScintillatorSD002",
            JustWarning,
            "G4HCofThisEvent is null."
        );

        return;
    }

    hitCollectionOfEvent->AddHitsCollection(
        fHitsCollectionId,
        fHitsCollection
    );
}


G4bool ScintillatorSD::ProcessHits(
    G4Step* step,
    G4TouchableHistory*
)
{
    if (step == nullptr ||
        fHitsCollection == nullptr) {
        return false;
    }

    const auto* track =
        step->GetTrack();

    const auto* prePoint =
        step->GetPreStepPoint();

    const auto* postPoint =
        step->GetPostStepPoint();

    if (track == nullptr ||
        prePoint == nullptr ||
        postPoint == nullptr) {
        return false;
    }

    const auto* touchable =
        prePoint->GetTouchable();

    if (touchable == nullptr) {
        return false;
    }

    if (touchable->GetHistoryDepth() <
        kDetectorDepth) {
        return false;
    }

    // 検出器全体が存在する層からCopyNoを取得する
    const G4int detectorCopyNo =
        touchable->GetCopyNumber(
            kDetectorDepth
        );

    if (detectorCopyNo < 0) {
        return false;
    }

    const DetectorKey detectorKey{
        detectorCopyNo
    };

    auto* hit =
        FindOrCreateHit(detectorKey);

    if (hit == nullptr) {
        return false;
    }


    /*
     * 中性子反応に関する処理。
     *
     * エネルギー付与が0でもhadronic反応が
     * 発生している可能性があるため、
     * edep判定より前に行う。
     */
    RecordNeutronInteraction(
        step,
        *hit
    );

    RecordNeutronCapture(
        step,
        *hit
    );


    /*
     * Offでは生成光子数を記録しない。
     */
    if (fAnalysisConfig != nullptr &&
        fAnalysisConfig
            ->IsOpticalRecordingEnabled()) {

        RecordGeneratedPhotons(
            step,
            *hit
        );
    }


    /*
     * エネルギー付与と可視エネルギーを記録する。
     */
    const G4double energyDeposit =
        step->GetTotalEnergyDeposit();

    if (energyDeposit != 0.0) {
        auto* saturation =
            G4LossTableManager::Instance()
                ->EmSaturation();

        G4double visibleEnergyDeposit = 0.0;

        if (saturation != nullptr) {
            visibleEnergyDeposit =
                saturation
                    ->VisibleEnergyDepositionAtAStep(
                        step
                    );
        }

        hit->AddEnergyDeposit(
            energyDeposit,
            visibleEnergyDeposit
        );
    }

    return true;
}


ScintillatorHit*
ScintillatorSD::FindOrCreateHit(
    const DetectorKey& detectorKey
)
{
    const auto found =
        fHitsByDetector.find(detectorKey);

    if (found != fHitsByDetector.end()) {
        return found->second;
    }

    if (fHitsCollection == nullptr) {
        return nullptr;
    }

    auto* hit =
        new ScintillatorHit(detectorKey);

    fHitsCollection->insert(hit);

    fHitsByDetector.emplace(
        detectorKey,
        hit
    );

    return hit;
}


void ScintillatorSD::RecordNeutronInteraction(
    const G4Step* step,
    ScintillatorHit& hit
)
{
    if (step == nullptr) {
        return;
    }

    const auto* track =
        step->GetTrack();

    const auto* prePoint =
        step->GetPreStepPoint();

    const auto* postPoint =
        step->GetPostStepPoint();

    if (track == nullptr ||
        prePoint == nullptr ||
        postPoint == nullptr) {
        return;
    }

    if (track->GetDefinition() !=
        G4Neutron::NeutronDefinition()) {
        return;
    }

    const auto* process =
        postPoint->GetProcessDefinedStep();

    const G4bool isHadronicInteraction =
        process != nullptr &&
        process->GetProcessType() ==
            fHadronic;

    if (!isHadronicInteraction) {
        return;
    }


    /*
     * parentID == 0の一次中性子だけを数える。
     */
    if (track->GetParentID() == 0) {
        hit
            .IncrementPrimaryNeutronInteractionCount();
    }


    // 2次中性子まで含めた記録をつけるために必要
    const auto* neutronInfo =
        dynamic_cast<const NeutronTrackInfo*>(
            track->GetUserInformation()
        );

    /*
     * NeutronTrackInfoを持たない中性子は、
     * 中性子系譜カウントと履歴保存の対象外。
     */
    if (neutronInfo == nullptr) {
        return;
    }

    hit
        .IncrementNeutronLineageInteractionCount();


    /*
     * 代表中性子がすでに確定していれば、
     * 新しい履歴で上書きしない。
     */
    if (hit.HasIncidentNeutronData()) {
        return;
    }


    /*
     * 選択された対象検出器へ入射済みであることを
     * 確認する。
     */
    if (!neutronInfo->HasEnteredTarget()) {
        return;
    }

    /*
     * TrackInfoに記録された入射先と、
     * 現在反応している検出器が同じか確認する。
     *
     * 対象でないシンチレータの反応を
     * 誤って代表履歴に採用することを防ぐ。
     */
    if (neutronInfo->GetEnteredDetectorCopyNo() !=
        hit.GetDetectorKey().detectorCopyNo) {
        return;
    }


    const auto* touchable =
        prePoint->GetTouchable();

    if (touchable == nullptr) {
        return;
    }

    const G4ThreeVector globalPosition =
        postPoint->GetPosition();

    const G4AffineTransform& transform =
        touchable->GetHistory()
            ->GetTopTransform();

    const G4ThreeVector localPosition =
        transform.TransformPoint(
            globalPosition
        );


    /*
     * 最初の追跡対象中性子反応の
     * 時刻と位置を保存する。
     */
    hit.SetFirstHit(
        postPoint->GetGlobalTime(),
        globalPosition,
        localPosition
    );


    IncidentNeutronData incidentData;

    incidentData.valid = true;

    incidentData.trackId =
        track->GetTrackID();

    incidentData.parentTrackId =
        track->GetParentID();

    incidentData.rootPrimaryTrackId =
        neutronInfo->GetRootPrimaryTrackId();

    incidentData.detectorCopyNo =
        neutronInfo
            ->GetEnteredDetectorCopyNo();

    incidentData.detectorEntryTime =
        neutronInfo
            ->GetDetectorEntryTime();

    incidentData.detectorEntryEnergy =
        neutronInfo
            ->GetDetectorEntryEnergy();

    incidentData.preDetectorScatterHistory =
        neutronInfo->GetScatterHistory();


    hit.SetIncidentNeutronData(
        incidentData
    );
}


void ScintillatorSD::RecordNeutronCapture(
    const G4Step* step,
    ScintillatorHit& hit
)
{
    if (step == nullptr) {
        return;
    }

    const auto* track =
        step->GetTrack();

    const auto* postPoint =
        step->GetPostStepPoint();

    if (track == nullptr ||
        postPoint == nullptr) {
        return;
    }

    if (track->GetDefinition() !=
        G4Neutron::NeutronDefinition()) {
        return;
    }

    const auto* process =
        postPoint->GetProcessDefinedStep();

    if (process == nullptr) {
        return;
    }

    const G4bool isNeutronInelastic =
        process->GetProcessSubType() ==
            fHadronInelastic ||
        process->GetProcessName() ==
            "neutronInelastic";

    if (!isNeutronInelastic) {
        return;
    }

    const auto* secondaries =
        step->GetSecondaryInCurrentStep();

    if (secondaries == nullptr) {
        return;
    }

    G4bool hasTriton = false;
    G4bool hasAlpha = false;

    for (const auto* secondary :
         *secondaries) {

        if (secondary == nullptr) {
            continue;
        }

        if (secondary->GetDefinition() ==
            G4Triton::TritonDefinition()) {

            hasTriton = true;
            continue;
        }

        if (secondary->GetDefinition() ==
            G4Alpha::AlphaDefinition()) {

            hasAlpha = true;
        }
    }

    if (hasTriton && hasAlpha) {
        hit.MarkNeutronCapture();
    }
}


void ScintillatorSD::RecordGeneratedPhotons(
    const G4Step* step,
    ScintillatorHit& hit
)
{
    if (step == nullptr) {
        return;
    }

    const auto* secondaries =
        step->GetSecondaryInCurrentStep();

    if (secondaries == nullptr) {
        return;
    }

    for (const auto* secondary :
         *secondaries) {

        if (secondary == nullptr) {
            continue;
        }

        if (secondary->GetDefinition() !=
            G4OpticalPhoton::
                OpticalPhotonDefinition()) {
            continue;
        }

        const auto* creatorProcess =
            secondary->GetCreatorProcess();

        if (creatorProcess == nullptr) {
            continue;
        }

        if (creatorProcess->GetProcessName() !=
            "Scintillation") {
            continue;
        }

        hit.IncrementGeneratedPhotons();
    }
}