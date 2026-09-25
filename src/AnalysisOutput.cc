#include "AnalysisOutput.hh"

#include "DetectorChannelKey.hh"
#include "IncidentNeutronData.hh"
#include "PhotocathodeHit.hh"
#include "ScintillatorHit.hh"

#include "G4AnalysisManager.hh"
#include "G4Exception.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>


namespace {

/*
 * ROOTへ書き込む物理量の無効値。
 *
 * AnalysisOutput.hh内のColumnIdの初期値-1とは
 * 意味が異なることに注意する。
 */
constexpr G4double kInvalidPosition = -99999.0;
constexpr G4double kInvalidTime = -99999.0;
constexpr G4double kInvalidEnergy = -99999.0;
constexpr G4int kMissingIdentifier = -1;

}


void AnalysisOutput::Book()
{
    auto* analysisManager =
        G4AnalysisManager::Instance();

    analysisManager->SetNtupleMerging(true);

    fNtupleId =
        analysisManager->CreateNtuple(
            "tree",
            "Detector and PMT channel data"
        );


    // イベント・チャンネル識別情報
    fRunIdColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "RunID"
        );

    fEventIdColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "EventID"
        );

    fDetectorCopyNoColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "DetectorCopyNo"
        );

    fPmtCopyNoColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PmtCopyNo"
        );

    fIsDetectorRepresentativeColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "IsDetectorRepresentative"
        );

    fOpticalRecordingModeColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "OpticalRecordingMode"
        );

    fNeutronHistoryTargetColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "NeutronHistoryTarget"
        );


    // シンチレータ応答
    fScintiEdepColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "ScintiEdep_MeV"
        );

    fScintiEvisColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "ScintiEvis_MeV"
        );

    fGeneratedPhotonsColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "GeneratedPhotons"
        );

    fPrimaryNeutronInteractionCountColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PrimaryNeutronInteractionCount"
        );

    fNeutronLineageInteractionCountColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "NeutronLineageInteractionCount"
        );

    fHasNeutronCaptureColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "HasNeutronCapture"
        );


    // 最初の反応
    fHasFirstHitColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "HasFirstHit"
        );

    fFirstHitTimeColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitTime_ns"
        );

    fFirstHitGlobalXColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitGlobalX_mm"
        );

    fFirstHitGlobalYColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitGlobalY_mm"
        );

    fFirstHitGlobalZColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitGlobalZ_mm"
        );

    fFirstHitLocalXColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitLocalX_mm"
        );

    fFirstHitLocalYColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitLocalY_mm"
        );

    fFirstHitLocalZColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitLocalZ_mm"
        );

    fFirstHitRadiusColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitRadius_mm"
        );


    // 入射中性子情報
    fHasIncidentNeutronDataColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "HasIncidentNeutronData"
        );

    fIncidentNeutronTrackIdColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "IncidentNeutronTrackID"
        );

    fIncidentNeutronParentTrackIdColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "IncidentNeutronParentTrackID"
        );

    fIncidentNeutronRootPrimaryTrackIdColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "IncidentNeutronRootPrimaryTrackID"
        );

    fIncidentDetectorCopyNoColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "IncidentDetectorCopyNo"
        );

    fDetectorEntryTimeColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "DetectorEntryTime_ns"
        );

    fDetectorEntryEnergyColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "DetectorEntryEnergy_MeV"
        );

    fPreDetectorScatterCountColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PreDetectorScatterCount"
        );


    // 入射前散乱履歴
    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "ScatterTrackIDs",
        fScatterTrackIds
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "ScatterParentTrackIDs",
        fScatterParentTrackIds
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "ScatterObjectTypes",
        fScatterObjectTypes
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "ScatterObjectCopyNos",
        fScatterObjectCopyNos
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "ScatterProcessTypes",
        fScatterProcessTypes
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "ScatterTimes_ns",
        fScatterTimes
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "ScatterEnergiesBefore_MeV",
        fScatterEnergiesBefore
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "ScatterEnergiesAfter_MeV",
        fScatterEnergiesAfter
    );


    // PMT集計情報
    fArrivedPhotonsColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "ArrivedPhotons"
        );

    fDetectedPhotonsColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "DetectedPhotons"
        );

    fArrivalEfficiencyColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "ArrivalEfficiency"
        );

    fDetectionEfficiencyColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "DetectionEfficiency"
        );

    fSumArrivedPhotonsColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PmtSumArrivedPhotons"
        );

    fSumDetectedPhotonsColumnId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PmtSumDetectedPhotons"
        );

    fSumArrivalEfficiencyColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "PmtSumArrivalEfficiency"
        );

    fSumDetectionEfficiencyColumnId =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "PmtSumDetectionEfficiency"
        );


    // Detailedモードの光学光子情報
    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitTimes_ns",
        fHitTimes
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "TransportTimes_ns",
        fTransportTimes
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "TrackLengths_mm",
        fTrackLengths
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitPosX_mm",
        fHitPosX
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitPosY_mm",
        fHitPosY
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitPosZ_mm",
        fHitPosZ
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "ScintillatorBoundaryCounts",
        fScintillatorBoundaryCounts
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "LightGuideBoundaryCounts",
        fLightGuideBoundaryCounts
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "ScintillatorReflectionCounts",
        fScintillatorReflectionCounts
    );

    analysisManager->CreateNtupleIColumn(
        fNtupleId,
        "LightGuideReflectionCounts",
        fLightGuideReflectionCounts
    );


    analysisManager->FinishNtuple(
        fNtupleId
    );
}


