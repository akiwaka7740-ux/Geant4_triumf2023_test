#include "AnalysisConfig.hh"

void AnalysisConfig::SetOpticalRecordingMode(
    OpticalRecordingMode mode
)
{
    fOpticalRecordingMode = mode;
}

OpticalRecordingMode
AnalysisConfig::GetOpticalRecordingMode() const
{
    return fOpticalRecordingMode;
}

G4bool
AnalysisConfig::IsOpticalRecordingEnabled() const
{
    return fOpticalRecordingMode
        != OpticalRecordingMode::Off;
}

G4bool
AnalysisConfig::IsOpticalDetailEnabled() const
{
    return fOpticalRecordingMode
        == OpticalRecordingMode::Detailed;
}

void AnalysisConfig::SetNeutronHistoryTarget(
    GeometryObjectType target
)
{
    switch (target) {
    case GeometryObjectType::LiGlass:
    case GeometryObjectType::UROKO:
        fNeutronHistoryTarget = target;
        return;

    default:
        G4Exception(
            "AnalysisConfig::SetNeutronHistoryTarget",
            "AnalysisConfig001",
            FatalException,
            "Neutron history target must be "
            "GeometryObjectType::LiGlass or "
            "GeometryObjectType::UROKO."
        );
        return;
    }
}


GeometryObjectType
AnalysisConfig::GetNeutronHistoryTarget() const
{
    return fNeutronHistoryTarget;
}