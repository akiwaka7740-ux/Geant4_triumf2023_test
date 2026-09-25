#include "SteppingAction.hh"

#include "AnalysisConfig.hh"

#include "G4Neutron.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4Track.hh"


SteppingAction::SteppingAction(
    std::shared_ptr<const AnalysisConfig> config
)
    : fNeutronStepProcessor(config),
      fOpticalPhotonStepProcessor(config)
{
}


void SteppingAction::UserSteppingAction(
    const G4Step* step
)
{
    if (step == nullptr) {
        return;
    }

    const auto* track = step->GetTrack();

    if (track == nullptr) {
        return;
    }

    const auto* particle =
        track->GetDefinition();

    if (particle == nullptr) {
        return;
    }


    /*
     * 中性子のステップ処理
     */
    if (particle ==
        G4Neutron::NeutronDefinition()) {

        fNeutronStepProcessor.Process(step);
        return;
    }


    /*
     * 光学光子のステップ処理
     */
    if (particle ==
        G4OpticalPhoton::
            OpticalPhotonDefinition()) {

        fOpticalPhotonStepProcessor.Process(step);
        return;
    }


    /*
     * その他の粒子について、
     * SteppingActionでは処理を行わない。
     */
}