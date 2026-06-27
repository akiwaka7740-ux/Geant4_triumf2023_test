#include "Material/PMTGlassMat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

PMTGlassMat::PMTGlassMat() {
    G4NistManager* nist = G4NistManager::Instance();
    
    // NISTの標準パイレックスガラスをベースに、独立した "PMT_Glass" を生成
    G4Material* baseMat = nist->FindOrBuildMaterial("G4_Pyrex_Glass");
    fMaterial = new G4Material("PMT_Glass", baseMat->GetDensity(), baseMat);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    //本来はエネルギーでパラメータを登録する必要がある
    //ヘルパー関数：[波長(nm), 値] の表からGeant4用の物性配列を自動登録する
    auto AddPropertyFromNm = [&](const char* key, const std::vector<std::pair<double, double>>& data) {
        std::vector<G4double> energies;
        std::vector<G4double> values;
        for (auto it = data.rbegin(); it != data.rend(); ++it) {
            energies.push_back((1239.84193 / it->first) * eV);
            values.push_back(it->second);
        }
        mpt->AddProperty(key, energies, values, false, true); 
    };
 
    //屈折率(値が一定なら、始点と終点のみで良い）
    AddPropertyFromNm("RINDEX", {
        { 360.0, 1.473 },
        { 520.0, 1.473 }
    });

    //吸収長
    AddPropertyFromNm("ABSLENGTH", {
        { 360.0, 100.0 * mm },
        { 520.0, 100.0 * mm }
    });

    fMaterial->SetMaterialPropertiesTable(mpt);
}

PMTGlassMat::~PMTGlassMat() {
}