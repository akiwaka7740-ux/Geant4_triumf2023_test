#include "LogVol/UROKOLogVol.hh"

#include "Material/BC408Mat.hh"
#include "Material/PMTGlassMat.hh"
#include "Material/VacuumMat.hh"
#include "Material/AcrylicMat.hh"

#include "Surface/UROKO/CathodeSurface.hh"
#include "Surface/UROKO/DielectricSurface.hh"
#include "Surface/UROKO/MirrorSurface.hh"
#include "Surface/UROKO/DiffuseSurface.hh"

#include "G4Box.hh"
#include "G4Trd.hh"
#include "G4Polyhedra.hh"
#include "G4UnionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4MultiUnion.hh"

#include "G4LogicalVolume.hh"
#include "G4LogicalBorderSurface.hh"
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

#include <cmath>

using namespace CLHEP;

namespace {

G4Transform3D Move(
    G4double dx,
    G4double dy,
    G4double dz)
{
    return G4Transform3D(
        G4RotationMatrix(),
        G4ThreeVector(dx, dy, dz));
}

} // namespace

UROKOLogVol::UROKOLogVol(
    G4String Name,
    G4UserLimits* fStepLimit,
    G4bool checkOverlaps)
{
    // =============================================================
    // 1. 寸法パラメータ
    // =============================================================

    // シンチレータ前面の薄い真空層
    const G4double bumper_T = 0.01 * mm;

    // シンチレータ
    const G4double thickness_UROKO = 29.0 * mm;
    const G4double hexagon_r_UROKO = 100.0 * mm;
    const G4double hexagon_rr_UROKO =
        hexagon_r_UROKO * std::sqrt(3.0) / 2.0;

    // ライトガイド
    const G4double guide_L_UROKO = 40.0 * mm;
    const G4double guide_S_UROKO = 10.0 * mm;

    // PMT
    const G4double PMT_W_UROKO = 26.2 * mm;
    const G4double PMT_L_UROKO = 32.5 * mm;
    const G4double PMT_C_UROKO = 30.0 * mm;

    // 光電面
    const G4double cathode_W_UROKO = 23.0 * mm;
    const G4double cathode_T_UROKO = 0.8 * mm;

    // 全長：111.51 mm
    total_Z =
        bumper_T
        + thickness_UROKO
        + guide_L_UROKO
        + guide_S_UROKO
        + PMT_L_UROKO;

    // =============================================================
    // 2. 各部品のSolid
    // =============================================================

    // -------------------------------------------------------------
    // 2.1 Bumper・シンチレータ
    // -------------------------------------------------------------

    G4double rInner[] = {0.0, 0.0};

    G4double zScinti[] = {
        0.0,
        thickness_UROKO
    };

    G4double rOuterScinti[] = {
        hexagon_rr_UROKO,
        hexagon_rr_UROKO
    };

    auto* solid_Scinti = new G4Polyhedra(
        Name + "_ScintiSolid",
        0 * deg,
        360 * deg,
        6,
        2,
        zScinti,
        rInner,
        rOuterScinti);

    // 従来どおり、シンチレータよりわずかに大きくする。
    const G4double bumper_r_in =
        hexagon_r_UROKO + 0.001 * mm;

    const G4double bumper_rr_in =
        bumper_r_in * std::sqrt(3.0) / 2.0;

    G4double zBumper[] = {
        0.0,
        bumper_T
    };

    G4double rOuterBumper[] = {
        bumper_rr_in,
        bumper_rr_in
    };

    auto* solid_Bumper = new G4Polyhedra(
        Name + "_BumperSolid",
        0 * deg,
        360 * deg,
        6,
        2,
        zBumper,
        rInner,
        rOuterBumper);

    // -------------------------------------------------------------
    // 2.2 ライトガイド
    // -------------------------------------------------------------

    const G4double guide_r_in =
        hexagon_r_UROKO + 0.001 * mm;

    const G4double guide_rr_in =
        guide_r_in * std::sqrt(3.0) / 2.0;

    // 2個のPMTと、その間の隙間を合わせた幅：56.2 mm
    const G4double PMT_W2_UROKO =
        (PMT_W_UROKO
         + (PMT_C_UROKO - PMT_W_UROKO) / 2.0)
        * 2.0;

    G4double zGuide[] = {
        -guide_L_UROKO / 2.0,
         guide_L_UROKO / 2.0
    };

    G4double rOuterGuide[] = {
        guide_rr_in,
        (PMT_W2_UROKO * std::sqrt(3.0) + PMT_W_UROKO)
            / 4.0
    };

    auto* solid_Guide1 = new G4Trd(
        Name + "_Guide1",
        guide_r_in,
        PMT_W2_UROKO / 2.0,
        guide_rr_in,
        PMT_W_UROKO / 2.0,
        guide_L_UROKO / 2.0);

    auto* solid_Guide2 = new G4Polyhedra(
        Name + "_Guide2",
        0 * deg,
        360 * deg,
        6,
        2,
        zGuide,
        rInner,
        rOuterGuide);

    auto* solid_Guide3 = new G4IntersectionSolid(
        Name + "_Guide3",
        solid_Guide1,
        solid_Guide2);

    // 出口側の直方体部分
    auto* solid_Guide4 = new G4Box(
        Name + "_Guide4",
        PMT_W2_UROKO / 2.0,
        PMT_W_UROKO / 2.0,
        guide_S_UROKO / 2.0);

    const G4Transform3D transform_GuideExit = Move(
        0,
        0,
        guide_L_UROKO / 2.0 + guide_S_UROKO / 2.0);

    auto* solid_Guide = new G4UnionSolid(
        Name + "_GuideSolid",
        solid_Guide3,
        solid_Guide4,
        transform_GuideExit);

    // -------------------------------------------------------------
    // 2.3 PMT・光電面
    // -------------------------------------------------------------

    auto* solid_PMT = new G4Box(
        Name + "_PMTSolid",
        PMT_W_UROKO / 2.0,
        PMT_W_UROKO / 2.0,
        PMT_L_UROKO / 2.0);

    auto* solid_Cathode = new G4Box(
        Name + "_CathodeSolid",
        cathode_W_UROKO / 2.0,
        cathode_W_UROKO / 2.0,
        cathode_T_UROKO / 2.0);

    // =============================================================
    // 3. 各部品の配置変換
    // 親Solidの合成と子Volumeの配置で共有する。
    // =============================================================

    const G4double z_bump_pos = -total_Z / 2.0;

    const G4double z_Scinti =
        z_bump_pos + bumper_T;

    const G4double z_guide =
        z_Scinti
        + thickness_UROKO
        + guide_L_UROKO / 2.0;

    const G4double z_pmt =
        z_Scinti
        + thickness_UROKO
        + guide_L_UROKO
        + guide_S_UROKO
        + PMT_L_UROKO / 2.0;

    G4Transform3D transform_Bumper =
        Move(0, 0, z_bump_pos);

    G4Transform3D transform_Scinti =
        Move(0, 0, z_Scinti);

    G4Transform3D transform_Guide =
        Move(0, 0, z_guide);

    G4Transform3D transform_PMT0 =
        Move(-PMT_C_UROKO / 2.0, 0, z_pmt);

    G4Transform3D transform_PMT1 =
        Move(PMT_C_UROKO / 2.0, 0, z_pmt);

    // =============================================================
    // 4. 親Solid：全5部品の和集合
    //
    // Bumper、Scinti、Guide、PMT0、PMT1を含める。
    // 光電面はPMT内部に収まるため別途追加しない。
    // =============================================================

    auto* urokoEnvelope = new G4MultiUnion(Name + "_Solid");

    urokoEnvelope->AddNode(*solid_Bumper, transform_Bumper);
    urokoEnvelope->AddNode(*solid_Scinti, transform_Scinti);
    urokoEnvelope->AddNode(*solid_Guide, transform_Guide);
    urokoEnvelope->AddNode(*solid_PMT, transform_PMT0);
    urokoEnvelope->AddNode(*solid_PMT, transform_PMT1);

    urokoEnvelope->Voxelize();

    Solid = urokoEnvelope;

    // =============================================================
    // 5. Material
    // =============================================================

    auto* fVacuum = new VacuumMat();
    auto* fBC408 = new BC408Mat();
    auto* fPMTGlass = new PMTGlassMat();
    auto* fAcrylic = new AcrylicMat();

    auto* matVacuum = fVacuum->GetMaterial();
    auto* matBC408 = fBC408->GetMaterial();
    auto* matGlass = fPMTGlass->GetMaterial();
    auto* matAcrylic = fAcrylic->GetMaterial();
    auto* matAl =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");

    // =============================================================
    // 6. Logical Volumeと可視化
    // =============================================================

    // 親は子部品と同じ占有領域を持つ。
    // 部品周囲やPMT間の空間はWorldの素材となる。
    LogVol = new G4LogicalVolume(
        Solid,
        matVacuum,
        Name + "_LogVol",
        nullptr,
        nullptr,
        fStepLimit,
        false);

    LogVol->SetVisAttributes(G4VisAttributes::GetInvisible());

    // 前面の薄い真空層
    auto* LogVol_Bumper = new G4LogicalVolume(
        solid_Bumper,
        matVacuum,
        Name + "_LV_Bumper",
        nullptr,
        nullptr,
        fStepLimit,
        false);

    LogVol_Bumper->SetVisAttributes(
        G4VisAttributes::GetInvisible());

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
    LogVol_Guide = new G4LogicalVolume(
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

    // PMT：同じLogical Volumeを2か所に配置する。
    LogVol_PMT = new G4LogicalVolume(
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
    // 親Solidへの登録と同じ配置変換を使用する。
    // =============================================================

    auto* physBumper = new G4PVPlacement(
        transform_Bumper,
        LogVol_Bumper,
        "Bumper",
        LogVol,
        false,
        0,
        checkOverlaps);

    auto* physScinti = new G4PVPlacement(
        transform_Scinti,
        LogVol_Scinti,
        "Scinti",
        LogVol,
        false,
        0,
        checkOverlaps);

    auto* physGuide = new G4PVPlacement(
        transform_Guide,
        LogVol_Guide,
        "Guide",
        LogVol,
        false,
        0,
        checkOverlaps);

    auto* physPMT0 = new G4PVPlacement(
        transform_PMT0,
        LogVol_PMT,
        "PMT0",
        LogVol,
        false,
        0,
        checkOverlaps);

    auto* physPMT1 = new G4PVPlacement(
        transform_PMT1,
        LogVol_PMT,
        "PMT1",
        LogVol,
        false,
        1,
        checkOverlaps);

    // 光電面はPMTの子Volume。
    // 従来どおり、PMT入射面から0.8 mm奥に前面を置く。
    const G4double z_cathode =
        -PMT_L_UROKO / 2.0
        + 3.0 * cathode_T_UROKO / 2.0;

    new G4PVPlacement(
        Move(0, 0, z_cathode),
        LogVol_Cathode,
        "Cathode",
        LogVol_PMT,
        false,
        0,
        checkOverlaps);

    // =============================================================
    // 8. Optical Surface
    // 使用中の光学境界設定を維持する。
    // =============================================================

    auto* surfCathode = (new CathodeSurface())->GetSurface();
    auto* surfDiel = (new DielectricSurface())->GetSurface();
    auto* surfMirror = (new MirrorSurface())->GetSurface();
    auto* surfDiffuse = (new DiffuseSurface())->GetSurface();

    // 全体の表面設定
    new G4LogicalSkinSurface(
        "ScintiSkin",
        LogVol_Scinti,
        surfMirror);

    new G4LogicalSkinSurface(
        "GuideSkin",
        LogVol_Guide,
        surfMirror);

    new G4LogicalSkinSurface(
        "CathodeSkin",
        LogVol_Cathode,
        surfCathode);

    // シンチレータ前面からBumperへ進む方向だけ上書きする。
    new G4LogicalBorderSurface(
        "ScintiToBumper",
        physScinti,
        physBumper,
        surfDiffuse);

    // シンチレータ ⇄ ライトガイド
    new G4LogicalBorderSurface(
        "ScintiToGuide",
        physScinti,
        physGuide,
        surfDiel);

    new G4LogicalBorderSurface(
        "GuideToScinti",
        physGuide,
        physScinti,
        surfDiel);

    // ライトガイド ⇄ PMT0
    new G4LogicalBorderSurface(
        "GuideToPMT0",
        physGuide,
        physPMT0,
        surfDiel);

    new G4LogicalBorderSurface(
        "PMT0ToGuide",
        physPMT0,
        physGuide,
        surfDiel);

    // ライトガイド ⇄ PMT1
    new G4LogicalBorderSurface(
        "GuideToPMT1",
        physGuide,
        physPMT1,
        surfDiel);

    new G4LogicalBorderSurface(
        "PMT1ToGuide",
        physPMT1,
        physGuide,
        surfDiel);
}

UROKOLogVol::~UROKOLogVol()
{
    delete Solid;
    delete LogVol;
}