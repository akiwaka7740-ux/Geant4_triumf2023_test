#ifndef RUN_INFO_WRITER_HH
#define RUN_INFO_WRITER_HH

#include "AnalysisConfig.hh"
#include "RunConditionsWriter.hh"

#include "globals.hh"

struct RunInfo {
    G4int runId = -1;
    G4long requestedEvents = 0;
    G4long processedEvents = 0;
    G4long selectedEvents = 0;
    EventSelectionMode selectionMode =
        EventSelectionMode::All;
    RunConditionsWriter::Status status =
        RunConditionsWriter::Status::Running;
};

namespace RunInfoWriter {

void Write(
    const G4String& rootFilePath,
    const RunInfo& info
);

}

#endif