#ifndef CATHODESD_HH
#define CATHODESD_HH

#include "G4VSensitiveDetector.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

#include <vector>


class CathodeSD : public G4VSensitiveDetector {
public:
    CathodeSD(G4String name);
    ~CathodeSD() override = default;

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

private:
    G4int fPhotonCount[2];
    //ベクトル要素についてはEventActionの管轄
};
#endif