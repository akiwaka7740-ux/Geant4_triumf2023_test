#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4RunManager.hh"
#include <vector>

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

private:
    std::vector<G4double> fHitTimeList[2];
    std::vector<G4double> fHitPosXList[2];
    std::vector<G4double> fHitPosYList[2];
    std::vector<G4double> fHitPosZList[2];
  
};
#endif