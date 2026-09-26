#include "RunAction.hh"

#include "EventAction.hh"
#include "OutputConfig.hh"
#include "SourceConditionsReader.hh"

#include "G4AnalysisManager.hh"
#include "G4Exception.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"
#include "RunConditionsWriter.hh"

#include <atomic>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>


namespace
{

// worker の ROOT 出力失敗を master に伝える。
// master が Run 開始時にリセットする。
std::atomic<bool> gRunOutputFailed{false};

[[noreturn]] void FailRunOutput(
    const char* code,
    const G4String& message
)
{
    G4Exception(
        "RunAction",
        code,
        FatalException,
        message.c_str()
    );

    throw std::runtime_error(message.c_str());
}

G4String FormatEnergyForFileName(G4double energy)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());

    stream
        << std::setprecision(12)
        << std::defaultfloat
        << energy / MeV;

    return stream.str() + "MeV";
}

G4String GetOutputRootFileName(
    const OutputConfig& config,
    const RunConditions& conditions
)
{
    std::ostringstream fileName;
    fileName.imbue(std::locale::classic());

    switch (config.namingMode) {
    case OutputNamingMode::Timestamp:
        fileName << config.executionId;
        break;

    case OutputNamingMode::Source:
        // 現在の Reader は単色エネルギーだけを受け付ける
        fileName
            << conditions.particleName
            << "_mono_"
            << FormatEnergyForFileName(
                conditions.monoEnergy
            );
        break;

    default:
        G4Exception(
            "GetOutputRootFileName",
            "RunAction002",
            FatalException,
            "Unknown output naming mode."
        );

        throw std::runtime_error(
            "Unknown output naming mode."
        );
    }

    // 同じエネルギーを繰り返しても Run ID で区別する
    fileName
        << "_run"
        << std::setfill('0')
        << std::setw(3)
        << conditions.runId
        << ".root";

    const auto outputPath =
        std::filesystem::path(
            std::string(config.executionDirectory)
        )
        / "root"
        / fileName.str();

    return outputPath.string();
}

}

RunAction::RunAction(
    EventAction* eventAction,
    std::shared_ptr<const OutputConfig> outputConfig
)
    : fOutputConfig(std::move(outputConfig))
{
    if (fOutputConfig == nullptr) {
        G4Exception(
            "RunAction::RunAction",
            "RunAction001",
            FatalException,
            "OutputConfig is null."
        );
    }

    // master と各 worker に同じ ntuple 構成を定義する
    fAnalysisOutput.Book();

    // worker の EventAction に解析出力を接続する
    if (eventAction != nullptr) {
        eventAction->SetAnalysisOutput(
            &fAnalysisOutput
        );
    }
}

void RunAction::BeginOfRunAction(const G4Run* run)
{
    fRunConditions.reset();
    fOutputRootFileName.clear();

    if (run == nullptr) {
        FailRunOutput(
            "RunAction004",
            "Run is null at BeginOfRunAction."
        );
    }

    if (IsMaster()) {
        gRunOutputFailed.store(false);
    }

    // Run の線源条件を取得
    fRunConditions =
        SourceConditionsReader::Read(
            run->GetRunID()
        );

    // ROOT の出力先を確定
    fOutputRootFileName =
        GetOutputRootFileName(
            *fOutputConfig,
            *fRunConditions
        );

    // ROOT を開く前に開始時の記録を残す
    if (IsMaster()) {
        RunConditionsWriter::Write(
            *fOutputConfig,
            *fRunConditions,
            fOutputRootFileName,
            run->GetNumberOfEventToBeProcessed(),
            0,
            RunConditionsWriter::Status::Running
        );
    }

    auto* analysisManager =
        G4AnalysisManager::Instance();

    if (!analysisManager->OpenFile(
            fOutputRootFileName
        )) {
        gRunOutputFailed.store(true);

        if (IsMaster()) {
            RunConditionsWriter::Write(
                *fOutputConfig,
                *fRunConditions,
                fOutputRootFileName,
                run->GetNumberOfEventToBeProcessed(),
                0,
                RunConditionsWriter::Status::Failed
            );
        }

        FailRunOutput(
            "RunAction003",
            G4String("Cannot open ROOT output file: ")
                + fOutputRootFileName
        );
    }

    if (IsMaster()) {
        G4cout
            << "ROOT output: "
            << fOutputRootFileName
            << G4endl;
    }
}




void RunAction::EndOfRunAction(const G4Run* run)
{
    if (
        run == nullptr ||
        !fRunConditions.has_value() ||
        fOutputRootFileName.empty()
    ) {
        FailRunOutput(
            "RunAction005",
            "Run output information is missing."
        );
    }

    if (fRunConditions->runId != run->GetRunID()) {
        FailRunOutput(
            "RunAction006",
            "Stored conditions do not match the current Run ID."
        );
    }

    auto* analysisManager =
        G4AnalysisManager::Instance();

    // Write が失敗しても CloseFile は呼ぶ
    const G4bool writeSucceeded =
        analysisManager->Write();

    const G4bool closeSucceeded =
        analysisManager->CloseFile();

    if (!writeSucceeded || !closeSucceeded) {
        gRunOutputFailed.store(true);
    }

    // JSON の更新は master のみ。
    // 逐次実行でも IsMaster() は true となる。
    if (!IsMaster()) {
        return;
    }

    const G4int requestedEvents =
        run->GetNumberOfEventToBeProcessed();

    const G4int processedEvents =
        run->GetNumberOfEvent();

    using Status = RunConditionsWriter::Status;

    Status status = Status::Completed;

    if (
        gRunOutputFailed.load() ||
        processedEvents > requestedEvents
    ) {
        status = Status::Failed;
    }
    else if (processedEvents < requestedEvents) {
        status = Status::Aborted;
    }

    RunConditionsWriter::Write(
        *fOutputConfig,
        *fRunConditions,
        fOutputRootFileName,
        requestedEvents,
        processedEvents,
        status
    );

    if (status == Status::Failed) {
        FailRunOutput(
            "RunAction007",
            "ROOT output failed or the processed event count "
            "is inconsistent. See the run conditions JSON."
        );
    }

    G4cout
        << "Finishing run "
        << run->GetRunID()
        << ": processed "
        << processedEvents
        << " / "
        << requestedEvents
        << " events"
        << G4endl;
}