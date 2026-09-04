#include "AnalysisOutput.hh"
#include "ScintiSD.hh"
#include "CathodeSD.hh"
#include "DetectorChannelKey.hh"

#include "G4AnalysisManager.hh"

#include <cmath>

void AnalysisOutput::Book()
{
    auto* analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetNtupleMerging(true);

    fNtupleId =
        analysisManager->CreateNtuple(
            "tree",
            "Detector and PMT channel data"
        );

    fRunId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "RunID"
        );

    fEventId =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "EventID"
        );

    fDetectorCopyNo =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "DetectorCopyNo"
        );

    fPmtCopyNo =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PmtCopyNo"
        );

    fIsDetectorRepresentative =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "IsDetectorRepresentative"
        );

    fScintiEdep =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "ScintiEdep"
        );

    fScintiEvis =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "ScintiEvis"
        );

    fGeneratedPhotons =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "GeneratedPhotons"
        );

    fNeutronInteractionCount =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "NeutronInteractionCount"
        );

    fFirstHitTime =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitTime"
        );

    fFirstHitGlobalX =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitGlobalX"
        );

    fFirstHitGlobalY =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitGlobalY"
        );

    fFirstHitGlobalZ =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitGlobalZ"
        );

    fFirstHitLocalX =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitLocalX"
        );

    fFirstHitLocalY =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitLocalY"
        );

    fFirstHitLocalZ =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitLocalZ"
        );

    fFirstHitRadius =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "FirstHitRadius"
        );

    fArrivedPhotons =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "ArrivedPhotons"
        );

    fDetectedPhotons =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "DetectedPhotons"
        );

    fArrivalEfficiency =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "ArrivalEfficiency"
        );

    fDetectionEfficiency =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "DetectionEfficiency"
        );

    fSumArrivedPhotons =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PmtSumArrivedPhotons"
        );

    fSumDetectedPhotons =
        analysisManager->CreateNtupleIColumn(
            fNtupleId,
            "PmtSumDetectedPhotons"
        );

    fSumArrivalEfficiency =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "PmtSumArrivalEfficiency"
        );

    fSumDetectionEfficiency =
        analysisManager->CreateNtupleDColumn(
            fNtupleId,
            "PmtSumDetectionEfficiency"
        );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitTimes",
        fHitTimes
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitPosX",
        fHitPosX
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitPosY",
        fHitPosY
    );

    analysisManager->CreateNtupleDColumn(
        fNtupleId,
        "HitPosZ",
        fHitPosZ
    );

    analysisManager->FinishNtuple(
        fNtupleId
    );
}

void AnalysisOutput::FillChannelRow(
    G4int runId,
    G4int eventId,
    const PmtChannelKey& channelKey,
    const ScintiEventData& scintiData,
    const PmtEventData& pmtData,
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

    // vector branch用バッファ
    fHitTimes = pmtData.hitTimes;

    fHitPosX.clear();
    fHitPosY.clear();
    fHitPosZ.clear();

    fHitPosX.reserve(
        pmtData.hitPositions.size()
    );
    fHitPosY.reserve(
        pmtData.hitPositions.size()
    );
    fHitPosZ.reserve(
        pmtData.hitPositions.size()
    );

    for (const auto& position
         : pmtData.hitPositions) {
        fHitPosX.push_back(position.x());
        fHitPosY.push_back(position.y());
        fHitPosZ.push_back(position.z());
    }

    G4double firstHitRadius = -99999.0;

    if (scintiData.neutronInteractionCount > 0) {
        firstHitRadius = std::sqrt(
            scintiData.firstHitPosLocal.x()
                * scintiData.firstHitPosLocal.x()
            + scintiData.firstHitPosLocal.y()
                * scintiData.firstHitPosLocal.y()
        );
    }

    const G4int isRepresentative =
        channelKey.pmtCopyNo == 0
        ? 1
        : 0;

    auto fillI =
        [&](G4int columnId, G4int value) {
            analysisManager->FillNtupleIColumn(
                fNtupleId,
                columnId,
                value
            );
        };

    auto fillD =
        [&](G4int columnId, G4double value) {
            analysisManager->FillNtupleDColumn(
                fNtupleId,
                columnId,
                value
            );
        };

    fillI(fRunId, runId);
    fillI(fEventId, eventId);

    fillI(
        fDetectorCopyNo,
        channelKey.detector.detectorCopyNo
    );

    fillI(
        fPmtCopyNo,
        channelKey.pmtCopyNo
    );

    fillI(
        fIsDetectorRepresentative,
        isRepresentative
    );

    fillD(fScintiEdep, scintiData.totalEdep);
    fillD(fScintiEvis, scintiData.totalEvis);
    fillD(
        fGeneratedPhotons,
        scintiData.generatedPhotons
    );

    fillI(
        fNeutronInteractionCount,
        scintiData.neutronInteractionCount
    );

    fillD(
        fFirstHitTime,
        scintiData.firstHitTime
    );

    fillD(
        fFirstHitGlobalX,
        scintiData.firstHitPosGlobal.x()
    );
    fillD(
        fFirstHitGlobalY,
        scintiData.firstHitPosGlobal.y()
    );
    fillD(
        fFirstHitGlobalZ,
        scintiData.firstHitPosGlobal.z()
    );

    fillD(
        fFirstHitLocalX,
        scintiData.firstHitPosLocal.x()
    );
    fillD(
        fFirstHitLocalY,
        scintiData.firstHitPosLocal.y()
    );
    fillD(
        fFirstHitLocalZ,
        scintiData.firstHitPosLocal.z()
    );

    fillD(
        fFirstHitRadius,
        firstHitRadius
    );

    fillI(
        fArrivedPhotons,
        pmtData.arrivedPhotons
    );

    fillI(
        fDetectedPhotons,
        pmtData.detectedPhotons
    );

    fillD(
        fArrivalEfficiency,
        arrivalEfficiency
    );

    fillD(
        fDetectionEfficiency,
        detectionEfficiency
    );

    fillI(
        fSumArrivedPhotons,
        sumArrivedPhotons
    );

    fillI(
        fSumDetectedPhotons,
        sumDetectedPhotons
    );

    fillD(
        fSumArrivalEfficiency,
        sumArrivalEfficiency
    );

    fillD(
        fSumDetectionEfficiency,
        sumDetectionEfficiency
    );

    analysisManager->AddNtupleRow(
        fNtupleId
    );
}