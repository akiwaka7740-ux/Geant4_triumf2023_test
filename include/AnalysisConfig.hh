#ifndef ANALYSIS_CONFIG_HH
#define ANALYSIS_CONFIG_HH

#include "GeometryObjectType.hh"

#include "globals.hh"

enum class OpticalRecordingMode : G4int {
    Off      = 0,
    Summary  = 1,
    Detailed = 2
};

class AnalysisConfig {
public:
    AnalysisConfig() = default;
    ~AnalysisConfig() = default;

    void SetOpticalRecordingMode(
        OpticalRecordingMode mode
    );

    OpticalRecordingMode
    GetOpticalRecordingMode() const;

    //OpticalRecordingModeを直接参照せずとも状況が把握できる
    G4bool IsOpticalRecordingEnabled() const;
    G4bool IsOpticalDetailEnabled() const;

    //中性子散乱履歴の終点となる検出器
    void SetNeutronHistoryTarget(GeometryObjectType target);

    GeometryObjectType GetNeutronHistoryTarget() const;



private:
    OpticalRecordingMode fOpticalRecordingMode =
        OpticalRecordingMode::Summary;
    
    //defaultはLiGlass
    GeometryObjectType fNeutronHistoryTarget =
        GeometryObjectType::LiGlass;
};

#endif
