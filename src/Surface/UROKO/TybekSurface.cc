#include "Surface/UROKO/TybekSurface.hh"

TybekSurface::TybekSurface() {
    // "TybekSurface" という名前の光学面オブジェクトを作成
    fSurface = new G4OpticalSurface("TybekSurface");

    // 実測データ(LUT)モデルを採用
    fSurface->SetType(dielectric_LUT);
    fSurface->SetModel(LUT);

    // デュポン社タイベック + 空気ギャップ仕様の実測データを適用
    fSurface->SetFinish(groundtyvekair);
}

TybekSurface::~TybekSurface() {}