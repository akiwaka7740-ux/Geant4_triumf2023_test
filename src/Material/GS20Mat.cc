#include "Material/GS20Mat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

GS20Mat::GS20Mat() {
    G4NistManager* nist = G4NistManager::Instance();

    // 1. Elementの定義
    G4Element* elO  = nist->FindOrBuildElement("O");
    G4Element* elSi = nist->FindOrBuildElement("Si");
    G4Element* elMg = nist->FindOrBuildElement("Mg");
    G4Element* elAl = nist->FindOrBuildElement("Al");
    G4Element* elCe = nist->FindOrBuildElement("Ce");       

    // 2. 濃縮リチウム (6Li 95% enrichment) の定義
    G4Isotope* iso_Li6 = new G4Isotope("Li6", 3, 6, 6.015*g/mole);
    G4Isotope* iso_Li7 = new G4Isotope("Li7", 3, 7, 7.016*g/mole);

    G4Element* elLi_enriched = new G4Element("Enriched_Lithium", "Li", 2);
    elLi_enriched->AddIsotope(iso_Li6, 95.0*perCent);
    elLi_enriched->AddIsotope(iso_Li7,  5.0*perCent);

    // 3. 各構成化合物 (酸化物) の定義
    G4Material* SiO2 = new G4Material("SiO2", 2.20*g/cm3, 2);
    SiO2->AddElement(elSi, 1);
    SiO2->AddElement(elO,  2);

    G4Material* MgO = new G4Material("MgO", 3.58*g/cm3, 2);
    MgO->AddElement(elMg, 1);
    MgO->AddElement(elO,  1);

    G4Material* Al2O3 = new G4Material("Al2O3", 3.95*g/cm3, 2);
    Al2O3->AddElement(elAl, 2);
    Al2O3->AddElement(elO,  3);

    G4Material* Ce2O3 = new G4Material("Ce2O3", 6.20*g/cm3, 2);
    Ce2O3->AddElement(elCe, 2);
    Ce2O3->AddElement(elO,  3);

    G4Material* Li2O = new G4Material("Li2O", 2.01*g/cm3, 2);
    Li2O->AddElement(elLi_enriched, 2);
    Li2O->AddElement(elO,  1);

    // 4. Liガラスシンチレータ (GS20) の完成
    G4double density_GS20 = 2.5 * g/cm3;
    // ここでローカル変数ではなく、クラスのメンバ変数 fMaterial に代入します
    fMaterial = new G4Material("GS20", density_GS20, 5);

    fMaterial->AddMaterial(SiO2,  58.5 * perCent);
    fMaterial->AddMaterial(MgO,    4.0 * perCent);
    fMaterial->AddMaterial(Al2O3, 18.0 * perCent);
    fMaterial->AddMaterial(Ce2O3,  4.0 * perCent);
    fMaterial->AddMaterial(Li2O,  15.5 * perCent);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    //本来はエネルギーでパラメータを登録する必要がある
    //ヘルパー関数：[波長(nm), 値] の表からGeant4用の物性配列を自動登録する
    auto AddPropertyFromNm = [&](const char* key, const std::vector<std::pair<double, double>>& data) {
        std::vector<G4double> energies;
        std::vector<G4double> values;

        // Geant4は「エネルギー昇順」必須。データシートの「波長昇順」を後ろから読むとちょうどエネルギー昇順になる
        for (auto it = data.rbegin(); it != data.rend(); ++it) //rbegin は reverse beginを指す（最後尾の要素からスタートする）
        {
            double nm = it->first;
            energies.push_back((1239.84193 / nm) * eV); // λ(nm) -> E(eV) 変換
            values.push_back(it->second);
        }
        mpt->AddProperty(key, energies, values, false, true);//4=新規か、5=spline補完を適用するか
    };

    //　光子の発生数
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. * 0.25 /MeV);   //カタログ上はplasticの20~30%
    //　光子発生のばらつき（ポアソン分布）  
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0); 
    //単一の成分のみと仮定
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 70*ns);    

}

GS20Mat::~GS20Mat() {
    // 補足: G4MaterialはGeant4のカーネルが内部で一括管理して破棄してくれるため、
    // ここでわざわざ delete fMaterial; を書く必要はありません。
}