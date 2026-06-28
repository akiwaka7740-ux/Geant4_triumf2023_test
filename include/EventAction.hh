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
    void AddHitTime(G4double time) { fHitTimeList.push_back(time); }
    void AddHitPos(G4double x, G4double y, G4double z) { 
        fHitPosXList.push_back(x);
        fHitPosYList.push_back(y);
        fHitPosZList.push_back(z);
    }


    // RunAction でアドレスを紐づけるための参照Getter
    std::vector<G4double>& GetHitTimeListRef() { return fHitTimeList; }
    std::vector<G4double>& GetHitPosXListRef() { return fHitPosXList; }
    std::vector<G4double>& GetHitPosYListRef() { return fHitPosYList; }
    std::vector<G4double>& GetHitPosZListRef() { return fHitPosZList; }

private:
    std::vector<G4double> fHitTimeList;
    std::vector<G4double> fHitPosXList;
    std::vector<G4double> fHitPosYList;
    std::vector<G4double> fHitPosZList;
};
#endif