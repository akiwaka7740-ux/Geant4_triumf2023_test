#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

#include "EventAction.hh"

class EventAction;

class RunAction : public G4UserRunAction{
    public:
        RunAction(EventAction* eventAction);
        ~RunAction();

        virtual void BeginOfRunAction(const G4Run *);
        virtual void EndOfRunAction(const G4Run *);

    private:
        EventAction* fEventAction;
};

#endif