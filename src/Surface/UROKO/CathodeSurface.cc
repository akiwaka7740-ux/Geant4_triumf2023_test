#include "Surface/UROKO/CathodeSurface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"

CathodeSurface::CathodeSurface() {
    fSurface = new G4OpticalSurface("CathodeSurface");
    fSurface->SetType(dielectric_metal); // ガラスから金属への吸収面
    fSurface->SetModel(glisur);
    fSurface->SetFinish(polished);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    std::vector<G4double> ephoton = { 2.034 * eV, 4.136 * eV };
    std::vector<G4double> ref     = { 0.0, 0.0 }; // 反射ゼロ（全吸収）
    std::vector<G4double> eff     = { 1.0, 1.0 }; // 量子効率100%（全検知）<=理想的であると仮定

    mpt->AddProperty("REFLECTIVITY", ephoton, ref, false, true);
    mpt->AddProperty("EFFICIENCY", ephoton, eff, false, true);

    fSurface->SetMaterialPropertiesTable(mpt);
}

CathodeSurface::~CathodeSurface() {}