#ifndef ANALYSIS_MESSENGER_HH
#define ANALYSIS_MESSENGER_HH

#include "G4UImessenger.hh"

#include <memory>

class AnalysisConfig;
class G4UIcommand;
class G4UIdirectory;
class G4UIcmdWithAString;

class AnalysisMessenger
    : public G4UImessenger {
public:
    explicit AnalysisMessenger(
        std::shared_ptr<AnalysisConfig> config
    );

    ~AnalysisMessenger() override;

    void SetNewValue(
        G4UIcommand* command,
        G4String newValue
    ) override;

private:
    std::shared_ptr<AnalysisConfig> fConfig;

    G4UIdirectory* fDirectory = nullptr;

    //光学光子の記録モード
    G4UIcmdWithAString*
        fOpticalRecordingCommand = nullptr;

    G4UIcmdWithAString*
        fNeutronHistoryTargetCommand = nullptr;
};

#endif