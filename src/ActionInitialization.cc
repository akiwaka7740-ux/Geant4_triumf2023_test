#include "ActionInitialization.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"


ActionInitialization::ActionInitialization()
{
    fRunConfig = new RunConfig();
}

ActionInitialization::~ActionInitialization()
{
    delete fRunConfig;
}

void ActionInitialization::BuildForMaster () const
{
    EventAction* masterEventAction = new EventAction();
    RunAction *runAction = new RunAction(masterEventAction,fRunConfig);
    SetUserAction(runAction);

}


void ActionInitialization::Build () const
{   
    EventAction* eventAction = new EventAction();
    SetUserAction(eventAction);

    PrimaryGenerator *generator = new PrimaryGenerator(fRunConfig);
    SetUserAction(generator);

    RunAction *runAction = new RunAction(eventAction, fRunConfig);
    SetUserAction(runAction);

    SteppingAction *steppingAction = new SteppingAction();
    SetUserAction(steppingAction);

}
