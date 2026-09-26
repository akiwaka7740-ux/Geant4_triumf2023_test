#include "PrimaryGenerator.hh"

#include "G4GeneralParticleSource.hh"

PrimaryGenerator::PrimaryGenerator()
    : fParticleSource(new G4GeneralParticleSource())
{
}

PrimaryGenerator::~PrimaryGenerator()
{
    delete fParticleSource;
}

void PrimaryGenerator::GeneratePrimaries(G4Event* event)
{
    fParticleSource->GeneratePrimaryVertex(event);
}