#include "LogVol/HILELogVol.hh"
#include "Material/BC408Mat.hh"
#include "Material/PMTGlassMat.hh"
#include "Surface/UROKO/CathodeSurface.hh"

#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4Trd.hh"
#include "G4UnionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4MultiUnion.hh"

#include "G4LogicalVolume.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpticalSurface.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"
#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

using namespace CLHEP;

namespace {

enum Axis { X, Y, Z };

G4Transform3D Rotate(int axis, double angle)
{
    G4RotationMatrix rot;

    if (axis == Axis::X) rot.rotateX(angle);
    if (axis == Axis::Y) rot.rotateY(angle);
    if (axis == Axis::Z) rot.rotateZ(angle);

    return G4Transform3D(rot, G4ThreeVector(0, 0, 0));
}

} // namespace

HILELogVol::HILELogVol(
    G4String Name,
    G4UserLimits* fStepLimit,
    G4bool checkOverlaps)
{
    // =============================================================
    // 1. 寸法パラメータ
    // =============================================================

    // シンチレータ
    const G4double rMin_Scinti = 700.0 * mm;
    const G4double rMax_Scinti = 712.5 * mm;
    const G4double h_Scinti = 150.0 * mm;
    const G4double dPhi_Scinti = 12.3 * deg;

    // ライトガイド
    const G4double h_GuideTaper = 45.0 * mm;
    const G4double h_GuideCyl = 5.0 * mm;

    // PMT：Li-glassと同じ寸法
    const G4double d_PMT = 53.0 * mm;
    const G4double h_PMT = 215.0 * mm;

    // 光電面：Li-glassと同じ寸法
    const G4double d_Cathode = 46.0 * mm;
    const G4double t_Cathode = 1.0 * mm;

    // PMT入射面から光電面までのガラス厚
    const G4double t_PMTWindow = 1.0 * mm;

    // =============================================================
    // 2. 各部品のSolid
    // =============================================================

    // シンチレータ：湾曲板
    auto* solid_Scinti = new G4Tubs(
        Name + "_ScintiSolid",
        rMin_Scinti,
        rMax_Scinti,
        h_Scinti / 2.0,
        0,
        dPhi_Scinti);

    // =============================================================
    // ライトガイド
    //
    // 図面の主要寸法：
    //   全長                  50 mm
    //   入口幅               150 mm
    //   入口厚さ              12.5 mm
    //   出口円柱直径          48 mm
    //   出口円柱長さ           5 mm
    //   幅方向の入口直線部     5 mm
    //
    // 円錐台を平面で切り出した形状として再現する。
    // =============================================================

    const G4double guideInputWidth = 150.0 * mm;
    const G4double guideInputThickness = 12.5 * mm;
    const G4double guideOutputDiameter = 48.0 * mm;
    const G4double guideInputStraightLength = 5.0 * mm;

    const G4double guideInputHalfWidth =
        guideInputWidth / 2.0;

    const G4double guideOutputRadius =
        guideOutputDiameter / 2.0;

    // 幅方向の傾斜が続く長さ：45 - 5 = 40 mm
    const G4double guideWidthTaperLength =
        h_GuideTaper - guideInputStraightLength;

    // 円錐台の傾斜を入口まで延長した半径。
    //
    // 入口から5 mmの位置で半径75 mm、
    // 入口から45 mmの位置で半径24 mmとなる。
    //
    // 24 + (75 - 24) * 45 / 40 = 81.375 mm
    const G4double guideConeInputRadius =
        guideOutputRadius
        + (guideInputHalfWidth - guideOutputRadius)
            * h_GuideTaper / guideWidthTaperLength;

    auto* solid_Cone = new G4Cons(
        Name + "_Cone",
        0,
        guideConeInputRadius,
        0,
        guideOutputRadius,
        h_GuideTaper / 2.0,
        0,
        360 * deg);

    // 幅を150 mmに制限する。
    // 厚さは入口12.5 mmから出口48 mmまで連続的に広がる。
    auto* solid_FlatCut = new G4Trd(
        Name + "_FlatCut",
        guideInputHalfWidth,
        guideInputHalfWidth,
        guideInputThickness / 2.0,
        guideOutputRadius,
        h_GuideTaper / 2.0);

    auto* solid_Taper = new G4IntersectionSolid(
        Name + "_Taper",
        solid_Cone,
        solid_FlatCut);

    // PMT接合側：直径48 mm、長さ5 mm
    auto* solid_TopCyl = new G4Tubs(
        Name + "_TopCyl",
        0,
        guideOutputRadius,
        h_GuideCyl / 2.0,
        0,
        360 * deg);

    auto* solid_Guide = new G4UnionSolid(
        Name + "_GuideSolid",
        solid_Taper,
        solid_TopCyl,
        nullptr,
        G4ThreeVector(
            0,
            0,
            h_GuideTaper / 2.0 + h_GuideCyl / 2.0));

    // PMT：ガラスの円柱
    auto* solid_PMT = new G4Tubs(
        Name + "_PMTSolid",
        0,
        d_PMT / 2.0,
        h_PMT / 2.0,
        0,
        360 * deg);

    // 光電面
    auto* solid_Cathode = new G4Tubs(
        Name + "_CathodeSolid",
        0,
        d_Cathode / 2.0,
        t_Cathode / 2.0,
        0,
        360 * deg);

    // =============================================================
    // 3. 各部品の配置変換
    // 親Solidの合成と子Volumeの配置で共有する。
    // =============================================================

    G4Transform3D transform_Scinti =
        G4Translate3D(
            -(rMin_Scinti + rMax_Scinti) / 2.0,
            0,
            0);

    G4Transform3D transform_Guide =
        Rotate(Axis::Y, 90.0 * deg)
        * Rotate(Axis::X, 90.0 * deg)
        * G4Translate3D(0, 0, h_GuideTaper / 2.0);

    // PMT入射面はライトガイド出口に接触する。
    // PMTの長さを変更しても、この接合面の位置は維持される。
    G4Transform3D transform_PMT =
        G4Translate3D(0, -(h_GuideCyl + h_GuideTaper), 0)
        * Rotate(Axis::X, 90.0 * deg)
        * G4Translate3D(0, 0, h_PMT / 2.0);

    // =============================================================
    // 4. 親Solid：各部品の和集合
    // 光電面はPMT内部に収まるため別途追加しない。
    // =============================================================

    auto* hileEnvelope = new G4MultiUnion(Name + "_Solid");

    hileEnvelope->AddNode(*solid_Scinti, transform_Scinti);
    hileEnvelope->AddNode(*solid_Guide, transform_Guide);
    hileEnvelope->AddNode(*solid_PMT, transform_PMT);

    hileEnvelope->Voxelize();

    Solid = hileEnvelope;

    // =============================================================
    // 5. Material
    // =============================================================

    auto* fBC408 = new BC408Mat();
    auto* fPMTGlass = new PMTGlassMat();
    auto* nist = G4NistManager::Instance();

    auto* matVacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matBC408 = fBC408->GetMaterial();
    auto* matAcrylic = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
    auto* matGlass = fPMTGlass->GetMaterial();
    auto* matAl = nist->FindOrBuildMaterial("G4_Al");

    // =============================================================
    // 6. Logical Volumeと可視化
    // =============================================================

    // 親の領域は子部品で満たされる。
    // 部品の周囲は親Volumeに含まれず、Worldの素材となる。
    LogVol = new G4LogicalVolume(
        Solid,
        matVacuum,
        Name + "_LogVol",
        nullptr,
        nullptr,
        fStepLimit,
        false);

    LogVol->SetVisAttributes(G4VisAttributes::GetInvisible());

    // シンチレータ
    LogVol_Scinti = new G4LogicalVolume(
        solid_Scinti,
        matBC408,
        Name + "_LV_Scinti",
        nullptr,
        nullptr,
        fStepLimit,
        false);

    auto* visScinti = new G4VisAttributes(
        true, G4Colour(0.0, 0.0, 1.0, 0.5));
    visScinti->SetForceSolid(true);
    LogVol_Scinti->SetVisAttributes(visScinti);

    // ライトガイド
    auto* LogVol_Guide = new G4LogicalVolume(
        solid_Guide,
        matAcrylic,
        Name + "_LV_Guide",
        nullptr,
        nullptr,
        fStepLimit,
        false);

    auto* visGuide = new G4VisAttributes(
        true, G4Colour(0.5, 0.5, 0.5, 0.4));
    visGuide->SetForceSolid(true);
    LogVol_Guide->SetVisAttributes(visGuide);

    // PMT
    auto* LogVol_PMT = new G4LogicalVolume(
        solid_PMT,
        matGlass,
        Name + "_LV_PMT",
        nullptr,
        nullptr,
        fStepLimit,
        false);

    auto* visPMT = new G4VisAttributes(
        true, G4Colour(1.0, 0.0, 1.0, 0.8));
    visPMT->SetForceSolid(true);
    LogVol_PMT->SetVisAttributes(visPMT);

    // 光電面
    LogVol_Cathode = new G4LogicalVolume(
        solid_Cathode,
        matAl,
        Name + "_LV_Cathode",
        nullptr,
        nullptr,
        fStepLimit,
        false);

    auto* visCathode = new G4VisAttributes(
        true, G4Colour(1.0, 1.0, 0.0, 1.0));
    visCathode->SetForceSolid(true);
    LogVol_Cathode->SetVisAttributes(visCathode);

    // =============================================================
    // 7. Physical Volume
    // =============================================================

    new G4PVPlacement(
        transform_Scinti,
        LogVol_Scinti,
        "Scinti",
        LogVol,
        false,
        0,
        checkOverlaps);

    new G4PVPlacement(
        transform_Guide,
        LogVol_Guide,
        "Guide",
        LogVol,
        false,
        0,
        checkOverlaps);

    new G4PVPlacement(
        transform_PMT,
        LogVol_PMT,
        "PMT",
        LogVol,
        false,
        0,
        checkOverlaps);

    // HILEでは、PMTのローカル-Z側が入射面。
    // 入射面から1 mmのガラスを挟んで光電面を配置する。
    const G4double z_cathode =
        -h_PMT / 2.0 + t_PMTWindow + t_Cathode / 2.0;

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(0, 0, z_cathode),
        LogVol_Cathode,
        "Cathode",
        LogVol_PMT,
        false,
        0,
        checkOverlaps);

    // =============================================================
    // 8. 光電面の光学境界
    // Li-glassと同じCathodeSurfaceを使用する。
    // SDの割り当ては別途ConstructSDandField()で実装する。
    // =============================================================

    auto* surfCathode = (new CathodeSurface())->GetSurface();

    new G4LogicalSkinSurface(
        Name + "_CathodeSkin",
        LogVol_Cathode,
        surfCathode);
}

HILELogVol::~HILELogVol()
{
    delete Solid;
    delete LogVol;
}