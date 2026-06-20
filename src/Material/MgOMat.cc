#include "Material/MgOMat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

MgOMat::MgOMat() {
    G4NistManager* nist = G4NistManager::Instance();

    // 1. Elementの定義
    G4Element* elO  = nist->FindOrBuildElement("O");
    G4Element* elMg = nist->FindOrBuildElement("Mg");

    // 2. 構成化合物 (酸化物) の定義
    fMaterial = new G4Material("MgO", 3.58*g/cm3, 2);
    fMaterial->AddElement(elMg, 1);
    fMaterial->AddElement(elO,  1);



}

MgOMat::~MgOMat() {

}