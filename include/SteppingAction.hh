#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"

#include "NeutronStepProcessor.hh"
#include "OpticalPhotonStepProcessor.hh"

#include <memory>

class AnalysisConfig;
class G4Step;


class SteppingAction final
    : public G4UserSteppingAction {
public:
    explicit SteppingAction(
        std::shared_ptr<const AnalysisConfig> config
    );

    ~SteppingAction() override = default;

    /*
     * ステップが発生するたびに
     * Geant4カーネルから呼ばれる。
     */
    void UserSteppingAction(
        const G4Step* step
    ) override;


private:
    NeutronStepProcessor
        fNeutronStepProcessor;

    OpticalPhotonStepProcessor
        fOpticalPhotonStepProcessor;
};

#endif