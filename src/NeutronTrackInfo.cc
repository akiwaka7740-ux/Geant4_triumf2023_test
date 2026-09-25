#include "NeutronTrackInfo.hh"

#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

NeutronTrackInfo::NeutronTrackInfo(
    G4int rootPrimaryTrackId
)
    : G4VUserTrackInformation(
          "NeutronTrackInfo"
      ),
      fRootPrimaryTrackId(
          rootPrimaryTrackId
      )
{
}

NeutronTrackInfo::NeutronTrackInfo(
    const NeutronTrackInfo& other
)
    : G4VUserTrackInformation(other),
      fRootPrimaryTrackId(
          other.fRootPrimaryTrackId
      ),
      fHasEnteredTarget(
          other.fHasEnteredTarget
      ),
      fEnteredDetectorCopyNo(
          other.fEnteredDetectorCopyNo
      ),
      fDetectorEntryTime(
          other.fDetectorEntryTime
      ),
      fDetectorEntryEnergy(
          other.fDetectorEntryEnergy
      ),
      fScatterHistory(
          other.fScatterHistory
      )
{
}

void NeutronTrackInfo::RecordScatter(
    const NeutronScatterRecord& record
)
{
    if (fHasEnteredTarget) {
        return;
    }

    fScatterHistory.push_back(record);
}

void NeutronTrackInfo::MarkTargetEntry(
    G4int detectorCopyNo,
    G4double entryTime,
    G4double entryEnergy
)
{
    if (fHasEnteredTarget) {
        return;
    }

    fHasEnteredTarget = true;

    fEnteredDetectorCopyNo =
        detectorCopyNo;

    fDetectorEntryTime =
        entryTime;

    fDetectorEntryEnergy =
        entryEnergy;
}



void NeutronTrackInfo::
SetLastScatterOutgoingEnergy(
    G4double energy
)
{
    if (fScatterHistory.empty()) {
        return;
    }

    fScatterHistory.back()
        .kineticEnergyAfter = energy;
}

void NeutronTrackInfo::Print() const
{
    G4cout
        << "NeutronTrackInfo:"
        << " rootPrimaryTrackId="
        << fRootPrimaryTrackId
        << ", enteredTarget="
        << fHasEnteredTarget
        << ", detectorCopyNo="
        << fEnteredDetectorCopyNo
        << ", entryTime="
        << fDetectorEntryTime / ns
        << " ns"
        << ", entryEnergy="
        << fDetectorEntryEnergy / MeV
        << " MeV"
        << ", scatterCount="
        << fScatterHistory.size()
        << G4endl;
}