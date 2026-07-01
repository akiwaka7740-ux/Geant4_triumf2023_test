#ifndef SCINTISD_HH
#define SCINTISD_HH

#include "G4VSensitiveDetector.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4ThreeVector.hh"

class ScintiSD : public G4VSensitiveDetector {
public:
    ScintiSD(G4String name);
    ~ScintiSD() override = default; //defaultとして中身も定義済み

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

private:
    G4double fTotalEdep;
    G4double fGenerateedPhotons;
    G4ThreeVector fFirstHitPos;
    G4int fHitCount;
};
#endif