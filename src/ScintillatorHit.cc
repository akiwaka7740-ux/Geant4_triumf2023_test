#include "ScintillatorHit.hh"

#include "G4SystemOfUnits.hh"
#include "G4ios.hh"


G4ThreadLocal
G4Allocator<ScintillatorHit>*
    ScintillatorHitAllocator = nullptr;


ScintillatorHit::ScintillatorHit(
    const DetectorKey& detectorKey
)
    : G4VHit(),
      fDetectorKey(detectorKey)
{
}


G4bool ScintillatorHit::operator==(
    const ScintillatorHit& other
) const
{
    return fDetectorKey ==
           other.fDetectorKey;
}


void ScintillatorHit::Print()
{
    G4cout
        << "ScintillatorHit:"
        << " detectorCopyNo="
        << fDetectorKey.detectorCopyNo

        << ", totalEdep="
        << fTotalEdep / MeV
        << " MeV"

        << ", totalEvis="
        << fTotalEvis / MeV
        << " MeV"

        << ", generatedPhotons="
        << fGeneratedPhotons

        << ", primaryNeutronInteractions="
        << fPrimaryNeutronInteractionCount

        << ", neutronLineageInteractions="
        << fNeutronLineageInteractionCount

        << ", hasNeutronCapture="
        << fHasNeutronCapture

        << ", hasIncidentNeutronData="
        << fIncidentNeutron.valid;

    if (HasFirstHit()) {
        G4cout
            << ", firstHitTime="
            << fFirstHitTime / ns
            << " ns";
    }

    if (fIncidentNeutron.valid) {
        G4cout
            << ", incidentNeutronTrackId="
            << fIncidentNeutron.trackId

            << ", rootPrimaryTrackId="
            << fIncidentNeutron.rootPrimaryTrackId

            << ", detectorEntryTime="
            << fIncidentNeutron.detectorEntryTime / ns
            << " ns"

            << ", detectorEntryEnergy="
            << fIncidentNeutron.detectorEntryEnergy / MeV
            << " MeV"

            << ", preDetectorScatterCount="
            << fIncidentNeutron
                   .preDetectorScatterHistory
                   .size();
    }

    G4cout << G4endl;
}