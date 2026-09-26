#ifndef RUNCONDITIONSWRITER_HH
#define RUNCONDITIONSWRITER_HH

#include "RunConditions.hh"

struct OutputConfig;

namespace RunConditionsWriter
{

enum class Status {
    Running,
    Completed,
    Aborted,
    Failed
};

void Write(
    const OutputConfig& outputConfig,
    const RunConditions& conditions,
    const G4String& rootFilePath,
    G4int requestedEvents,
    G4int processedEvents,
    Status status
);

}

#endif