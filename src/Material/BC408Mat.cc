#include "Material/BC408Mat.hh"
#include "G4NistManager.hh"

BC408Mat::BC408Mat() {
    G4NistManager* nist = G4NistManager::Instance();
    
    // NISTデータベースからポリビニルトルエン（BC408の主成分）を取得
    fMaterial = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");

    // =======================================================
    // 💡 将来的な拡張ポイント:
    // もし今後「Optical Photon（光学光子）」のシミュレーションを
    // 行う場合は、ここに G4MaterialPropertiesTable を作成し、
    // fMaterial->SetMaterialPropertiesTable(...) として
    // 屈折率や発光スペクトルを追加することになります。
    // =======================================================
}

BC408Mat::~BC408Mat() {
}