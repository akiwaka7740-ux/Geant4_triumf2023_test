#include "OpticalPhotonTrackingAction.hh"

#include "OpticalPhotonTrackInfo.hh"

#include "G4OpticalPhoton.hh"
#include "G4Track.hh"
#include "G4TrackingManager.hh"

void OpticalPhotonTrackingAction::PreUserTrackingAction(
    const G4Track* track
)
{
    if (track == nullptr) {
        return;
    }

    if (track->GetDefinition()
        != G4OpticalPhoton::OpticalPhotonDefinition()) {
        return;
    }

    // 他のUserTrackInformationを上書きしない
    if (track->GetUserInformation() != nullptr) {
        return;
    }

    fpTrackingManager->SetUserTrackInformation(
        new OpticalPhotonTrackInfo()
    );
}