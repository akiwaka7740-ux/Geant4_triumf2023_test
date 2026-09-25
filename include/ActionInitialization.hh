#ifndef ACTIONINITIALIZATION_HH
#define ACTIONINITIALIZATION_HH

#include <memory>

#include "G4VUserActionInitialization.hh"

#include "PrimaryGenerator.hh"
#include "RunAction.hh"
#include "RunConfig.hh"

class AnalysisConfig;

class ActionInitialization : public G4VUserActionInitialization
{
    public:
        explicit ActionInitialization(std::shared_ptr<const AnalysisConfig> analysisConfig);
        ~ActionInitialization() override;

        void BuildForMaster() const override;
        void Build() const override;

    private:
        RunConfig *fRunConfig = nullptr;
        std::shared_ptr<const AnalysisConfig> fAnalysisConfig;

};

#endif