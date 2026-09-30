#include "RunAction.hh"

#include "EventAction.hh"
#include "OutputConfig.hh"
#include "RunConditionsWriter.hh"
#include "RunInfoWriter.hh"
#include "SourceConditionsReader.hh"

#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Exception.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"

#include <atomic>
#include <exception>
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
    std::shared_ptr<const AnalysisConfig> analysisConfig,
    std::shared_ptr<const OutputConfig> outputConfig
)
    : fAnalysisConfig(std::move(analysisConfig)),
      fOutputConfig(std::move(outputConfig))
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "RunAction::RunAction",
            "RunAction008",
            FatalException,
            "AnalysisConfig is null."
        );
    }

    if (fOutputConfig == nullptr) {
        G4Exception(
            "RunAction::RunAction",
            "RunAction001",
            FatalException,
            "OutputConfig is null."
        );
    }

    G4AccumulableManager::Instance()
        ->RegisterAccumulable(fSelectedEvents);

    // master と各 worker に同じ ntuple 構成を定義する
    fAnalysisOutput.Book();

    // worker の EventAction に解析出力と RunAction を接続する
    if (eventAction != nullptr) {
        eventAction->SetAnalysisOutput(
            &fAnalysisOutput
        );
        eventAction->SetRunAction(this);
    }
}

void RunAction::CountSelectedEvent()
{
    fSelectedEvents += 1;
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

    G4AccumulableManager::Instance()->Reset();

    fRunEventSelectionMode =
        fAnalysisConfig->GetEventSelectionMode();

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
            0,
            fRunEventSelectionMode,
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
                0,
                fRunEventSelectionMode,
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

    // worker の値を master に合算する。
    // master と逐次実行では Merge() は何もしない。
    G4AccumulableManager::Instance()->Merge();

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

    // JSON と run_info の更新は master のみ。
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

    // Geant4 の ROOT ファイルを閉じた後で run_info を追記する。
    if (status != Status::Failed) {
        RunInfo info;
        info.runId = run->GetRunID();
        info.requestedEvents = requestedEvents;
        info.processedEvents = processedEvents;
        info.selectedEvents = fSelectedEvents.GetValue();
        info.selectionMode = fRunEventSelectionMode;
        info.status = status;

        try {
            RunInfoWriter::Write(
                fOutputRootFileName,
                info
            );
        }
        catch (const std::exception& error) {
            gRunOutputFailed.store(true);
            status = Status::Failed;

            G4cerr
                << "run_info output failed: "
                << error.what()
                << G4endl;
        }
    }

    // run_info の追記結果を反映した状態を JSON に保存する
    RunConditionsWriter::Write(
        *fOutputConfig,
        *fRunConditions,
        fOutputRootFileName,
        requestedEvents,
        processedEvents,
        fSelectedEvents.GetValue(),
        fRunEventSelectionMode,
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