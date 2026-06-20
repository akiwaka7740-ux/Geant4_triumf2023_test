#ifndef PRIMARYGENARATORMESSANGER_HH
#define PRIMARYGENARATORMESSANGER_HH

#include "G4UImessenger.hh"
#include "G4String.hh"

class PrimaryGenerator;
class G4UIdirectory;
class G4UIcmdWithAString;

class PrimaryGeneratorMessenger : public G4UImessenger
{
    public:
        PrimaryGeneratorMessenger(PrimaryGenerator *);
        ~PrimaryGeneratorMessenger();

        virtual void SetNewValue(G4UIcommand *, G4String);

    private:
        PrimaryGenerator *fAction;
        G4UIdirectory *fDir;
        G4UIcmdWithAString *fSourceCmd;
};

#endif