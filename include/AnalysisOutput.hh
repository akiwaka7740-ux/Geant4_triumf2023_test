#ifndef ANALYSISOUTPUT_HH
#define ANALYSISOUTPUT_HH

#include "globals.hh"

#include <vector>

struct ScintiEventData;
struct PmtEventData;
struct PmtChannelKey;

class AnalysisOutput {
public:
    AnalysisOutput() = default;
    ~AnalysisOutput() = default;

    void Book();

    void FillChannelRow(
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
    );

private:
    G4int fNtupleId = -1;

    G4int fRunId = -1;
    G4int fEventId = -1;
    G4int fDetectorCopyNo = -1;
    G4int fPmtCopyNo = -1;
    G4int fIsDetectorRepresentative = -1;

    G4int fScintiEdep = -1;
    G4int fScintiEvis = -1;
    G4int fGeneratedPhotons = -1;
    G4int fNeutronInteractionCount = -1;
    G4int fFirstHitTime = -1;

    G4int fFirstHitGlobalX = -1;
    G4int fFirstHitGlobalY = -1;
    G4int fFirstHitGlobalZ = -1;

    G4int fFirstHitLocalX = -1;
    G4int fFirstHitLocalY = -1;
    G4int fFirstHitLocalZ = -1;
    G4int fFirstHitRadius = -1;

    G4int fArrivedPhotons = -1;
    G4int fDetectedPhotons = -1;
    G4int fArrivalEfficiency = -1;
    G4int fDetectionEfficiency = -1;

    G4int fSumArrivedPhotons = -1;
    G4int fSumDetectedPhotons = -1;
    G4int fSumArrivalEfficiency = -1;
    G4int fSumDetectionEfficiency = -1;

    // vector branchが参照する実体
    std::vector<G4double> fHitTimes;
    std::vector<G4double> fHitPosX;
    std::vector<G4double> fHitPosY;
    std::vector<G4double> fHitPosZ;
};

#endif