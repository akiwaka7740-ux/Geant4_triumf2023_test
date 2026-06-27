#include "Surface/UROKO/MirrorSurface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"

MirrorSurface::MirrorSurface() {
    fSurface = new G4OpticalSurface("MirrorSurface");
    fSurface->SetType(dielectric_dielectric);
    fSurface->SetModel(unified);
    fSurface->SetFinish(polishedbackpainted);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    std::vector<G4double> ephoton = { 2.034 * eV, 4.136 * eV }; //298~610nm
    std::vector<G4double> rindex  = { 1.0,  1.0  };
    std::vector<G4double> ref     = { 0.98, 0.98 }; // 鏡面反射率98%
    std::vector<G4double> spike   = { 1.0,  1.0  };

    mpt->AddProperty("RINDEX", ephoton, rindex, false, true);
    mpt->AddProperty("REFLECTIVITY", ephoton, ref, false, true);
    mpt->AddProperty("SPECULARSPIKECONSTANT", ephoton, spike, false, true);

    fSurface->SetMaterialPropertiesTable(mpt);
}

MirrorSurface::~MirrorSurface() {}