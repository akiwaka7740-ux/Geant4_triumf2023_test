#include "Surface/UROKO/DielectricSurface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"

DielectricSurface::DielectricSurface() {
    fSurface = new G4OpticalSurface("DielectricSurface");
    fSurface->SetType(dielectric_dielectric);
    fSurface->SetModel(unified);
    fSurface->SetFinish(polished); // グリスで密着したツルツル面

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    std::vector<G4double> ephoton = { 2.034 * eV, 4.136 * eV }; //298~610nm
    std::vector<G4double> ref     = { 0.999, 0.999 }; // 鏡面反射率99.9%
    std::vector<G4double> spike   = { 1.0,   1.0   };

    mpt->AddProperty("REFLECTIVITY", ephoton, ref, false, true);
    mpt->AddProperty("SPECULARSPIKECONSTANT", ephoton, spike, false, true);
    //mpt->AddConstProperty("SIGMA_ALPHA", 0.0); //errorが出た

    fSurface->SetMaterialPropertiesTable(mpt);
}

DielectricSurface::~DielectricSurface() {}