void AnalysisOutput::FillChannelRow(
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
)
{
    auto* analysisManager =
        G4AnalysisManager::Instance();


    /*
     * 各行の初期値。
     *
     * ScintillatorHitが存在しない場合でも、
     * 前の行の値が残らないように毎回初期化する。
     */
    G4double scintiEdep = 0.0;
    G4double scintiEvis = 0.0;

    G4int generatedPhotons = 0;

    G4int primaryNeutronInteractionCount = 0;
    G4int neutronLineageInteractionCount = 0;

    G4int hasNeutronCapture = 0;


    G4int hasFirstHit = 0;

    G4double firstHitTime = kInvalidTime;

    G4double firstHitGlobalX = kInvalidPosition;
    G4double firstHitGlobalY = kInvalidPosition;
    G4double firstHitGlobalZ = kInvalidPosition;

    G4double firstHitLocalX = kInvalidPosition;
    G4double firstHitLocalY = kInvalidPosition;
    G4double firstHitLocalZ = kInvalidPosition;

    G4double firstHitRadius = kInvalidPosition;


    G4int hasIncidentNeutronData = 0;

    G4int incidentNeutronTrackId = kMissingIdentifier;
    G4int incidentNeutronParentTrackId = kMissingIdentifier;
    G4int incidentNeutronRootPrimaryTrackId = kMissingIdentifier;

    G4int incidentDetectorCopyNo = kMissingIdentifier;

    G4double detectorEntryTime = kInvalidTime;
    G4double detectorEntryEnergy = kInvalidEnergy;

    G4int preDetectorScatterCount = 0;


    /*
     * 前のROOT行の散乱履歴が残らないように、
     * 毎回すべてのvectorを空にする。
     */
    fScatterTrackIds.clear();
    fScatterParentTrackIds.clear();

    fScatterObjectTypes.clear();
    fScatterObjectCopyNos.clear();

    fScatterProcessTypes.clear();

    fScatterTimes.clear();
    fScatterEnergiesBefore.clear();
    fScatterEnergiesAfter.clear();


    if (scintillatorHit != nullptr) {
        scintiEdep =
            scintillatorHit->GetTotalEdep() / MeV;

        scintiEvis =
            scintillatorHit->GetTotalEvis() / MeV;

        generatedPhotons =
            scintillatorHit->GetGeneratedPhotons();

        primaryNeutronInteractionCount =
            scintillatorHit
                ->GetPrimaryNeutronInteractionCount();

        neutronLineageInteractionCount =
            scintillatorHit
                ->GetNeutronLineageInteractionCount();

        hasNeutronCapture =
            scintillatorHit->HasNeutronCapture()
            ? 1
            : 0;


        if (scintillatorHit->HasFirstHit()) {
            hasFirstHit = 1;

            firstHitTime =
                scintillatorHit->GetFirstHitTime()
                / ns;

            const auto& globalPosition =
                scintillatorHit
                    ->GetFirstHitPosGlobal();

            firstHitGlobalX =
                globalPosition.x() / mm;

            firstHitGlobalY =
                globalPosition.y() / mm;

            firstHitGlobalZ =
                globalPosition.z() / mm;


            const auto& localPosition =
                scintillatorHit
                    ->GetFirstHitPosLocal();

            firstHitLocalX =
                localPosition.x() / mm;

            firstHitLocalY =
                localPosition.y() / mm;

            firstHitLocalZ =
                localPosition.z() / mm;

            firstHitRadius =
                std::sqrt(
                    firstHitLocalX
                        * firstHitLocalX
                    + firstHitLocalY
                        * firstHitLocalY
                );
        }


        if (
            scintillatorHit
                ->HasIncidentNeutronData()
        ) {
            hasIncidentNeutronData = 1;

            const auto& incidentData =
                scintillatorHit
                    ->GetIncidentNeutronData();

            incidentNeutronTrackId =
                incidentData.trackId;

            incidentNeutronParentTrackId =
                incidentData.parentTrackId;

            incidentNeutronRootPrimaryTrackId =
                incidentData.rootPrimaryTrackId;

            incidentDetectorCopyNo =
                incidentData.detectorCopyNo;

            detectorEntryTime =
                incidentData.detectorEntryTime
                / ns;

            detectorEntryEnergy =
                incidentData.detectorEntryEnergy
                / MeV;


            const auto& scatterHistory =
                incidentData
                    .preDetectorScatterHistory;

            preDetectorScatterCount =
                static_cast<G4int>(
                    scatterHistory.size()
                );


            fScatterTrackIds.reserve(
                scatterHistory.size()
            );

            fScatterParentTrackIds.reserve(
                scatterHistory.size()
            );

            fScatterObjectTypes.reserve(
                scatterHistory.size()
            );

            fScatterObjectCopyNos.reserve(
                scatterHistory.size()
            );

            fScatterProcessTypes.reserve(
                scatterHistory.size()
            );

            fScatterTimes.reserve(
                scatterHistory.size()
            );

            fScatterEnergiesBefore.reserve(
                scatterHistory.size()
            );

            fScatterEnergiesAfter.reserve(
                scatterHistory.size()
            );


            for (const auto& scatter
                 : scatterHistory) {
                fScatterTrackIds.push_back(
                    scatter.trackId
                );

                fScatterParentTrackIds.push_back(
                    scatter.parentTrackId
                );

                fScatterObjectTypes.push_back(
                    static_cast<G4int>(
                        scatter.objectType
                    )
                );

                fScatterObjectCopyNos.push_back(
                    scatter.objectCopyNo
                );

                fScatterProcessTypes.push_back(
                    static_cast<G4int>(
                        scatter.process
                    )
                );

                fScatterTimes.push_back(
                    scatter.globalTime / ns
                );

                fScatterEnergiesBefore.push_back(
                    scatter.kineticEnergyBefore
                    / MeV
                );

                fScatterEnergiesAfter.push_back(
                    scatter.kineticEnergyAfter
                    / MeV
                );
            }
        }
    }


    /*
     * PhotocathodeHit内のDetailed用vectorが
     * 同じ長さであることを確認する。
     */
    if (
        !photocathodeHit
            .HasConsistentDetailSizes()
    ) {
        G4Exception(
            "AnalysisOutput::FillChannelRow",
            "AnalysisOutput001",
            FatalException,
            "PhotocathodeHit detail vectors "
            "have inconsistent sizes."
        );
    }


    /*
     * 光学光子詳細情報も、前の行の値を残さないよう
     * 毎回初期化する。
     */
    fHitTimes.clear();
    fTransportTimes.clear();
    fTrackLengths.clear();

    fHitPosX.clear();
    fHitPosY.clear();
    fHitPosZ.clear();


    const auto& hitTimes =
        photocathodeHit.GetHitTimes();

    const auto& transportTimes =
        photocathodeHit.GetTransportTimes();

    const auto& trackLengths =
        photocathodeHit.GetTrackLengths();

    const auto& hitPositions =
        photocathodeHit.GetHitPositions();


    fHitTimes.reserve(hitTimes.size());
    fTransportTimes.reserve(
        transportTimes.size()
    );

    fTrackLengths.reserve(
        trackLengths.size()
    );

    fHitPosX.reserve(hitPositions.size());
    fHitPosY.reserve(hitPositions.size());
    fHitPosZ.reserve(hitPositions.size());


    for (const auto value : hitTimes) {
        fHitTimes.push_back(value / ns);
    }

    for (const auto value : transportTimes) {
        fTransportTimes.push_back(
            value / ns
        );
    }

    for (const auto value : trackLengths) {
        fTrackLengths.push_back(
            value / mm
        );
    }

    for (const auto& position
         : hitPositions) {
        fHitPosX.push_back(
            position.x() / mm
        );

        fHitPosY.push_back(
            position.y() / mm
        );

        fHitPosZ.push_back(
            position.z() / mm
        );
    }


    fScintillatorBoundaryCounts =
        photocathodeHit
            .GetScintillatorBoundaryCounts();

    fLightGuideBoundaryCounts =
        photocathodeHit
            .GetLightGuideBoundaryCounts();

    fScintillatorReflectionCounts =
        photocathodeHit
            .GetScintillatorReflectionCounts();

    fLightGuideReflectionCounts =
        photocathodeHit
            .GetLightGuideReflectionCounts();


    const G4int isDetectorRepresentative =
        channelKey.pmtCopyNo == 0
        ? 1
        : 0;


    /*
     * scalar列の書き込みを簡潔にするための
     * ローカル関数。
     */
    auto fillI =
        [&](G4int columnId, G4int value) {
            analysisManager
                ->FillNtupleIColumn(
                    fNtupleId,
                    columnId,
                    value
                );
        };

    auto fillD =
        [&](G4int columnId, G4double value) {
            analysisManager
                ->FillNtupleDColumn(
                    fNtupleId,
                    columnId,
                    value
                );
        };


    // イベント・チャンネル識別情報
    fillI(
        fRunIdColumnId,
        runId
    );

    fillI(
        fEventIdColumnId,
        eventId
    );

    fillI(
        fDetectorCopyNoColumnId,
        channelKey.detector.detectorCopyNo
    );

    fillI(
        fPmtCopyNoColumnId,
        channelKey.pmtCopyNo
    );

    fillI(
        fIsDetectorRepresentativeColumnId,
        isDetectorRepresentative
    );

    fillI(
        fOpticalRecordingModeColumnId,
        static_cast<G4int>(
            opticalRecordingMode
        )
    );

    fillI(
        fNeutronHistoryTargetColumnId,
        static_cast<G4int>(
            neutronHistoryTarget
        )
    );


    // シンチレータ応答
    fillD(
        fScintiEdepColumnId,
        scintiEdep
    );

    fillD(
        fScintiEvisColumnId,
        scintiEvis
    );

    fillI(
        fGeneratedPhotonsColumnId,
        generatedPhotons
    );

    fillI(
        fPrimaryNeutronInteractionCountColumnId,
        primaryNeutronInteractionCount
    );

    fillI(
        fNeutronLineageInteractionCountColumnId,
        neutronLineageInteractionCount
    );

    fillI(
        fHasNeutronCaptureColumnId,
        hasNeutronCapture
    );


    // 最初の反応
    fillI(
        fHasFirstHitColumnId,
        hasFirstHit
    );

    fillD(
        fFirstHitTimeColumnId,
        firstHitTime
    );

    fillD(
        fFirstHitGlobalXColumnId,
        firstHitGlobalX
    );

    fillD(
        fFirstHitGlobalYColumnId,
        firstHitGlobalY
    );

    fillD(
        fFirstHitGlobalZColumnId,
        firstHitGlobalZ
    );

    fillD(
        fFirstHitLocalXColumnId,
        firstHitLocalX
    );

    fillD(
        fFirstHitLocalYColumnId,
        firstHitLocalY
    );

    fillD(
        fFirstHitLocalZColumnId,
        firstHitLocalZ
    );

    fillD(
        fFirstHitRadiusColumnId,
        firstHitRadius
    );


    // 入射中性子情報
    fillI(
        fHasIncidentNeutronDataColumnId,
        hasIncidentNeutronData
    );

    fillI(
        fIncidentNeutronTrackIdColumnId,
        incidentNeutronTrackId
    );

    fillI(
        fIncidentNeutronParentTrackIdColumnId,
        incidentNeutronParentTrackId
    );

    fillI(
        fIncidentNeutronRootPrimaryTrackIdColumnId,
        incidentNeutronRootPrimaryTrackId
    );

    fillI(
        fIncidentDetectorCopyNoColumnId,
        incidentDetectorCopyNo
    );

    fillD(
        fDetectorEntryTimeColumnId,
        detectorEntryTime
    );

    fillD(
        fDetectorEntryEnergyColumnId,
        detectorEntryEnergy
    );

    fillI(
        fPreDetectorScatterCountColumnId,
        preDetectorScatterCount
    );


    // PMT情報
    fillI(
        fArrivedPhotonsColumnId,
        photocathodeHit.GetArrivedPhotons()
    );

    fillI(
        fDetectedPhotonsColumnId,
        photocathodeHit.GetDetectedPhotons()
    );

    fillD(
        fArrivalEfficiencyColumnId,
        arrivalEfficiency
    );

    fillD(
        fDetectionEfficiencyColumnId,
        detectionEfficiency
    );

    fillI(
        fSumArrivedPhotonsColumnId,
        sumArrivedPhotons
    );

    fillI(
        fSumDetectedPhotonsColumnId,
        sumDetectedPhotons
    );

    fillD(
        fSumArrivalEfficiencyColumnId,
        sumArrivalEfficiency
    );

    fillD(
        fSumDetectionEfficiencyColumnId,
        sumDetectionEfficiency
    );


    /*
     * vector branchはAddNtupleRow時点の
     * バッファ内容がROOTへ保存される。
     */
    analysisManager->AddNtupleRow(
        fNtupleId
    );
}