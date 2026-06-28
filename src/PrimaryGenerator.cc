#include "PrimaryGenerator.hh"
#include "PrimaryGeneratorMessenger.hh" // 【修正】インクルードを追加

PrimaryGenerator::PrimaryGenerator()
{
    fParticleGun = new G4ParticleGun(1);
    fMessenger = new PrimaryGeneratorMessenger(this);

    fSourceType = "neutron"; // デフォルトのソースタイプを設定

    //位置は共通で原点とする
    fParticleGun->SetParticlePosition(G4ThreeVector(0.*m, 0.*m, 0.*m));
}

PrimaryGenerator::~PrimaryGenerator()
{
    delete fParticleGun;
    delete fMessenger; 
}

void PrimaryGenerator::GeneratePrimaries(G4Event *anEvent)
{
    /*
    if (fSourceType == "neutron") {
        G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition *particle = particleTable->FindParticle("neutron");
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(0.500 * MeV);
        fParticleGun->SetParticleMomentumDirection(G4RandomDirection());
    }
    */

    if (fSourceType == "neutron") {
        G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition *particle = particleTable->FindParticle("neutron");
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(0.500 * MeV);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(1., 0., 0.));
    }

    else if (fSourceType == "gamma") {
        G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition *particle = particleTable->FindParticle("gamma");
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(1 * MeV);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(1., 0., 0.));
    }

    else if (fSourceType == "gamma(137Cs)") {
        G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition *particle = particleTable->FindParticle("gamma");
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(0.661660 * MeV);
        fParticleGun->SetParticleMomentumDirection(G4RandomDirection());
    }
    else if (fSourceType == "137Cs") {
        G4IonTable *ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition *ion = ionTable->GetIon(55, 137, 0.0);
        fParticleGun->SetParticleDefinition(ion);
        fParticleGun->SetParticleEnergy(0.* eV);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 0.));
    }
    else if (fSourceType == "90Sr") {
        G4IonTable *ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition *ion = ionTable->GetIon(38, 90, 0.0);
        fParticleGun->SetParticleDefinition(ion);
        fParticleGun->SetParticleEnergy(0.* eV);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 0.));
    }
    else if (fSourceType == "90Y") {
        G4IonTable *ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition *ion = ionTable->GetIon(39, 90, 0.0);
        fParticleGun->SetParticleDefinition(ion);
        fParticleGun->SetParticleEnergy(0.* eV);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 0.));
    }
    else {
        G4cerr << "Error: Unknown source type: " << fSourceType << G4endl;
        return;
    }

    fParticleGun->GeneratePrimaryVertex(anEvent);
}