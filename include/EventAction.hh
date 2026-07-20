#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <vector>

class AnalysisOutput;

class EventAction : public G4UserEventAction {
public:
    EventAction() = default;
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    // CathodeSD から時刻を投げ込むための窓口
    void AddHitTime(G4int id, G4double time) { fHitTimeList[id].push_back(time); }
    void AddHitPos(G4int id, G4double x, G4double y, G4double z) { 
        fHitPosXList[id].push_back(x);
        fHitPosYList[id].push_back(y);
        fHitPosZList[id].push_back(z);
    }

    std::vector<G4double>& GetHitTimeListRef(G4int id) { return fHitTimeList[id]; }
    std::vector<G4double>& GetHitPosXListRef(G4int id) { return fHitPosXList[id]; }
    std::vector<G4double>& GetHitPosYListRef(G4int id) { return fHitPosYList[id]; }
    std::vector<G4double>& GetHitPosZListRef(G4int id) { return fHitPosZList[id]; }

    std::vector<G4double>& GetScintiPosGlobalRef() { return fScintiPosGlobal; }
    std::vector<G4double>& GetScintiPosLocalRef()  { return fScintiPosLocal; }

    G4double GetEfficiency(G4int id) const {
        if (id == 0) return fEff[0];
        else if (id == 1) return fEff[1];
        else return 0.0;
    }

    void SetAnalysisOutput(AnalysisOutput* analysisOutput) { fAnalysisOutput = analysisOutput; }
    AnalysisOutput* GetAnalysisOutput() const { return fAnalysisOutput; }


private:
    std::vector<G4double> fHitTimeList[2];
    std::vector<G4double> fHitPosXList[2];
    std::vector<G4double> fHitPosYList[2];
    std::vector<G4double> fHitPosZList[2];

    std::vector<G4double> fScintiPosGlobal;
    std::vector<G4double> fScintiPosLocal;
    G4double fRadius;

    G4double fEff[2] = {0.0, 0.0};

    AnalysisOutput* fAnalysisOutput = nullptr;
  
};
#endif