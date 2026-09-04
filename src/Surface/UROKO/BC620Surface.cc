#include "Surface/UROKO/BC620Surface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"


BC620Surface::BC620Surface() {

    fSurface = new G4OpticalSurface("BC620Surface");
    fSurface->SetType(dielectric_dielectric);
    fSurface->SetModel(unified);
    fSurface->SetFinish(groundbackpainted); 
    fSurface->SetSigmaAlpha(5.0*deg); 

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    std::vector<G4double> ephoton = { 2.034 * eV, 4.136 * eV }; //298~610nm
    std::vector<G4double> rindex  = { 2.1,  2.1  };
     std::vector<G4double> ref     = { 0.95, 0.95 }; //厳密には波長依存性あり
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

BC620Surface::~BC620Surface() {}