#include "PrimaryGenerator.hh"
#include "PrimaryGeneratorMessenger.hh" 
#include "RunConfig.hh"


PrimaryGenerator::PrimaryGenerator(RunConfig* runConfig)
    : fRunConfig(runConfig)
{
    fParticleGun = new G4ParticleGun(1);
    fMessenger = new PrimaryGeneratorMessenger(this);

    fParticleGun->SetParticlePosition(fRunConfig->GetPosition());
}

void PrimaryGenerator::SetSourceType(G4String type)
{
    fRunConfig->SetSourceType(type);
}

PrimaryGenerator::~PrimaryGenerator()
{
    delete fParticleGun;
    delete fMessenger; 
}

void PrimaryGenerator::GeneratePrimaries(G4Event* anEvent)
{
    const auto& sourceType = fRunConfig->GetSourceType();

    if (sourceType == "137Cs") {
        G4IonTable* ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition* ion = ionTable->GetIon(55, 137, 0.0);
        fParticleGun->SetParticleDefinition(ion);
    }
    else if (sourceType == "90Sr") {
        G4IonTable* ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition* ion = ionTable->GetIon(38, 90, 0.0);
        fParticleGun->SetParticleDefinition(ion);
    }
    else if (sourceType == "90Y") {
        G4IonTable* ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition* ion = ionTable->GetIon(39, 90, 0.0);
        fParticleGun->SetParticleDefinition(ion);
    }
    else {
        G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition* particle =
            particleTable->FindParticle(fRunConfig->GetParticleName());
        fParticleGun->SetParticleDefinition(particle);
    }

    fParticleGun->SetParticleEnergy(fRunConfig->GetEnergy());
    fParticleGun->SetParticlePosition(fRunConfig->GetPosition());

    if (fRunConfig->GetDirectionMode() == RunConfig::DirectionMode::RandomIsotropic) {
        fParticleGun->SetParticleMomentumDirection(G4RandomDirection());
    }
    else if (fRunConfig->GetDirectionMode() == RunConfig::DirectionMode::Fixed) {
        fParticleGun->SetParticleMomentumDirection(fRunConfig->GetFixedDirection());
    }
    else {
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 0.));
    }

    fParticleGun->GeneratePrimaryVertex(anEvent);
}