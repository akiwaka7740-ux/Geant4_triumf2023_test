#ifndef PHYSICSLIST_HH
#define PHYSICSLIST_HH

#include "G4VModularPhysicsList.hh"
#include "FTFP_BERT_HP.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4EmLivermorePhysics.hh"
#include "G4OpticalPhysics.hh"


class PhysicsList : public FTFP_BERT_HP //継承
{
    public:
        PhysicsList();
        ~PhysicsList();

};

#endif