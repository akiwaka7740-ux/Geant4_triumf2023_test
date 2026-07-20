#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"

class SteppingAction : public G4UserSteppingAction {
public:
    SteppingAction();
    ~SteppingAction() override = default;

    // ステップが発生するたびにGeant4カーネルから自動で呼ばれる関数
    void UserSteppingAction(const G4Step* step) override;
};

#endif