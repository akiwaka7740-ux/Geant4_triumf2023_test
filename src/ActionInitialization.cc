#include "ActionInitialization.hh"
#include "EventAction.hh"

ActionInitialization::ActionInitialization()
{}

ActionInitialization::~ActionInitialization()
{}

void ActionInitialization::BuildForMaster () const
{
    EventAction* masterEventAction = new EventAction();
    RunAction *runAction = new RunAction(masterEventAction);
    SetUserAction(runAction);

}


void ActionInitialization::Build () const
{

    EventAction* eventAction = new EventAction();
    SetUserAction(eventAction);
    
    PrimaryGenerator *generator = new PrimaryGenerator();
    SetUserAction(generator);

    RunAction *runAction = new RunAction(eventAction);
    SetUserAction(runAction);

}
