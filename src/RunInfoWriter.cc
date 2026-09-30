#include "RunInfoWriter.hh"

#include "TFile.h"
#include "TTree.h"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

const char* SelectionModeName(EventSelectionMode mode)
{
    switch (mode) {
    case EventSelectionMode::All:
        return "all";
    case EventSelectionMode::ScintiEdep:
        return "edep";
    case EventSelectionMode::NeutronCapture:
        return "capture";
    }

    throw std::runtime_error(
        "Unknown event selection mode."
    );
}

const char* StatusName(RunConditionsWriter::Status status)
{
    using Status = RunConditionsWriter::Status;

    switch (status) {
    case Status::Completed:
        return "completed";
    case Status::Aborted:
        return "aborted";
    default:
        throw std::runtime_error(
            "RunInfo requires completed or aborted status."
        );
    }
}

} // namespace

namespace RunInfoWriter {

void Write(
    const G4String& rootFilePath,
    const RunInfo& info
)
{
    if (
        info.runId < 0 ||
        info.requestedEvents < 0 ||
        info.processedEvents < 0 ||
        info.selectedEvents < 0 ||
        info.selectedEvents > info.processedEvents ||
        info.processedEvents > info.requestedEvents
    ) {
        throw std::runtime_error(
            "Invalid Run event counts."
        );
    }

    std::string selectionMode =
        SelectionModeName(info.selectionMode);
    std::string status =
        StatusName(info.status);

    // 採用イベントがあるのに ROOT ファイルがなければ異常。
    // 採用0件の場合は run_info だけのファイルを作れる。
    if (
        info.selectedEvents > 0 &&
        !std::filesystem::exists(
            std::filesystem::path{rootFilePath.c_str()}
        )
    ) {
        throw std::runtime_error(
            "Event ROOT file is missing."
        );
    }

    TFile file(rootFilePath.c_str(), "UPDATE");

    if (!file.IsOpen() || file.IsZombie()) {
        throw std::runtime_error(
            "Cannot open ROOT file for run_info."
        );
    }

    if (file.Get("run_info") != nullptr) {
        throw std::runtime_error(
            "run_info already exists in ROOT file."
        );
    }

    if (!file.cd()) {
        throw std::runtime_error(
            "Cannot select ROOT file directory."
        );
    }

    Int_t schemaVersion = 1;
    Int_t runId = info.runId;

    Long64_t requestedEvents =
        static_cast<Long64_t>(info.requestedEvents);
    Long64_t processedEvents =
        static_cast<Long64_t>(info.processedEvents);
    Long64_t selectedEvents =
        static_cast<Long64_t>(info.selectedEvents);

    TTree tree("run_info", "Run-level metadata");

    if (
        tree.Branch(
            "SchemaVersion",
            &schemaVersion,
            "SchemaVersion/I"
        ) == nullptr ||
        tree.Branch(
            "RunID",
            &runId,
            "RunID/I"
        ) == nullptr ||
        tree.Branch(
            "RequestedEvents",
            &requestedEvents,
            "RequestedEvents/L"
        ) == nullptr ||
        tree.Branch(
            "ProcessedEvents",
            &processedEvents,
            "ProcessedEvents/L"
        ) == nullptr ||
        tree.Branch(
            "SelectedEvents",
            &selectedEvents,
            "SelectedEvents/L"
        ) == nullptr ||
        tree.Branch(
            "EventSelectionMode",
            &selectionMode
        ) == nullptr ||
        tree.Branch(
            "Status",
            &status
        ) == nullptr
    ) {
        throw std::runtime_error(
            "Cannot create run_info branches."
        );
    }

    if (tree.Fill() <= 0) {
        throw std::runtime_error(
            "Cannot fill run_info."
        );
    }

    if (tree.Write() <= 0) {
        throw std::runtime_error(
            "Cannot write run_info."
        );
    }

    file.Flush();

    if (file.TestBit(TFile::kWriteError)) {
        throw std::runtime_error(
            "Cannot flush run_info ROOT file."
        );
    }

    file.Close();

    if (file.TestBit(TFile::kWriteError)) {
        throw std::runtime_error(
            "Cannot close run_info ROOT file."
        );
    }
}

} // namespace RunInfoWriter