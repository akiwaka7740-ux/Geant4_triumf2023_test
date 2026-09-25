#ifndef PHOTOCATHODE_SD_HH
#define PHOTOCATHODE_SD_HH

#include "PhotocathodeHit.hh"

#include "G4VSensitiveDetector.hh"
#include "globals.hh"

#include <map>
#include <memory>
#include <set>


class AnalysisConfig;
class G4HCofThisEvent;
class G4Step;
class G4TouchableHistory;


class PhotocathodeSD final
    : public G4VSensitiveDetector {
public:
    static const G4String
        kHitsCollectionName;


    PhotocathodeSD(
        const G4String& name,
        std::shared_ptr<const AnalysisConfig> config
    );

    ~PhotocathodeSD() override = default;


    /*
     * イベント開始時にHit Collectionを作成し、
     * 登録済みの全PMTチャンネルについて
     * PhotocathodeHitを用意する。
     */
    void Initialize(
        G4HCofThisEvent* hitCollectionOfEvent
    ) override;


    /*
     * 通常のvolume内Stepでは記録しない。
     *
     * 光電面での判定はOpBoundaryの状態が必要なため、
     * ProcessBoundaryInteraction()から記録する。
     */
    G4bool ProcessHits(
        G4Step* step,
        G4TouchableHistory* history
    ) override;


    /*
     * 光学光子が光電面境界へ到達したときに、
     * OpticalPhotonStepProcessorから呼ぶ。
     *
     * detected=false:
     *   到達数だけを加算する。
     *
     * detected=true:
     *   到達数と検出数を加算し、
     *   Detailedモードでは光子ごとの情報も保存する。
     */
    G4bool ProcessBoundaryInteraction(
        const G4Step* step,
        G4bool detected
    );


    /*
     * DetectorConstructionで、
     * 実在するPMTチャンネルを登録する。
     */
    void RegisterChannel(
        const PmtChannelKey& channelKey
    );


private:
    /*
     * post-step touchableから、
     * 検出器copyNoとPMT copyNoを取得する。
     */
    static G4bool FindChannelKey(
        const G4Step* step,
        PmtChannelKey& channelKey
    );


    /*
     * 登録済みチャンネルに対応するHitを取得する。
     * 未登録チャンネルについて新しいHitは作らない。
     */
    PhotocathodeHit* FindHit(
        const PmtChannelKey& channelKey
    );


    /*
     * Detailedモードでのみ、
     * 光子ごとの時刻・距離・位置・反射回数を記録する。
     */
    static void AppendDetailedPhotonData(
        const G4Step* step,
        PhotocathodeHit& hit
    );


    std::shared_ptr<const AnalysisConfig>
        fAnalysisConfig;


    /*
     * DetectorConstructionで登録された、
     * 実在するPMTチャンネル。
     */
    std::set<PmtChannelKey>
        fRegisteredChannels;


    /*
     * イベントごとに作成されるHit Collection。
     * 所有権はG4HCofThisEventが管理する。
     */
    PhotocathodeHitsCollection*
        fHitsCollection = nullptr;

    G4int fHitsCollectionId = -1;


    /*
     * PMTチャンネルからHitを高速に取得するための
     * 非所有ポインタmap。
     */
    std::map<PmtChannelKey, PhotocathodeHit*>
        fHitsByChannel;
};

#endif