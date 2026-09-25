#ifndef TRACKING_ACTION_HH
#define TRACKING_ACTION_HH

#include "G4UserTrackingAction.hh"

#include <memory>

class AnalysisConfig;
class G4Track;

class TrackingAction final
    : public G4UserTrackingAction {
public:
    explicit TrackingAction(
        std::shared_ptr<const AnalysisConfig>
            analysisConfig
    );

    ~TrackingAction() override = default;

    void PreUserTrackingAction(
        const G4Track* track
    ) override;

    void PostUserTrackingAction(
        const G4Track* track
    ) override;

private:
    std::shared_ptr<const AnalysisConfig>
        fAnalysisConfig;
};

#endif