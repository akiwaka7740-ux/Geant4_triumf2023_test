#include "Material/util/common.hh"

#include "LogVol/MagnetLogVol.hh"
#include "Material/NeomaxMat.hh"

#include "G4VSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Polycone.hh"
#include "G4UnionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4MultiUnion.hh"

#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"
#include "G4SystemOfUnits.hh"

#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include <map>

namespace MagnetUtil {

enum Axis { X, Y, Z };

G4Transform3D Move(double dx, double dy, double dz)
{
    return G4Transform3D(
        G4RotationMatrix(),
        G4ThreeVector(dx, dy, dz)
    );
}

G4Transform3D Rotate(int axis, double angle)
{
    G4RotationMatrix rot;

    if (axis == Axis::X) rot.rotateX(angle);
    if (axis == Axis::Y) rot.rotateY(angle);
    if (axis == Axis::Z) rot.rotateZ(angle);

    return G4Transform3D(rot, G4ThreeVector(0, 0, 0));
}

enum class ColID {
    White,
    LightGrey,
    LightGray,
    Gray,
    Grey,
    Black,
    Brown,
    Red,
    Green,
    Blue,
    Cyan,
    Magenta,
    Yellow,
    DarkGrey,
};

struct RGB {
    double R;
    double G;
    double B;
};

static std::map<ColID, RGB> defaultRGB = {
    {ColID::White,     {1.0,  1.0,  1.0}},
    {ColID::LightGrey, {0.7,  0.7,  0.7}},
    {ColID::LightGray, {0.7,  0.7,  0.7}},
    {ColID::Gray,      {0.5,  0.5,  0.5}},
    {ColID::Grey,      {0.5,  0.5,  0.5}},
    {ColID::Black,     {0.0,  0.0,  0.0}},
    {ColID::Brown,     {0.45, 0.25, 0.0}},
    {ColID::Red,       {1.0,  0.0,  0.0}},
    {ColID::Green,     {0.0,  1.0,  0.0}},
    {ColID::Blue,      {0.0,  0.0,  1.0}},
    {ColID::Cyan,      {0.0,  1.0,  1.0}},
    {ColID::Magenta,   {1.0,  0.0,  1.0}},
    {ColID::Yellow,    {1.0,  1.0,  0.0}},
    {ColID::DarkGrey,  {0.25, 0.25, 0.25}},
};

G4Colour Color(ColID id, double opacity = 1.0)
{
    return G4Colour(
        defaultRGB[id].R,
        defaultRGB[id].G,
        defaultRGB[id].B,
        opacity
    );
}

} // namespace MagnetUtil

using namespace MagnetUtil;

