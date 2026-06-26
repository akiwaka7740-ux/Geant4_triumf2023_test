#include "Material/AcrylicMat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

AcrylicMat::AcrylicMat() {
    G4NistManager* nist = G4NistManager::Instance();
    
    G4Material* baseMat = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
    fMaterial = new G4Material("Acrylic", baseMat->GetDensity(), baseMat);

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

    //屈折率 (全波長で 1.5)
    AddPropertyFromNm("RINDEX", {
        { 360.0, 1.5 },
        { 520.0, 1.5 }
    });

    //吸収長 (全波長で 2000 mm)
    AddPropertyFromNm("ABSLENGTH", {
        { 360.0, 2000.0 * mm },
        { 520.0, 2000.0 * mm }
    });

    fMaterial->SetMaterialPropertiesTable(mpt);
}

AcrylicMat::~AcrylicMat() {
}