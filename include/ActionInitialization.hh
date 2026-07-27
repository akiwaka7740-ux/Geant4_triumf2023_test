#ifndef ACTIONINITIALIZATION_HH
#define ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

#include "PrimaryGenerator.hh"
#include "RunAction.hh"
#include "RunConfig.hh"

class ActionInitialization : public G4VUserActionInitialization
{
    public:
        ActionInitialization();
        ~ActionInitialization();

        virtual void BuildForMaster() const;
        virtual void Build() const;

    private:
        RunConfig *fRunConfig = nullptr;

};

#endif