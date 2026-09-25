#ifndef ANALYSISOUTPUT_HH
#define ANALYSISOUTPUT_HH

#include "AnalysisConfig.hh"
#include "globals.hh"

#include <vector>


struct PmtChannelKey;

class ScintillatorHit;
class PhotocathodeHit;


class AnalysisOutput {
public:
    AnalysisOutput() = default;
    ~AnalysisOutput() = default;


    /*
     * ROOT ntupleの列を作成する。
     */
    void Book();


    /*
     * 1つのPMTチャンネルに対応するROOTの1行を記録する。
     *
     * scintillatorHit:
     *   対応するScintillatorHitが存在しない場合はnullptr。
     *
     * photocathodeHit:
     *   このPMTチャンネルに対応するPhotocathodeHit。
     */
    void FillChannelRow(
        G4int runId,
        G4int eventId,
        const PmtChannelKey& channelKey,
        const ScintillatorHit* scintillatorHit,
        const PhotocathodeHit& photocathodeHit,
        OpticalRecordingMode opticalRecordingMode,
        GeometryObjectType neutronHistoryTarget,
        G4int sumArrivedPhotons,
        G4int sumDetectedPhotons,
        G4double arrivalEfficiency,
        G4double detectionEfficiency,
        G4double sumArrivalEfficiency,
        G4double sumDetectionEfficiency
    );


private:
    /*
     * 以下のG4int変数は物理量ではなく、
     * G4AnalysisManagerから返されるROOT列番号を保持する。
     *
     * -1は「列がまだ作成されていない」ことを表す。
     */
    G4int fNtupleId = -1;


    // イベント・検出器識別情報の列番号
    G4int fRunIdColumnId = -1;
    G4int fEventIdColumnId = -1;

    G4int fDetectorCopyNoColumnId = -1;
    G4int fPmtCopyNoColumnId = -1;

    G4int fIsDetectorRepresentativeColumnId = -1;

    G4int fOpticalRecordingModeColumnId = -1;
    G4int fNeutronHistoryTargetColumnId = -1;


    // シンチレータ応答の列番号
    G4int fScintiEdepColumnId = -1;
    G4int fScintiEvisColumnId = -1;

    G4int fGeneratedPhotonsColumnId = -1;

    G4int fPrimaryNeutronInteractionCountColumnId = -1;
    G4int fNeutronLineageInteractionCountColumnId = -1;

    G4int fHasNeutronCaptureColumnId = -1;


    // シンチレータ内で最初に記録された反応の列番号
    G4int fHasFirstHitColumnId = -1;
    G4int fFirstHitTimeColumnId = -1;

    G4int fFirstHitGlobalXColumnId = -1;
    G4int fFirstHitGlobalYColumnId = -1;
    G4int fFirstHitGlobalZColumnId = -1;

    G4int fFirstHitLocalXColumnId = -1;
    G4int fFirstHitLocalYColumnId = -1;
    G4int fFirstHitLocalZColumnId = -1;

    G4int fFirstHitRadiusColumnId = -1;


    // 入射中性子情報の列番号
    G4int fHasIncidentNeutronDataColumnId = -1;

    G4int fIncidentNeutronTrackIdColumnId = -1;
    G4int fIncidentNeutronParentTrackIdColumnId = -1;
    G4int fIncidentNeutronRootPrimaryTrackIdColumnId = -1;

    G4int fIncidentDetectorCopyNoColumnId = -1;

    G4int fDetectorEntryTimeColumnId = -1;
    G4int fDetectorEntryEnergyColumnId = -1;

    G4int fPreDetectorScatterCountColumnId = -1;


    /*
     * 検出器へ入射する前の中性子散乱履歴。
     *
     * これらは列番号ではなく、
     * ROOTのvector branchが直接参照するデータバッファ。
     *
     * 同じ添字が同一の散乱を表す。
     */
    std::vector<G4int> fScatterTrackIds;
    std::vector<G4int> fScatterParentTrackIds;

    std::vector<G4int> fScatterObjectTypes;
    std::vector<G4int> fScatterObjectCopyNos;

    std::vector<G4int> fScatterProcessTypes;

    std::vector<G4double> fScatterTimes;
    std::vector<G4double> fScatterEnergiesBefore;
    std::vector<G4double> fScatterEnergiesAfter;


    // PMT集計情報の列番号
    G4int fArrivedPhotonsColumnId = -1;
    G4int fDetectedPhotonsColumnId = -1;

    G4int fArrivalEfficiencyColumnId = -1;
    G4int fDetectionEfficiencyColumnId = -1;

    G4int fSumArrivedPhotonsColumnId = -1;
    G4int fSumDetectedPhotonsColumnId = -1;

    G4int fSumArrivalEfficiencyColumnId = -1;
    G4int fSumDetectionEfficiencyColumnId = -1;


    /*
     * Detailedモードで記録する光学光子情報。
     *
     * これらも列番号ではなく、
     * ROOTのvector branchが直接参照するデータバッファ。
     *
     * 同じ添字が同一の検出光子を表す。
     */
    std::vector<G4double> fHitTimes;
    std::vector<G4double> fTransportTimes;
    std::vector<G4double> fTrackLengths;

    std::vector<G4double> fHitPosX;
    std::vector<G4double> fHitPosY;
    std::vector<G4double> fHitPosZ;

    std::vector<G4int> fScintillatorBoundaryCounts;
    std::vector<G4int> fLightGuideBoundaryCounts;

    std::vector<G4int> fScintillatorReflectionCounts;
    std::vector<G4int> fLightGuideReflectionCounts;
};

#endif