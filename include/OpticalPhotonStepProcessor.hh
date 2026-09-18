#ifndef OPTICAL_PHOTON_STEP_PROCESSOR_HH
#define OPTICAL_PHOTON_STEP_PROCESSOR_HH

#include "OpticalPhotonTrackInfo.hh"

#include "G4OpBoundaryProcess.hh"
#include "G4StepStatus.hh"

class G4Track;
class G4VProcess;

struct OpticalPhotonStepContext {
    G4Track* track = nullptr;

    OpticalPhotonRegion preRegion =
        OpticalPhotonRegion::Other;

    OpticalPhotonRegion postRegion =
        OpticalPhotonRegion::Other;

    const G4VProcess* processDefinedStep = nullptr;

    G4StepStatus stepStatus = fUndefined;

    G4OpBoundaryProcessStatus boundaryStatus =
        Undefined;
};

class OpticalPhotonStepProcessor {
public:
    OpticalPhotonStepProcessor() = default;
    ~OpticalPhotonStepProcessor() = default;

    void Process(
        const OpticalPhotonStepContext& context
    ) const;

private:
    static G4bool IsReflection(
        G4OpBoundaryProcessStatus status
    );
};

#endif