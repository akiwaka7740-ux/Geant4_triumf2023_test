#ifndef SCINTILLATOR_SD_HH
#define SCINTILLATOR_SD_HH

#include "ScintillatorHit.hh"

#include "G4VSensitiveDetector.hh"
#include "globals.hh"

#include <map>
#include <memory>


class AnalysisConfig;
class G4HCofThisEvent;
class G4Step;
class G4TouchableHistory;


class ScintillatorSD final
    : public G4VSensitiveDetector {
public:
    /*
     * EventActionがHit Collection IDを取得するときにも
     * 同じ名前を使用する。
     */
    static const G4String
        kHitsCollectionName;


    ScintillatorSD(
        const G4String& name,
        std::shared_ptr<const AnalysisConfig> config
    );

    ~ScintillatorSD() override = default;


    /*
     * イベント開始時に新しいHit Collectionを作成し、
     * G4HCofThisEventへ登録する。
     */
    void Initialize(
        G4HCofThisEvent* hitCollectionOfEvent
    ) override;


    /*
     * シンチレータ内でStepが発生するたびに呼ばれる。
     */
    G4bool ProcessHits(
        G4Step* step,
        G4TouchableHistory* history
    ) override;


private:
    /*
     * detectorCopyNoに対応するHitを取得する。
     *
     * まだ存在しない場合は新しいHitを作成して、
     * Hit Collectionへ登録する。
     */
    ScintillatorHit* FindOrCreateHit(
        const DetectorKey& detectorKey
    );


    /*
     * 中性子hadronic反応を処理する。
     *
     * 一次中性子反応数、系譜全体の反応数、
     * 最初の反応位置、IncidentNeutronDataを更新する。
     */
    static void RecordNeutronInteraction(
        const G4Step* step,
        ScintillatorHit& hit
    );


    /*
     * LiGlassの中性子捕獲反応に相当する、
     * tritonとalphaの生成を確認する。
     */
    static void RecordNeutronCapture(
        const G4Step* step,
        ScintillatorHit& hit
    );


    /*
     * Scintillationプロセスで生成された
     * 光学光子数を記録する。
     *
     * OpticalRecordingMode::Offでは呼び出さない。
     */
    static void RecordGeneratedPhotons(
        const G4Step* step,
        ScintillatorHit& hit
    );


    std::shared_ptr<const AnalysisConfig>
        fAnalysisConfig;


    /*
     * fHitsCollectionの所有権は、
     * 登録後のG4HCofThisEventが管理する。
     */
    ScintillatorHitsCollection*
        fHitsCollection = nullptr;

    G4int fHitsCollectionId = -1;


    /*
     * StepごとにHit Collection全体を線形探索しないための
     * 一時的な検索用map。
     *
     * Hit自体の所有権は持たない。
     * イベント開始時にclearする。
     */
    std::map<DetectorKey, ScintillatorHit*>
        fHitsByDetector;
};

#endif