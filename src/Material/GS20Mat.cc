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
}

GS20Mat::~GS20Mat() {
    // 補足: G4MaterialはGeant4のカーネルが内部で一括管理して破棄してくれるため、
    // ここでわざわざ delete fMaterial; を書く必要はありません。
}