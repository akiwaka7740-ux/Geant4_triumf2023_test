#ifndef OPTICAL_PHOTON_TRACK_INFO_HH
#define OPTICAL_PHOTON_TRACK_INFO_HH


#include "G4VUserTrackInformation.hh"
#include "globals.hh"

enum class OpticalPhotonRegion {
    Other,
    Scintillator,
    LightGuide
};

class OpticalPhotonTrackInfo : public G4VUserTrackInformation {
public:
    OpticalPhotonTrackInfo() = default;
    ~OpticalPhotonTrackInfo() override = default;

    void RecordBoundaryInteraction(OpticalPhotonRegion region, G4bool wasReflected);

    G4int GetScintillatorBoundaryCount() const{
        return fScintillatorBoundaryCount;
    }

    G4int GetLightGuideBoundaryCount() const{
        return fLightGuideBoundaryCount;
    }

    
    G4int GetScintillatorReflectionCount() const {
        return fScintillatorReflectionCount;
    }

    G4int GetLightGuideReflectionCount() const {
        return fLightGuideReflectionCount;
    }

    G4int GetTotalReflectionCount() const {
        return fScintillatorReflectionCount
             + fLightGuideReflectionCount;
    }

    void Print() const override;

private:
    G4int fScintillatorBoundaryCount = 0;
    G4int fLightGuideBoundaryCount = 0;

    G4int fScintillatorReflectionCount = 0;
    G4int fLightGuideReflectionCount = 0;
};

#endif // OPTICAL_PHOTON_TRACK_INFO_HH