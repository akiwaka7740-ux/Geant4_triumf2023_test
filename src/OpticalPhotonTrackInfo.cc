#include "OpticalPhotonTrackInfo.hh"

#include "G4ios.hh"

void OpticalPhotonTrackInfo::RecordBoundaryInteraction(
    OpticalPhotonRegion region,
    G4bool wasReflection
)
{

    switch (region) {
        case OpticalPhotonRegion::Scintillator:
            ++fScintillatorBoundaryCount;
            if (wasReflection) {
                ++fScintillatorReflectionCount;
            }
            break;

        case OpticalPhotonRegion::LightGuide:
            ++fLightGuideBoundaryCount;
            if (wasReflection) {
                ++fLightGuideReflectionCount;
            }
            break;

        case OpticalPhotonRegion::Other:
            break;
    }
}


void OpticalPhotonTrackInfo::Print() const
{
    G4cout
        << "OpticalPhotonTrackInfo:"
        << " scintillator boundary="
        << fScintillatorBoundaryCount
        << ", scintillator reflection="
        << fScintillatorReflectionCount
        << ", light-guide boundary="
        << fLightGuideBoundaryCount
        << ", light-guide reflection="
        << fLightGuideReflectionCount
        << G4endl;
}