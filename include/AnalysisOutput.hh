#ifndef ANALYSISOUTPUT_HH
#define ANALYSISOUTPUT_HH

#include "globals.hh"

class EventAction;

class AnalysisOutput {
public:
    AnalysisOutput() = default;
    ~AnalysisOutput() = default;

    void Book(EventAction* eventAction);

    void FillScinti(
        G4double edep,
        G4double evis,
        G4double generatedPhotons,
        G4int hitCount
    );

    void FillPMTPhotons(
        G4int pmt1Photons,
        G4int pmt2Photons
    );

    void FillEventSummary(
        G4double scintiHitRadius,
        G4double pmt1Efficiency,
        G4double pmt2Efficiency
    );

    void AddRow();

private:
    //以下は物理量に紐づくID(物理量の値そのものではないので注意)
    G4int fScintiEdep = -1;
    G4int fScintiEvis = -1;
    G4int fScintiPhotons = -1;
    G4int fScintiInteractionCount = -1;
    G4int fScintiHitPosGlobal = -1;
    G4int fScintiHitPosLocal = -1;
    G4int fScintiHitPosRadius = -1;

    G4int fPMTSumPhotons = -1;
    G4int fPMTSumEfficiency = -1;
    G4int fPMT1Photons = -1;
    G4int fPMT1Efficiency = -1;
    G4int fPMT2Photons = -1;
    G4int fPMT2Efficiency = -1;

    G4int fPMT1HitTimes = -1;
    G4int fPMT1HitPosX = -1;
    G4int fPMT1HitPosY = -1;
    G4int fPMT1HitPosZ = -1;

    G4int fPMT2HitTimes = -1;
    G4int fPMT2HitPosX = -1;
    G4int fPMT2HitPosY = -1;
    G4int fPMT2HitPosZ = -1;
};

#endif