#include "PhotocathodeHit.hh"

#include "G4ios.hh"


G4ThreadLocal
G4Allocator<PhotocathodeHit>*
    PhotocathodeHitAllocator = nullptr;


PhotocathodeHit::PhotocathodeHit(
    const PmtChannelKey& channelKey
)
    : G4VHit(),
      fChannelKey(channelKey)
{
}


G4bool PhotocathodeHit::operator==(
    const PhotocathodeHit& other
) const
{
    return fChannelKey ==
           other.fChannelKey;
}


void PhotocathodeHit::Print()
{
    G4cout
        << "PhotocathodeHit:"
        << " detectorCopyNo="
        << fChannelKey.detector.detectorCopyNo

        << ", pmtCopyNo="
        << fChannelKey.pmtCopyNo

        << ", arrivedPhotons="
        << fArrivedPhotons

        << ", detectedPhotons="
        << fDetectedPhotons

        << ", detailedPhotonCount="
        << fHitTimes.size()

        << ", detailSizesConsistent="
        << HasConsistentDetailSizes()

        << G4endl;
}