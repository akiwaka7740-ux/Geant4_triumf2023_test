#ifndef CATHODESD_HH
#define CATHODESD_HH

#include "G4VSensitiveDetector.hh"

class G4OpBoundaryProcess;
class G4Track;

class CathodeSD : public G4VSensitiveDetector {
public:
    CathodeSD(G4String name);
    ~CathodeSD() override = default;

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;
    G4double GetArrivedPhotons(G4int pmtIndex) const { return fPhotonArrivedCount[pmtIndex]; }
    G4double GetDetectedPhotons(G4int pmtIndex) const { return fPhotonDetectedCount[pmtIndex];}

private:
    G4int fPhotonArrivedCount[2];
    G4int fPhotonDetectedCount[2];
    //ベクトル要素についてはEventActionの管轄

    G4OpBoundaryProcess* fBoundary = nullptr;
};
#endif