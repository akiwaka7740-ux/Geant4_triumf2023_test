#include "AnalysisMessenger.hh"

#include "AnalysisConfig.hh"
#include "GeometryObjectType.hh"

#include "G4Exception.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"

#include <utility>


AnalysisMessenger::AnalysisMessenger(
    std::shared_ptr<AnalysisConfig> config
)
    : fConfig(std::move(config))
{
    if (fConfig == nullptr) {
        G4Exception(
            "AnalysisMessenger::AnalysisMessenger",
            "AnalysisMessenger001",
            FatalException,
            "AnalysisConfig is null."
        );
    }

    fDirectory =
        new G4UIdirectory("/analysis/");

    fDirectory->SetGuidance(
        "Analysis output control commands."
    );


    // =========================================================
    // 光学光子記録モード
    // =========================================================

    fOpticalRecordingCommand =
        new G4UIcmdWithAString(
            "/analysis/opticalRecording",
            this
        );

    fOpticalRecordingCommand->SetGuidance(
        "Select optical photon recording mode."
    );

    fOpticalRecordingCommand->SetGuidance(
        "off: disable optical analysis recording."
    );

    fOpticalRecordingCommand->SetGuidance(
        "summary: record only photon counts "
        "and efficiencies."
    );

    fOpticalRecordingCommand->SetGuidance(
        "detailed: record per-photon data "
        "and boundary histories."
    );

    fOpticalRecordingCommand->SetParameterName(
        "mode",
        false
    );

    fOpticalRecordingCommand->SetCandidates(
        "off summary detailed"
    );

    fOpticalRecordingCommand->AvailableForStates(
        G4State_PreInit,
        G4State_Idle
    );


    // =========================================================
    // 中性子散乱履歴の対象検出器
    // =========================================================

    fNeutronHistoryTargetCommand =
        new G4UIcmdWithAString(
            "/analysis/neutronHistoryTarget",
            this
        );

    fNeutronHistoryTargetCommand->SetGuidance(
        "Select the detector that terminates "
        "the pre-detector neutron scatter history."
    );

    fNeutronHistoryTargetCommand->SetGuidance(
        "LiGlass: record scatter history until "
        "the neutron enters LiGlass."
    );

    fNeutronHistoryTargetCommand->SetGuidance(
        "UROKO: record scatter history until "
        "the neutron enters UROKO."
    );

    fNeutronHistoryTargetCommand->SetParameterName(
        "target",
        false
    );

    fNeutronHistoryTargetCommand->SetCandidates(
        "LiGlass UROKO"
    );

    fNeutronHistoryTargetCommand->AvailableForStates(
        G4State_PreInit,
        G4State_Idle
    );
}


AnalysisMessenger::~AnalysisMessenger()
{
    delete fNeutronHistoryTargetCommand;
    delete fOpticalRecordingCommand;
    delete fDirectory;
}


void AnalysisMessenger::SetNewValue(
    G4UIcommand* command,
    G4String newValue
)
{
    /*
     * このMessengerが管理していないコマンドなら、
     * 何も処理しない。
     */
    if (command != fOpticalRecordingCommand &&
        command != fNeutronHistoryTargetCommand) {
        return;
    }

    if (fConfig == nullptr) {
        G4Exception(
            "AnalysisMessenger::SetNewValue",
            "AnalysisMessenger002",
            FatalException,
            "AnalysisConfig is null."
        );

        return;
    }


    // =========================================================
    // 光学光子記録モード
    // =========================================================

    if (command == fOpticalRecordingCommand) {
        if (newValue == "off") {
            fConfig->SetOpticalRecordingMode(
                OpticalRecordingMode::Off
            );

            return;
        }

        if (newValue == "summary") {
            fConfig->SetOpticalRecordingMode(
                OpticalRecordingMode::Summary
            );

            return;
        }

        if (newValue == "detailed") {
            fConfig->SetOpticalRecordingMode(
                OpticalRecordingMode::Detailed
            );

            return;
        }

        G4ExceptionDescription description;

        description
            << "Unknown optical recording mode: "
            << newValue
            << ". Expected off, summary, or detailed.";

        G4Exception(
            "AnalysisMessenger::SetNewValue",
            "AnalysisMessenger003",
            JustWarning,
            description
        );

        return;
    }


    // =========================================================
    // 中性子散乱履歴の対象検出器
    // =========================================================

    if (command == fNeutronHistoryTargetCommand) {
        if (newValue == "LiGlass") {
            fConfig->SetNeutronHistoryTarget(
                GeometryObjectType::LiGlass
            );

            return;
        }

        if (newValue == "UROKO") {
            fConfig->SetNeutronHistoryTarget(
                GeometryObjectType::UROKO
            );

            return;
        }

        G4ExceptionDescription description;

        description
            << "Unknown neutron history target: "
            << newValue
            << ". Expected LiGlass or UROKO.";

        G4Exception(
            "AnalysisMessenger::SetNewValue",
            "AnalysisMessenger004",
            JustWarning,
            description
        );

        return;
    }
}