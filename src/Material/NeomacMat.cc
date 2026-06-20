#include "Material/NeomaxMat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

NeomaxMat::NeomaxMat() {
    G4NistManager* nist = G4NistManager::Instance();

    // 1. Element（元素）の取得
    G4Element* elNd = nist->FindOrBuildElement("Nd"); // ネオジム
    G4Element* elFe = nist->FindOrBuildElement("Fe"); // 鉄
    G4Element* elB  = nist->FindOrBuildElement("B");  // ホウ素（ボロン）

    // 2. NEOMAX (Nd2Fe14B) のマテリアル作成
    // 密度はネオジム磁石の一般的な値（約7.5 g/cm3）を使用します
    G4double density_Neomax = 7.5 * g/cm3;
    fMaterial = new G4Material("NEOMAX", density_Neomax, 3);

    // 3. 元素を原子数比（Nd : Fe : B = 2 : 14 : 1）で追加
    fMaterial->AddElement(elNd, 2);
    fMaterial->AddElement(elFe, 14);
    fMaterial->AddElement(elB,  1);
}

NeomaxMat::~NeomaxMat() {
    // 補足: G4MaterialはGeant4のカーネルが内部で一括管理して破棄してくれるため、
    // ここでわざわざ delete fMaterial; を書く必要はありません。
}