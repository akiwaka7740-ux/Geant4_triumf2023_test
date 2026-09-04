#ifndef CATHODESD_HH
#define CATHODESD_HH

#include "DetectorChannelKey.hh"

#include "G4VSensitiveDetector.hh"
#include "G4ThreeVector.hh"

#include <map>
#include <set>
#include <vector>

struct PmtEventData {
    G4int arrivedPhotons = 0;
    G4int detectedPhotons = 0;

    std::vector<G4double> hitTimes;
    std::vector<G4ThreeVector> hitPositions;
};

class G4Track;

class CathodeSD : public G4VSensitiveDetector {
public:
     using PmtDataMap = std::map<PmtChannelKey,PmtEventData>;

    CathodeSD(G4String name);
    ~CathodeSD() override = default;

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist) override; //毎回stepごとに呼ばれる
    G4bool ProcessBoundaryHit(const G4Step* step);
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

    const PmtDataMap& GetPmtData() const {return fPmtData;}
    const PmtEventData* FindPmtData(const PmtChannelKey& channelKey) const;

    void RegisterChannel(const PmtChannelKey& channelKey);

private:
    std::set<PmtChannelKey> fRegisteredChannels;
    PmtDataMap fPmtData;

};
#endif