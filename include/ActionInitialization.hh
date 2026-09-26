#ifndef ACTIONINITIALIZATION_HH
#define ACTIONINITIALIZATION_HH

#include <memory>

#include "G4VUserActionInitialization.hh"

class AnalysisConfig;
struct OutputConfig;

class ActionInitialization : public G4VUserActionInitialization
{
public:
    explicit ActionInitialization(
        std::shared_ptr<const AnalysisConfig> analysisConfig,
        std::shared_ptr<const OutputConfig> outputConfig
    );

    ~ActionInitialization() override;

    void BuildForMaster() const override;
    void Build() const override;

private:
    std::shared_ptr<const AnalysisConfig> fAnalysisConfig;
    std::shared_ptr<const OutputConfig> fOutputConfig;
};

#endif