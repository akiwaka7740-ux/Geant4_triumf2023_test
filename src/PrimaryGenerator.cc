#include "PrimaryGenerator.hh"

PrimaryGenerator::PrimaryGenerator()
{
    fParticleGun = new G4ParticleGun(1);

    // Particle position
    G4double x = 0. * m;
    G4double y = 0. * m;
    G4double z = 0. * m;

    G4ThreeVector pos(x,y,z);


    // Particle Type
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *particle = particleTable->FindParticle("neutron");

    fParticleGun -> SetParticlePosition(pos);
    fParticleGun -> SetParticleEnergy(0.500 * MeV);
    fParticleGun -> SetParticleDefinition(particle);

    
}

PrimaryGenerator::~PrimaryGenerator()
{
    delete fParticleGun;
}

void PrimaryGenerator::GeneratePrimaries(G4Event *anEvent)
{
    
    //球状分布
    G4ThreeVector randomMom = G4RandomDirection();
    
    //発射ごとに新しい方向にセット
    fParticleGun->SetParticleMomentumDirection(randomMom);
    

    /*
    G4ThreeVector mom(1.0,0., 0.);
    fParticleGun->SetParticleMomentumDirection(mom);
    */

    /*　確認用
    G4cout << "x: " << randomMom[0] << " y: " << randomMom[1] << " z: " << randomMom[2] << G4endl;
    */

    // Create vertex
    fParticleGun->GeneratePrimaryVertex(anEvent);
}