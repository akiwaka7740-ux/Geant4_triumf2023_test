#include "PhotocathodeSD.hh"

#include "AnalysisConfig.hh"
#include "OpticalPhotonTrackInfo.hh"

#include "G4Exception.hh"
#include "G4HCofThisEvent.hh"
#include "G4OpticalPhoton.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4VTouchable.hh"

#include <utility>


namespace {
    /*
     * post-step touchableの階層。
     *
     * depth=0 : Cathode
     * depth=1 : PMT
     * depth=2 : LiGlassまたはUROKO本体
     */
    constexpr G4int kPmtDepth = 1;
    constexpr G4int kDetectorDepth = 2;
}


const G4String
PhotocathodeSD::kHitsCollectionName =
    "PhotocathodeHits";


PhotocathodeSD::PhotocathodeSD(
    const G4String& name,
    std::shared_ptr<const AnalysisConfig> config
)
    : G4VSensitiveDetector(name),
      fAnalysisConfig(std::move(config))
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "PhotocathodeSD::PhotocathodeSD",
            "PhotocathodeSD001",
            FatalException,
            "AnalysisConfig is null."
        );
    }

    collectionName.insert(
        kHitsCollectionName
    );
}


void PhotocathodeSD::Initialize(
    G4HCofThisEvent* hitCollectionOfEvent
)
{
    fHitsByChannel.clear();
    fHitsCollection = nullptr;

    if (hitCollectionOfEvent == nullptr) {
        G4Exception(
            "PhotocathodeSD::Initialize",
            "PhotocathodeSD002",
            JustWarning,
            "G4HCofThisEvent is null."
        );

        return;
    }

    fHitsCollection =
        new PhotocathodeHitsCollection(
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

    hitCollectionOfEvent->AddHitsCollection(
        fHitsCollectionId,
        fHitsCollection
    );


    /*
     * 光子が一つも到達しなかったPMTについても
     * 出力行を作れるように、登録済みチャンネルの
     * Hitをイベント開始時に作成する。
     */
    for (const auto& channelKey :
         fRegisteredChannels) {

        auto* hit =
            new PhotocathodeHit(channelKey);

        fHitsCollection->insert(hit);

        fHitsByChannel.emplace(
            channelKey,
            hit
        );
    }
}


G4bool PhotocathodeSD::ProcessHits(
    G4Step*,
    G4TouchableHistory*
)
{
    /*
     * 光電面での記録にはOpBoundaryの状態が必要。
     * 通常のSensitive Detector呼び出しでは記録せず、
     * ProcessBoundaryInteraction()を使用する。
     */
    return false;
}


G4bool
PhotocathodeSD::ProcessBoundaryInteraction(
    const G4Step* step,
    G4bool detected
)
{
    if (step == nullptr ||
        fHitsCollection == nullptr ||
        fAnalysisConfig == nullptr) {
        return false;
    }

    /*
     * Offでは光学解析情報を記録しない。
     */
    if (!fAnalysisConfig
            ->IsOpticalRecordingEnabled()) {
        return false;
    }

    const auto* track =
        step->GetTrack();

    if (track == nullptr) {
        return false;
    }

    if (track->GetDefinition() !=
        G4OpticalPhoton::
            OpticalPhotonDefinition()) {
        return false;
    }


    /*
     * post-step touchableからPMTチャンネルを特定する。
     */
    PmtChannelKey channelKey{
        DetectorKey{-1},
        -1
    };

    if (!FindChannelKey(
            step,
            channelKey)) {
        return false;
    }

    auto* hit =
        FindHit(channelKey);

    if (hit == nullptr) {
        return false;
    }


    /*
     * この関数は光電面へ到達した光子について
     * 1回だけ呼ばれる前提なので、まず到達数を加算する。
     */
    hit->IncrementArrivedPhotons();


    if (!detected) {
        return true;
    }


    hit->IncrementDetectedPhotons();


    /*
     * Detailedの場合だけ光子ごとの情報を保存する。
     */
    if (fAnalysisConfig
            ->IsOpticalDetailEnabled()) {

        AppendDetailedPhotonData(
            step,
            *hit
        );
    }

    return true;
}


void PhotocathodeSD::RegisterChannel(
    const PmtChannelKey& channelKey
)
{
    if (channelKey.detector.detectorCopyNo < 0 ||
        channelKey.pmtCopyNo < 0) {
        return;
    }

    fRegisteredChannels.insert(
        channelKey
    );
}


G4bool PhotocathodeSD::FindChannelKey(
    const G4Step* step,
    PmtChannelKey& channelKey
)
{
    if (step == nullptr) {
        return false;
    }

    const auto* postPoint =
        step->GetPostStepPoint();

    if (postPoint == nullptr) {
        return false;
    }

    const auto* touchable =
        postPoint->GetTouchable();

    if (touchable == nullptr) {
        return false;
    }

    if (touchable->GetHistoryDepth() <
        kDetectorDepth) {
        return false;
    }

    const G4int pmtCopyNo =
        touchable->GetCopyNumber(
            kPmtDepth
        );

    const G4int detectorCopyNo =
        touchable->GetCopyNumber(
            kDetectorDepth
        );

    if (pmtCopyNo < 0 ||
        detectorCopyNo < 0) {
        return false;
    }

    channelKey = PmtChannelKey{
        DetectorKey{detectorCopyNo},
        pmtCopyNo
    };

    return true;
}


PhotocathodeHit*
PhotocathodeSD::FindHit(
    const PmtChannelKey& channelKey
)
{
    const auto found =
        fHitsByChannel.find(channelKey);

    if (found == fHitsByChannel.end()) {
        return nullptr;
    }

    return found->second;
}


void PhotocathodeSD::AppendDetailedPhotonData(
    const G4Step* step,
    PhotocathodeHit& hit
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


    /*
     * DetailedモードではTrackingActionが
     * OpticalPhotonTrackInfoを生成している。
     *
     * 取得できない場合は異常値-1を保存し、
     * vector同士の長さを一致させる。
     */
    G4int scintillatorBoundaryCount = -1;
    G4int lightGuideBoundaryCount = -1;

    G4int scintillatorReflectionCount = -1;
    G4int lightGuideReflectionCount = -1;

    const auto* trackInfo =
        dynamic_cast<
            const OpticalPhotonTrackInfo*
        >(
            track->GetUserInformation()
        );

    if (trackInfo != nullptr) {
        scintillatorBoundaryCount =
            trackInfo
                ->GetScintillatorBoundaryCount();

        lightGuideBoundaryCount =
            trackInfo
                ->GetLightGuideBoundaryCount();

        scintillatorReflectionCount =
            trackInfo
                ->GetScintillatorReflectionCount();

        lightGuideReflectionCount =
            trackInfo
                ->GetLightGuideReflectionCount();
    }


    /*
     * 単位変換はAnalysisOutputで行うため、
     * HitにはGeant4内部単位のまま保存する。
     */
    hit.AppendDetectedPhotonDetail(
        postPoint->GetGlobalTime(),
        track->GetLocalTime(),
        track->GetTrackLength(),
        postPoint->GetPosition(),
        scintillatorBoundaryCount,
        lightGuideBoundaryCount,
        scintillatorReflectionCount,
        lightGuideReflectionCount
    );
}