#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <vector>

class AnalysisOutput;

class EventAction : public G4UserEventAction {
public:
    EventAction() = default;
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void SetAnalysisOutput(AnalysisOutput* analysisOutput) {
        fAnalysisOutput = analysisOutput;
    }

private:
    AnalysisOutput* fAnalysisOutput = nullptr;
  
};
#endif