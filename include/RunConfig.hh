#ifndef RUNCONFIG_HH
#define RUNCONFIG_HH

#include "globals.hh"
#include "G4ThreeVector.hh"

class RunConfig {
public:
    enum class DirectionMode {
        RandomIsotropic,
        Fixed,
        AtRest
    };

    RunConfig();

    void SetSourceType(const G4String& sourceType);

    const G4String& GetSourceType() const { return fSourceType; }
    const G4String& GetParticleName() const { return fParticleName; }
    G4double GetEnergy() const { return fEnergy; }
    const G4String& GetEnergyUnitName() const { return fEnergyUnitName; }
    G4int GetParticlesPerEvent() const { return fParticlesPerEvent; }
    const G4ThreeVector& GetPosition() const { return fPosition; }
    const G4ThreeVector& GetFixedDirection() const { return fFixedDirection; }
    DirectionMode GetDirectionMode() const { return fDirectionMode; }

    G4String GetDirectionModeName() const;

private:
    G4String fSourceType = "neutron";
    G4String fParticleName = "neutron";
    G4double fEnergy = 1.00;
    G4String fEnergyUnitName = "MeV";
    G4int fParticlesPerEvent = 1;
    G4ThreeVector fPosition = G4ThreeVector(0., 0., 0.);
    G4ThreeVector fFixedDirection = G4ThreeVector(1., 0., 0.);
    DirectionMode fDirectionMode = DirectionMode::RandomIsotropic;
};

#endif