MagnetLogVol::MagnetLogVol(
    G4String Name,
    G4UserLimits* fStepLimit,
    G4bool checkOverlaps
)
{
    if (logmode) {
        G4cout << "-- MagnetLogVol::MagnetLogVol(G4String)\n";
    }

    // =============================================================
    // 1. 共通寸法
    // =============================================================

    // X-2876 REV.02:
    // ヨーク内面間隔140 mm、両側の補助磁石各3 mm、
    // 補助磁石間の開口134 mmという図面解釈。
    const G4double clearGap = 134.0 * mm;
    const G4double auxiliaryMagnetThickness = 3.0 * mm;

    const G4double yokeInnerGap =
        clearGap + 2.0 * auxiliaryMagnetThickness;

    const G4double yokeThickness = 25.0 * mm;
    const G4double bodyWidth = 132.0 * mm;
    const G4double roundEndRadius = 66.0 * mm;

    // 暫定:
    // 図面の70.4 mm区間をY方向全幅にわたる板としてモデル化。
    // 平面形状と支柱段差側の磁石は、部品図との照合が必要。
    const G4double auxiliaryMagnetLength = 70.4 * mm;

    // =============================================================
    // 2. 各部品のSolid
    // =============================================================

    // -------------------------------------------------------------
    // 2.1 主磁石
    // 外径122 mm、軸方向長さ45 mm。
    // 内径は40、60、80、100 mmの4段。
    // -------------------------------------------------------------

    const G4int numZ_Mag = 8;

    G4double zPlane_Magnet[] = {
         0.0 * mm,  5.0 * mm,
         5.0 * mm, 10.0 * mm,
        10.0 * mm, 20.0 * mm,
        20.0 * mm, 45.0 * mm
    };

    G4double rInner_Mag[] = {
         40.0 / 2.0 * mm,  40.0 / 2.0 * mm,
         60.0 / 2.0 * mm,  60.0 / 2.0 * mm,
         80.0 / 2.0 * mm,  80.0 / 2.0 * mm,
        100.0 / 2.0 * mm, 100.0 / 2.0 * mm
    };

    G4double rOuter_Mag[] = {
        122.0 / 2.0 * mm, 122.0 / 2.0 * mm,
        122.0 / 2.0 * mm, 122.0 / 2.0 * mm,
        122.0 / 2.0 * mm, 122.0 / 2.0 * mm,
        122.0 / 2.0 * mm, 122.0 / 2.0 * mm
    };

    G4VSolid* solid_Magnet = new G4Polycone(
        Name + "_Magnet_Solid",
        0,
        360 * deg,
        numZ_Mag,
        zPlane_Magnet,
        rInner_Mag,
        rOuter_Mag
    );

    // -------------------------------------------------------------
    // 2.2 Back
    // 外径122 mm、軸方向長さ25 mm。
    // 最後の10 mmは内半径50→61 mmのテーパー。
    // -------------------------------------------------------------

    const G4int numZ_Back = 4;

    G4double zPlane_Back[] = {
         0.0 * mm, 15.0 * mm,
        15.0 * mm, 25.0 * mm
    };

    G4double rInner_Back[] = {
        100.0 / 2.0 * mm, 100.0 / 2.0 * mm,
        100.0 / 2.0 * mm, 122.0 / 2.0 * mm
    };

    G4double rOuter_Back[] = {
        122.0 / 2.0 * mm, 122.0 / 2.0 * mm,
        122.0 / 2.0 * mm, 122.0 / 2.0 * mm
    };

    G4VSolid* solid_Back = new G4Polycone(
        Name + "_Back_Solid",
        0,
        360 * deg,
        numZ_Back,
        zPlane_Back,
        rInner_Back,
        rOuter_Back
    );

    // -------------------------------------------------------------
    // 2.3 Body01
    // 長方形と半円を合成し、直径122 mmの穴を開ける。
    // -------------------------------------------------------------

    G4VSolid* tmp11 = new G4Tubs(
        Name + "_Body01_Hole",
        0,
        122.0 / 2.0 * mm,
        40.0 / 2.0 * mm,
        0,
        360 * deg
    );

    G4VSolid* tmp12 = new G4Box(
        Name + "_Body01_Box",
        (66.0 + 145.0) / 2.0 * mm,
        bodyWidth / 2.0,
        yokeThickness / 2.0
    );

    // -90度から180度分なので、ローカル+x側の半円。
    G4VSolid* tmp13 = new G4Tubs(
        Name + "_Body01_HalfCylinder",
        0,
        roundEndRadius,
        yokeThickness / 2.0,
        -90 * deg,
        180 * deg
    );

    const G4double tmp1_xshift =
        (66.0 + 145.0) / 2.0 * mm;

    G4ThreeVector translation_tmp11(tmp1_xshift, 0, 0);

    G4VSolid* tmp14 = new G4UnionSolid(
        Name + "_Body01_Outer",
        tmp12,
        tmp13,
        nullptr,
        translation_tmp11
    );

    G4VSolid* solid_Body01 = new G4SubtractionSolid(
        Name + "_Body01",
        tmp14,
        tmp11,
        nullptr,
        translation_tmp11
    );

    // -------------------------------------------------------------
    // 2.4 Body02
    // 小開口60×60 mmは現状維持。
    // 58×60 mmおよび角Rへの変更は保留。
    // -------------------------------------------------------------

    const G4double x_tmp21 = 60.0 * mm / 2.0;
    const G4double x_tmp22 = 78.0 * mm / 2.0;

    // Y軸90度回転後のWorld Z方向の半長。
    const G4double x_tmp23 = yokeInnerGap / 2.0;

    const G4double y_tmp21 = 60.0 * mm / 2.0;
    const G4double y_tmp22 = bodyWidth / 2.0;
    const G4double y_tmp23 = bodyWidth / 2.0;

    const G4double z_tmp21 = 30.0 * mm / 2.0;
    const G4double z_tmp22 = 43.6 * mm / 2.0;
    const G4double z_tmp23 = z_tmp21 + z_tmp22;

    const G4double cutMargin = 5.0 * mm;

    G4VSolid* tmp21 = new G4Box(
        Name + "_Body02_SmallCut",
        x_tmp21,
        y_tmp21,
        z_tmp21 + cutMargin
    );

    G4VSolid* tmp22 = new G4Box(
        Name + "_Body02_LargeCut",
        x_tmp22,
        y_tmp22 + cutMargin,
        z_tmp22 + cutMargin
    );

    G4VSolid* tmp23 = new G4Box(
        Name + "_Body02_Base",
        x_tmp23,
        y_tmp23,
        z_tmp23
    );

    // 切り抜きは外側へ延長し、内部の段差位置を保持する。
    // 小開口の奥行き30 mm、大開口の奥行き43.6 mm。
    G4ThreeVector translation_tmp21(
        0,
        0,
        -(z_tmp23 - z_tmp21) - cutMargin
    );

    G4ThreeVector translation_tmp22(
        0,
        0,
        (z_tmp23 - z_tmp22) + cutMargin
    );

    G4VSolid* tmp24 = new G4SubtractionSolid(
        Name + "_sub1",
        tmp23,
        tmp21,
        nullptr,
        translation_tmp21
    );

    G4VSolid* body02 = new G4SubtractionSolid(
        Name + "_body02",
        tmp24,
        tmp22,
        nullptr,
        translation_tmp22
    );

    // -------------------------------------------------------------
    // 2.5 補助磁石
    // -------------------------------------------------------------

    G4VSolid* solid_AuxiliaryMagnet = new G4Box(
        Name + "_AuxiliaryMagnet_Solid",
        auxiliaryMagnetLength / 2.0,
        bodyWidth / 2.0,
        auxiliaryMagnetThickness / 2.0
    );

    // =============================================================
    // 3. 各部品の配置変換
    // 親の和集合と子のPhysicalVolumeで、同じ変換を共有する。
    // =============================================================

    const G4double h_Mag = 45.0 * mm;

    const G4double z_Mag = 49.6 * mm / 2.0;
    const G4double z_Back = z_Mag + h_Mag;

    const G4double x_Body01 = -tmp1_xshift;

    // 主磁石・Backの位置は維持する。
    // 主磁石背面とヨーク内面のZ座標差0.2 mmの
    // 部材対応（クランパ等）は未確定。
    const G4double z_Body01 =
        yokeInnerGap / 2.0 + yokeThickness / 2.0;

    const G4double x_Body02 =
        -(roundEndRadius + auxiliaryMagnetLength + z_tmp23);

    const G4double x_AuxiliaryMagnet =
        -(roundEndRadius + auxiliaryMagnetLength / 2.0);

    const G4double z_AuxiliaryMagnet =
        clearGap / 2.0 + auxiliaryMagnetThickness / 2.0;

    // +Z側
    const G4Transform3D trans_Magnet01 =
        Move(0, 0, z_Mag);

    const G4Transform3D trans_Back01 =
        Move(0, 0, z_Back);

    const G4Transform3D trans_Body01_01 =
        Move(x_Body01, 0, z_Body01);

    // -Z側。主磁石とBackは反転する。
    const G4Transform3D trans_Magnet02 =
        Move(0, 0, -z_Mag) * Rotate(Axis::Y, 180 * deg);

    const G4Transform3D trans_Back02 =
        Move(0, 0, -z_Back) * Rotate(Axis::Y, 180 * deg);

    const G4Transform3D trans_Body01_02 =
        Move(x_Body01, 0, -z_Body01);

    // 支柱
    const G4Transform3D trans_Body02 =
        Move(x_Body02, 0, 0) * Rotate(Axis::Y, 90 * deg);

    // 補助磁石
    const G4Transform3D trans_AuxiliaryMagnet01 =
        Move(x_AuxiliaryMagnet, 0, z_AuxiliaryMagnet);

    const G4Transform3D trans_AuxiliaryMagnet02 =
        Move(x_AuxiliaryMagnet, 0, -z_AuxiliaryMagnet);

    // =============================================================
    // 4. 全9部品の和集合を親Solidとして作成
    // 中央の隙間・貫通穴は親の占有領域に含めない。
    // =============================================================

    auto* magnetEnvelope = new G4MultiUnion(Name + "_Solid");

    magnetEnvelope->AddNode(*solid_Magnet, trans_Magnet01);
    magnetEnvelope->AddNode(*solid_Magnet, trans_Magnet02);

    magnetEnvelope->AddNode(*solid_Back, trans_Back01);
    magnetEnvelope->AddNode(*solid_Back, trans_Back02);

    magnetEnvelope->AddNode(*solid_Body01, trans_Body01_01);
    magnetEnvelope->AddNode(*solid_Body01, trans_Body01_02);

    magnetEnvelope->AddNode(*body02, trans_Body02);

    magnetEnvelope->AddNode(
        *solid_AuxiliaryMagnet,
        trans_AuxiliaryMagnet01
    );

    magnetEnvelope->AddNode(
        *solid_AuxiliaryMagnet,
        trans_AuxiliaryMagnet02
    );

    // 全ノード登録後に呼び出す。
    magnetEnvelope->Voxelize();

    Solid = magnetEnvelope;

    // =============================================================
    // 5. Material と LogicalVolume
    // =============================================================

    NeomaxMat* neomaxMat = new NeomaxMat();
    G4NistManager* nist = G4NistManager::Instance();

    G4Material* matNeomax = neomaxMat->GetMaterial();
    G4Material* matYoke = nist->FindOrBuildMaterial("G4_Fe");
    G4Material* matAir = nist->FindOrBuildMaterial("G4_AIR");

    // 親は全9部品の和集合。各領域の材質は子部品が与える。
    LogVol = new G4LogicalVolume(
        Solid,
        matAir,
        Name + "_LogVol",
        nullptr,
        nullptr,
        fStepLimit
    );

    LogVol->SetVisAttributes(G4VisAttributes::GetInvisible());

    G4LogicalVolume* LogVol_Mag = new G4LogicalVolume(
        solid_Magnet,
        matNeomax,
        Name + "_Mag_LV",
        nullptr,
        nullptr,
        fStepLimit
    );

    G4LogicalVolume* LogVol_Back = new G4LogicalVolume(
        solid_Back,
        matYoke,
        Name + "_Back_LV",
        nullptr,
        nullptr,
        fStepLimit
    );

    G4LogicalVolume* LogVol_Body01 = new G4LogicalVolume(
        solid_Body01,
        matYoke,
        Name + "_Body01_LV",
        nullptr,
        nullptr,
        fStepLimit
    );

    G4LogicalVolume* LogVol_Body02 = new G4LogicalVolume(
        body02,
        matYoke,
        Name + "_Body02_LV",
        nullptr,
        nullptr,
        fStepLimit
    );

    G4LogicalVolume* LogVol_AuxiliaryMagnet = new G4LogicalVolume(
        solid_AuxiliaryMagnet,
        matNeomax,
        Name + "_AuxiliaryMagnet_LV",
        nullptr,
        nullptr,
        fStepLimit
    );

    // =============================================================
    // 6. 表示属性
    // 現行の全パーツDarkGreyを維持。
    // =============================================================

    auto* visMag = new G4VisAttributes(
        TRUE, Color(ColID::DarkGrey)
    );
    visMag->SetForceSolid(true);
    LogVol_Mag->SetVisAttributes(visMag);

    auto* visBack = new G4VisAttributes(
        TRUE, Color(ColID::DarkGrey)
    );
    visBack->SetForceSolid(true);
    LogVol_Back->SetVisAttributes(visBack);

    auto* visBody01 = new G4VisAttributes(
        TRUE, Color(ColID::DarkGrey)
    );
    visBody01->SetForceSolid(true);
    LogVol_Body01->SetVisAttributes(visBody01);

    auto* visBody02 = new G4VisAttributes(
        TRUE, Color(ColID::DarkGrey)
    );
    visBody02->SetForceSolid(true);
    LogVol_Body02->SetVisAttributes(visBody02);

    auto* visAuxiliaryMagnet = new G4VisAttributes(
        TRUE, Color(ColID::DarkGrey)
    );
    visAuxiliaryMagnet->SetForceSolid(true);
    LogVol_AuxiliaryMagnet->SetVisAttributes(visAuxiliaryMagnet);

    // =============================================================
    // 7. PhysicalVolume
    // 和集合に登録した位置・回転と完全に一致させる。
    // =============================================================

    new G4PVPlacement(
        trans_Magnet01,
        LogVol_Mag,
        "_Mag_01",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_Back01,
        LogVol_Back,
        "_Back_01",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_Body01_01,
        LogVol_Body01,
        "_Body01_01",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_Magnet02,
        LogVol_Mag,
        "_Mag_02",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_Back02,
        LogVol_Back,
        "_Back_02",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_Body01_02,
        LogVol_Body01,
        "_Body01_02",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_Body02,
        LogVol_Body02,
        "_Body02",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_AuxiliaryMagnet01,
        LogVol_AuxiliaryMagnet,
        Name + "_AuxiliaryMagnet_01",
        LogVol,
        false,
        0,
        checkOverlaps
    );

    new G4PVPlacement(
        trans_AuxiliaryMagnet02,
        LogVol_AuxiliaryMagnet,
        Name + "_AuxiliaryMagnet_02",
        LogVol,
        false,
        1,
        checkOverlaps
    );

    if (logmode) {
        G4cout << "== MagnetLogVol::MagnetLogVol(G4String)\n";
    }
}

MagnetLogVol::~MagnetLogVol()
{
    if (logmode) {
        G4cout << "-- MagnetLogVol::~MagnetLogVol()\n";
    }

    delete Solid;
    delete LogVol;

    if (logmode) {
        G4cout << "== MagnetLogVol::~MagnetLogVol()\n";
    }
}