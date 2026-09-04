#ifndef SCINTISD_HH
#define SCINTISD_HH

#include "DetectorChannelKey.hh"

#include "G4VSensitiveDetector.hh"
#include "G4ThreeVector.hh"

#include <map>

struct ScintiEventData{
    G4double totalEdep = 0.0;
    G4double totalEvis = 0.0;
    G4double generatedPhotons = 0.0;

    G4double firstHitTime = -1.0;

    G4ThreeVector firstHitPosGlobal{
        -99999.0,
        -99999.0,
        -99999.0
    };

    G4ThreeVector firstHitPosLocal{
        -99999.0,
        -99999.0,
        -99999.0
    };

    G4int neutronInteractionCount = 0;
};


class ScintiSD : public G4VSensitiveDetector {
public:
    using ScintiDataMap = std::map<DetectorKey,ScintiEventData>;

    ScintiSD(G4String name);
    ~ScintiSD() override = default; //defaultとして中身も定義済み

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

    const ScintiDataMap& GetScintiData() const {return fScintiData;}

    const ScintiEventData* FindScintiData(const DetectorKey& detectorKey) const;
private:
    ScintiDataMap fScintiData;

    
};
#endif