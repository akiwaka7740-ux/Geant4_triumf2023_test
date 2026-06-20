#ifndef SENSITIVEDETECTOR
#define SENSITIVEDETECTOR

#include "G4VSensitiveDetector.hh"

#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

class SensitiveDetector : public G4VSensitiveDetector
{
public:
    SensitiveDetector(G4String);
    ~SensitiveDetector();

private:
    virtual G4bool ProcessHits(G4Step *, G4TouchableHistory *);

    virtual void Initialize(G4HCofThisEvent*) override;
    virtual void EndOfEvent(G4HCofThisEvent *) override;

    G4String fDetectorName;
    G4double fHitTime;
    G4double fHitTimeCapture;
    G4int fHitCounter;
    G4double fIncidentEnergy;
    G4double fEnergyDeposit;
    G4double fPosX, fPosY, fPosZ;

};

#endif

