#ifndef OPTICAL_PHOTON_TRACKING_ACTION_HH
#define OPTICAL_PHOTON_TRACKING_ACTION_HH

#include "G4UserTrackingAction.hh"

class G4Track;

class OpticalPhotonTrackingAction : public G4UserTrackingAction {
public:
    OpticalPhotonTrackingAction() = default;
    ~OpticalPhotonTrackingAction() override = default;

    void PreUserTrackingAction(const G4Track* track) override;
};

#endif