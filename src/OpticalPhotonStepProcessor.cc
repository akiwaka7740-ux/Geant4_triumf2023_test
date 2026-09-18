#include "OpticalPhotonStepProcessor.hh"

#include "G4Track.hh"


void OpticalPhotonStepProcessor::Process(
    const OpticalPhotonStepContext& context
) const
{
    if (context.track == nullptr) {
        return;
    }

    if (context.stepStatus != fGeomBoundary) {
        return;
    }

    if (context.preRegion == OpticalPhotonRegion::Other) {
        return;
    }

    if (context.boundaryStatus == Undefined
        || context.boundaryStatus == NotAtBoundary
        || context.boundaryStatus == StepTooSmall
        ) {
        return;
    }

    auto *trackInfo = dynamic_cast<OpticalPhotonTrackInfo*>(
        context.track->GetUserInformation()
    );

    if (trackInfo == nullptr) {
        return;
    }

    trackInfo->RecordBoundaryInteraction(
        context.preRegion,
        IsReflection(context.boundaryStatus)
    );
}

G4bool OpticalPhotonStepProcessor::IsReflection(
    G4OpBoundaryProcessStatus status
)
{
    switch (status) {
    //caseに当てはまる場合はtrueを返す
    case FresnelReflection:
    case TotalInternalReflection:
    case LambertianReflection:
    case LobeReflection:
    case SpikeReflection:
    case BackScattering:
        return true;

    default:
        return false;
    }
}