#include "Material/VacuumMat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

VacuumMat::VacuumMat() {
    G4NistManager* nist = G4NistManager::Instance();
    
    G4Material* baseMat = nist->FindOrBuildMaterial("G4_Galactic");
    fMaterial = new G4Material("Vacuum", baseMat->GetDensity(), baseMat);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    auto AddPropertyFromNm = [&](const char* key, const std::vector<std::pair<double, double>>& data) {
        std::vector<G4double> energies;
        std::vector<G4double> values;
        for (auto it = data.rbegin(); it != data.rend(); ++it) {
            energies.push_back((1239.84193 / it->first) * eV);
            values.push_back(it->second);
        }
        mpt->AddProperty(key, energies, values, false, true);
    };

    //全波長で屈折率 1.00
    AddPropertyFromNm("RINDEX", {
        { 360.0, 1.00 },
        { 520.0, 1.00 }
    });

    fMaterial->SetMaterialPropertiesTable(mpt);
}

VacuumMat::~VacuumMat() {
}