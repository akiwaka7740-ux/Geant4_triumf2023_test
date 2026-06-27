#include "Surface/UROKO/DiffuseSurface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"

DiffuseSurface::DiffuseSurface() {
    fSurface = new G4OpticalSurface("DiffuseSurface");
    fSurface->SetType(dielectric_dielectric);
    fSurface->SetModel(unified);
    fSurface->SetFinish(polishedbackpainted);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    std::vector<G4double> ephoton = { 2.034 * eV, 4.136 * eV };  //298~610nm
    std::vector<G4double> rindex  = { 1.0,  1.0  };
    std::vector<G4double> ref     = { 0.96, 0.96 }; // ランバート反射率96%（全方向へ均一に散乱）
    std::vector<G4double> spike   = { 0.0,  0.0  };
    std::vector<G4double> lobe    = { 0.0,  0.0  };
    std::vector<G4double> back    = { 0.0,  0.0  };

    mpt->AddProperty("RINDEX", ephoton, rindex, false, true);
    mpt->AddProperty("REFLECTIVITY", ephoton, ref, false, true);
    mpt->AddProperty("SPECULARSPIKECONSTANT", ephoton, spike, false, true);
    mpt->AddProperty("SPECULARLOBECONSTANT", ephoton, lobe, false, true);
    mpt->AddProperty("BACKSCATTERCONSTANT", ephoton, back, false, true);

    fSurface->SetMaterialPropertiesTable(mpt);
}

DiffuseSurface::~DiffuseSurface() {}