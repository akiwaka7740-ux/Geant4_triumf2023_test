#ifndef SCINTISD_HH
#define SCINTISD_HH

#include "G4VSensitiveDetector.hh"
#include "G4ThreeVector.hh"


class ScintiSD : public G4VSensitiveDetector {
public:
    ScintiSD(G4String name);
    ~ScintiSD() override = default; //defaultとして中身も定義済み

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

    G4double GetGeneratedPhotons() const { return fGeneratedPhotons; };

    G4ThreeVector GetFirstHitPosGlobal() const { return fFirstHitPosGlobal; }
    G4ThreeVector GetFirstHitPosLocal()  const { return fFirstHitPosLocal; }

private:
    G4double fTotalEdep;
    G4double fTotalEvis;
    G4double fGeneratedPhotons;
    G4double fFirstHitTime;
    G4ThreeVector fFirstHitPosGlobal;
    G4ThreeVector fFirstHitPosLocal;
    G4int fNeutronInteractionCount;
    
};
#endif