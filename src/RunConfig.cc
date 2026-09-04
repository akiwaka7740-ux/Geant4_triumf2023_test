#include "RunConfig.hh"
#include "G4SystemOfUnits.hh"

RunConfig::RunConfig()
{
    SetSourceType("neutron");
}

void RunConfig::SetSourceType(const G4String& sourceType)
{
    fSourceType = sourceType;
    fParticlesPerEvent = 1;
    fPosition = G4ThreeVector(0., 0., 0.);

    
    if (sourceType == "neutron") {
        fParticleName = "neutron";
        fEnergy = 0.5 * MeV;
        fEnergyUnitName = "MeV";
        fDirectionMode = DirectionMode::RandomIsotropic;
    }
    
    /*
    if (sourceType == "neutron") {
        fParticleName = "neutron";
        fEnergy = 1.00 * MeV;
        fEnergyUnitName = "MeV";
        fFixedDirection = G4ThreeVector(1., 0., 0.);
        fDirectionMode = DirectionMode::Fixed;    
    }
    */

    else if (sourceType == "electron") {
        fParticleName = "e-";
        fEnergy = 50.0 * keV;
        fEnergyUnitName = "keV";
        fDirectionMode = DirectionMode::RandomIsotropic;
    }

    else if (sourceType == "gamma") {
        fParticleName = "gamma";
        fEnergy =  2.3 * MeV;
        fEnergyUnitName = "MeV";
        //fFixedDirection = G4ThreeVector(1., 0., 0.);
        fDirectionMode = DirectionMode::RandomIsotropic;
    }
    else if (sourceType == "gamma(137Cs)") {
        fParticleName = "gamma";
        fEnergy = 0.661660 * MeV;
        fEnergyUnitName = "MeV";
        fDirectionMode = DirectionMode::RandomIsotropic;
    }
    else if (sourceType == "137Cs") {
        fParticleName = "ion:Cs137";
        fEnergy = 0. * eV;
        fEnergyUnitName = "eV";
        fDirectionMode = DirectionMode::AtRest;
    }
    else if (sourceType == "90Sr") {
        fParticleName = "ion:Sr90";
        fEnergy = 0. * eV;
        fEnergyUnitName = "eV";
        fDirectionMode = DirectionMode::AtRest;
    }
    else if (sourceType == "90Y") {
        fParticleName = "ion:Y90";
        fEnergy = 0. * eV;
        fEnergyUnitName = "eV";
        fDirectionMode = DirectionMode::AtRest;
    }
}

G4String RunConfig::GetDirectionModeName() const
{
    if (fDirectionMode == DirectionMode::RandomIsotropic) {
        return "random_isotropic";
    }
    if (fDirectionMode == DirectionMode::Fixed) {
        return "fixed";
    }
    return "at_rest";
}

