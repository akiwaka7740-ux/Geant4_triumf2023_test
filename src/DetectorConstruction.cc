#include "DetectorConstruction.hh"

#include "AnalysisConfig.hh"
#include "GeometryObjectType.hh"
#include "ScintillatorSD.hh"
#include "PhotocathodeSD.hh"

#include "LogVol/LiGlassLogVol.hh"
#include "LogVol/UROKOLogVol.hh"
#include "LogVol/HILELogVol.hh"
#include "LogVol/HPGeLogVol.hh"
#include "LogVol/BetaPlasticLogVol.hh"
#include "LogVol/MagnetLogVol.hh"
#include "LogVol/FrameLogVol.hh"
#include "LogVol/FloorLogVol.hh"
#include "LogVol/ShieldLogVol.hh"
#include "LogVol/ChamberLogVol.hh"
#include "LogVol/StopperLogVol.hh"

#include "G4Box.hh"
#include "G4Exception.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"

#include <map>
#include <utility>

// ======================================================================
// マスターコントロールパネル (検出器のON/OFFとID管理)
// ======================================================================
struct EnableAndID { G4bool Enable; G4int ID, nObj; };
/*
//本番用
std::map<G4String, EnableAndID> Mode = {
//  NAME         ENABLE   ID0   nObj
  {"LiGlass",   { false,    1,    1 } },
  {"UROKO",     { false,   10,    4 } },
  {"HILE",      { false,   20,    6 } },
  {"HPGe",      { false,   30,    7 } },
  {"BetaPlastic", { false,   40,    10 } },
  {"Magnet",     { false,   50,    1 } },
  { "Frame",     { false,   60,    1 } },
  { "Floor",     { false,   70,    1 } },
  { "Shield",    { false,   80,    6} },
  { "Chamber",   { false,   90,    1 } },
  { "Stopper",   { false,   100,    1 } },
};
*/

//テスト用
std::map<G4String, EnableAndID> Mode = {
    {"LiGlass", {
        false,
        ObjectBaseId(GeometryObjectType::LiGlass),
        1
    }},
    {"UROKO", {
        true,
        ObjectBaseId(GeometryObjectType::UROKO),
        4
    }},
    {"HILE", {
        false,
        ObjectBaseId(GeometryObjectType::HILE),
        6
    }},
    {"HPGe", {
        false,
        ObjectBaseId(GeometryObjectType::HPGe),
        7
    }},
    {"BetaPlastic", {
        false,
        ObjectBaseId(GeometryObjectType::BetaPlastic),
        10
    }},
    {"Magnet", {
        false,
        ObjectBaseId(GeometryObjectType::Magnet),
        1
    }},
    {"Frame", {
        false,
        ObjectBaseId(GeometryObjectType::Frame),
        1
    }},
    {"Floor", {
        false,
        ObjectBaseId(GeometryObjectType::Floor),
        1
    }},
    {"Shield", {
        false,
        ObjectBaseId(GeometryObjectType::Shield),
        6
    }},

    // Stopperの親VolumeはStopperであるため、同時に有効化することが必要
    {"Chamber", {
        false,
        ObjectBaseId(GeometryObjectType::Chamber),
        1
    }},
    {"Stopper", {
        false,
        ObjectBaseId(GeometryObjectType::Stopper),
        1
    }}
};

namespace {
    enum Axis { X, Y, Z };
    G4Transform3D Rotate(int axis, double angle) {
        G4RotationMatrix rot;
        if( axis==Axis::X ) rot.rotateX( angle );
        if( axis==Axis::Y ) rot.rotateY( angle );
        if( axis==Axis::Z ) rot.rotateZ( angle );
        return G4Transform3D( rot, G4ThreeVector( 0,0,0 ) );
    }
}

DetectorConstruction::DetectorConstruction(
    std::shared_ptr<const AnalysisConfig> config
)
    : G4VUserDetectorConstruction(),
      fAnalysisConfig(std::move(config))
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "DetectorConstruction::"
            "DetectorConstruction",
            "DetectorConstruction001",
            FatalException,
            "AnalysisConfig is null."
        );
    }
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{

    G4bool checkOverlaps = true;
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* matAir = nist->FindOrBuildMaterial("G4_AIR");

    // =============================================================
    // World Volume
    // =============================================================
    G4double world_size = 4.0 * m;
    G4Box* World_Solid = new G4Box("World_Solid", world_size/2., world_size/2., world_size/2.);
    fWorldLogicalVolume = new G4LogicalVolume(World_Solid, matAir, "World_LogVol");
    fWorldLogicalVolume->SetVisAttributes(new G4VisAttributes(TRUE, G4Colour(1.0, 1.0, 1.0, 0.0)));
    G4VPhysicalVolume* World_PhysVol = new G4PVPlacement(0, G4ThreeVector(), fWorldLogicalVolume, "World_PhysVol", 0, false, 0, checkOverlaps);

    G4String objName;

    // =============================================================
    // LiGlass Detector
    // =============================================================
    objName = "LiGlass";
    if( Mode[objName].Enable ) {
        fLiGlass = new LiGlassLogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* liGlassLogVol = fLiGlass->GetLogicalVolume();

        G4double frontLiGlass = -132.0 * mm;
        G4double length_scinti = 10.0 * mm;
        G4double length_PMT = 215.0 * mm;
        G4double length_emptyspace = 28.5 * mm;
        G4double totalLengthLiGlass = (length_emptyspace + length_scinti + length_PMT) ;

        G4RotationMatrix rotLiGlass;
        rotLiGlass.rotateX(-90.0 * deg);
        G4ThreeVector posLiGlass(0.0, frontLiGlass - (totalLengthLiGlass / 2.0) + length_emptyspace, 0.0);
        new G4PVPlacement(G4Transform3D(rotLiGlass, posLiGlass), objName+"_Phys", liGlassLogVol, World_PhysVol, false, Mode[objName].ID, checkOverlaps);
    }

    // =============================================================
    // UROKO Detector (4台)
    // =============================================================
    objName = "UROKO";
    if( Mode[objName].Enable ) {
        fUROKO = new UROKOLogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* UROKO_LogVol = fUROKO->GetLogicalVolume();

        /*
        //本番用　（旧式）
        const G4int nObj = Mode[objName].nObj;
        G4double updownDeg = 5.7 * deg;
        G4double sideDeg = 10.2 * deg;
        G4double margin_UROKO = 2.5 * mm;
        G4double hexagon_r = 100.0 * mm;
        G4double hexagon_rr = hexagon_r * std::sqrt(3.0) / 2.0;
        G4double total_Z_UROKO = 111.5 * mm;

        G4double swing_updown = (total_Z_UROKO / 2.0) * std::sin(updownDeg);
        G4double swing_side   = (total_Z_UROKO / 2.0) * std::sin(sideDeg);

        G4ThreeVector pos_UROKO[4] = {
                G4ThreeVector(1000*mm,  (hexagon_rr + margin_UROKO + swing_updown), 0),
                G4ThreeVector(1000*mm, -(hexagon_rr + margin_UROKO + swing_updown), 0),
                G4ThreeVector(1000*mm, 0,  (1.5 * hexagon_r + margin_UROKO + swing_side)),
                G4ThreeVector(1000*mm, 0, -(1.5 * hexagon_r + margin_UROKO + swing_side))
        };

        G4RotationMatrix rot_UROKO[4];
        rot_UROKO[0].rotateY(90.0*deg); rot_UROKO[0].rotateZ(updownDeg);
        rot_UROKO[1].rotateY(90.0*deg); rot_UROKO[1].rotateZ(-updownDeg);
        rot_UROKO[2].rotateY(90.0*deg - sideDeg);
        rot_UROKO[3].rotateY(90.0*deg + sideDeg);

        for(G4int i=0; i<nObj; i++) {
            pos_UROKO[i].setX(pos_UROKO[i].getX() + (total_Z_UROKO / 2.0));
            new G4PVPlacement(G4Transform3D(rot_UROKO[i], pos_UROKO[i]), objName+"_Phys", UROKO_LogVol, World_PhysVol, false, Mode[objName].ID + i, checkOverlaps);
        }
        */

        //本番用　（codex）

        constexpr G4int nObj = 4;

        // 配置数とID・解析側の設定を一致させる。
        if (Mode[objName].nObj != nObj) {
            G4Exception(
                "DetectorConstruction::Construct",
                "UROKOProductionCountMismatch",
                FatalException,
                "Production UROKO placement requires nObj = 4."
            );
            return nullptr;
        }

        // ---------------------------------------------------------
        // 配置パラメータ
        // ---------------------------------------------------------
        const G4double updownDeg = 5.7 * deg;
        const G4double sideDeg = 10.2 * deg;

        const G4double margin_UROKO = 2.5 * mm;
        const G4double hexagon_r = 100.0 * mm;
        const G4double hexagon_rr =
            hexagon_r * std::sqrt(3.0) / 2.0;

        const G4double total_Z_UROKO =
            fUROKO->GetTotalZ();

        const G4double bumperThickness = 0.01 * mm;

        // 原点からシンチレータ前面中心までの距離
        const G4double targetDistance = 1000.0 * mm;

        // ---------------------------------------------------------
        // 従来の配置を、各前面中心の方向を決める基準にする。
        // ---------------------------------------------------------
        const G4double swing_updown =
            (total_Z_UROKO / 2.0) * std::sin(updownDeg);

        const G4double swing_side =
            (total_Z_UROKO / 2.0) * std::sin(sideDeg);

        const G4ThreeVector pos_UROKO[nObj] = {
            G4ThreeVector(
                1000.0 * mm,
                hexagon_rr + margin_UROKO + swing_updown,
                0
            ),
            G4ThreeVector(
                1000.0 * mm,
                -(hexagon_rr + margin_UROKO + swing_updown),
                0
            ),
            G4ThreeVector(
                1000.0 * mm,
                0,
                1.5 * hexagon_r + margin_UROKO + swing_side
            ),
            G4ThreeVector(
                1000.0 * mm,
                0,
                -(1.5 * hexagon_r + margin_UROKO + swing_side)
            )
        };

        // ---------------------------------------------------------
        // 回転角は従来の本番配置を維持する。
        // ---------------------------------------------------------
        G4RotationMatrix rot_UROKO[nObj];

        rot_UROKO[0].rotateY(90.0 * deg);
        rot_UROKO[0].rotateZ(updownDeg);

        rot_UROKO[1].rotateY(90.0 * deg);
        rot_UROKO[1].rotateZ(-updownDeg);

        rot_UROKO[2].rotateY(90.0 * deg - sideDeg);
        rot_UROKO[3].rotateY(90.0 * deg + sideDeg);

        // シンチレータ前面中心の親Volume内ローカル座標。
        // 前面の薄い真空層（Bumper）の厚さを加える。
        const G4ThreeVector localFrontCenter(
            0,
            0,
            -total_Z_UROKO / 2.0 + bumperThickness
        );

        // ---------------------------------------------------------
        // 前面中心までの距離が1000 mmになるよう配置する。
        // ---------------------------------------------------------
        for (G4int i = 0; i < nObj; ++i) {
            G4ThreeVector originalPosition = pos_UROKO[i];

            originalPosition.setX(
                originalPosition.x() + total_Z_UROKO / 2.0
            );

            // 親原点から前面中心へのWorld座標系のベクトル
            const G4ThreeVector frontOffset =
                rot_UROKO[i] * localFrontCenter;

            // 従来の配置での前面中心
            const G4ThreeVector originalFrontCenter =
                originalPosition + frontOffset;

            // 原点から見た方向を維持し、距離を1000 mmにする。
            const G4ThreeVector targetFrontCenter =
                targetDistance * originalFrontCenter.unit();

            // 前面中心から親Volumeの配置位置を逆算する。
            const G4ThreeVector newPosition =
                targetFrontCenter - frontOffset;

            new G4PVPlacement(
                G4Transform3D(rot_UROKO[i], newPosition),
                objName + "_Phys",
                UROKO_LogVol,
                World_PhysVol,
                false,
                Mode[objName].ID + i,
                checkOverlaps
            );
        }

        /*
        //テスト用（1台だけ配置）
        const G4int nObj = Mode[objName].nObj;
        //ターゲット検出器間の距離は 1000 mm
        //G4ThreeVector pos_UROKO[1] = { G4ThreeVector(1000*mm, 0, 0) };

        G4ThreeVector pos_UROKO[] = { G4ThreeVector(1000.0*mm, 0, 0) };

        G4RotationMatrix rot_UROKO[1];
        rot_UROKO[0].rotateY(90.0*deg);

        for(G4int i=0; i<nObj; i++){
            pos_UROKO[i].setX(pos_UROKO[i].getX() + (fUROKO->GetTotalZ()/ 2.0 - 0.01*mm));  //真空層の文を引く
            new G4PVPlacement(G4Transform3D(rot_UROKO[i], pos_UROKO[i]), objName+"_Phys", UROKO_LogVol, World_PhysVol, false, Mode[objName].ID + i, checkOverlaps);
        }
        */
    }

    // =============================================================
    // HILE Detector (6台)
    // =============================================================
    objName = "HILE";
    if( Mode[objName].Enable ) {
        fHile = new HILELogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* Hile_LogVol = fHile->GetLogicalVolume();

        /*
        //本番用
        const G4int nObj = Mode[objName].nObj;

        // 1. 配置パラメータ（元の変数名を完全に維持）
        double rMin_HILE_Scinti = 700.0 * mm;
        double rMax_HILE_Scinti = 712.5 * mm;
        double h_HILE_Scinti    = 150.0 * mm;
        double rot_angle_HILE_1 = 27.4 * deg;
        double rot_angle_HILE_2 = 12.6 * deg;

        double x_shift_HILE = -h_HILE_Scinti/2 * std::sin(rot_angle_HILE_2) * mm ;
        double z_shift_HILE =  h_HILE_Scinti/2 * (1 + std::cos(rot_angle_HILE_2)) * mm ;
        double margin_HILE = 7.5 * mm ;

        // 2. 変換（Transform）の合成（元の変数名を完全に維持）
        G4Transform3D transform_HILE_UM =
                Rotate(Axis::Z, rot_angle_HILE_1) * G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0) * Rotate(Axis::X, 180 * deg);

        G4Transform3D transform_HILE_UL =
                Rotate(Axis::Z, rot_angle_HILE_1) * G4Translate3D(x_shift_HILE, 0, -z_shift_HILE - margin_HILE) *
                        G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0) * Rotate(Axis::Y, rot_angle_HILE_2) * Rotate(Axis::X, 180 * deg)  ;

        G4Transform3D transform_HILE_UR =
                Rotate(Axis::Z, rot_angle_HILE_1) * G4Translate3D(x_shift_HILE, 0, z_shift_HILE + margin_HILE) *
                        G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0) * Rotate(Axis::Y, -rot_angle_HILE_2) * Rotate(Axis::X, 180 * deg)  ;

        G4Transform3D transform_HILE_DM =
                Rotate(Axis::Z, -rot_angle_HILE_1) * G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0);

        G4Transform3D transform_HILE_DL =
                Rotate(Axis::Z, -rot_angle_HILE_1) * G4Translate3D(x_shift_HILE, 0, -z_shift_HILE - margin_HILE) *
                        G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0) * Rotate(Axis::Y, rot_angle_HILE_2)  ;

        G4Transform3D transform_HILE_DR =
                Rotate(Axis::Z, -rot_angle_HILE_1) * G4Translate3D(x_shift_HILE, 0, z_shift_HILE + margin_HILE) *
                        G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0) * Rotate(Axis::Y, -rot_angle_HILE_2) ;

        // 💡 3. 配列にまとめることでループ配置を可能にする
        G4Transform3D trans_HILE[6] = {
            transform_HILE_UM,
            transform_HILE_UL,
            transform_HILE_UR,
            transform_HILE_DM,
            transform_HILE_DL,
            transform_HILE_DR
        };

        // 4. 配置ループ (IDの自動付与)
        for(G4int i=0; i<nObj; i++) {
            new G4PVPlacement(trans_HILE[i], Hile_LogVol, objName+"_Phys", fWorldLogicalVolume, false, Mode[objName].ID + i, checkOverlaps);
        }
        */

        //テスト用（1台だけ配置）
        const G4int nObj = Mode[objName].nObj;
        G4double rMin_HILE_Scinti = 700.0 * mm;
        G4double rMax_HILE_Scinti = 712.5 * mm;
        G4double h_HILE_Scinti    = 150.0 * mm;

        G4Transform3D transform_HILE=
                Rotate(Axis::Z, 12.3/2 * deg) * G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0) * Rotate(Axis::X, 180 * deg);

        new G4PVPlacement(transform_HILE, Hile_LogVol, objName+"_Phys", fWorldLogicalVolume, false, Mode[objName].ID, checkOverlaps);

    }

    // =============================================================
    // HPGe Detector (7台)
    // =============================================================

    objName = "HPGe";
    if( Mode[objName].Enable){

        const int nObj = Mode[objName].nObj;

        G4String GeName[nObj]={
            "Handai55",   //Ge0 
            "SUNY_LEPS",  //Ge1
            "Kyudai80",   //Ge2 
            "SUNY_ALICE", //Ge3
            "SUNY_CINDY",  //Ge5
            "Handai60",  //GeR
            "Handai60"    //GeL

        };

        G4double AngleHPGe[nObj]={
            -135* deg,
            180 * deg,
            135 * deg,
            45 * deg,
            -45 * deg,
            0 * deg,
            180 * deg
        };

        G4double distanceHPGe[nObj]={
            78.0 * mm,
            131.0 * mm,  //Ge1(LEPS)だけ他より遠い (66+65)
            73.5 * mm,
            78.5 * mm,
            78.0 * mm,  //Ge5(CINDY)は距離がわからないので暫定
            24.8 *mm  + 20.0 * mm + 7.0 * mm, //磁石の図面から計算(Betaplasticが 2*2.5 mm + 隙間 2* 1.0 mm = 7.0 mm )
            24.8 *mm  + 20.0 * mm + 7.0 * mm

        };

        G4RotationMatrix rotHPGe[nObj];
        G4ThreeVector    vecHPGe[nObj];
        HPGeLogVol*      fHPGe[nObj];
        G4LogicalVolume* HPGe_LogVol[nObj];

        for(int i=0; i<nObj; i++){
            if(i<5){
                //円環状への配置
                rotHPGe[i].rotateX( 90 * deg );
                rotHPGe[i].rotateZ( AngleHPGe[i] );
                vecHPGe[i] = G4ThreeVector(0,0,distanceHPGe[i]);
                vecHPGe[i].rotateX( 90 * deg );
                vecHPGe[i].rotateZ( AngleHPGe[i] );
            }
            else{
                rotHPGe[i].rotateY( AngleHPGe[i] );
                vecHPGe[i] = G4ThreeVector(0,0,distanceHPGe[i]);
                vecHPGe[i].rotateY( AngleHPGe[i] );
            }
            fHPGe[i] = new HPGeLogVol(GeName[i], 0, checkOverlaps);
            HPGe_LogVol[i] = fHPGe[i]->GetLogicalVolume();
            new G4PVPlacement(G4Transform3D( rotHPGe[i], vecHPGe[i] ),
                GeName[i], HPGe_LogVol[i],
                World_PhysVol, false, Mode[objName].ID+i, checkOverlaps);
        }
    }

    objName = "Magnet";
    if( Mode[objName].Enable ) {
        MagnetLogVol* fMagnet = new MagnetLogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* Mag_LogVol = fMagnet->GetLogicalVolume();

        //そのまま配置すればOK
        new G4PVPlacement(0, G4ThreeVector(0,0,0), Mag_LogVol, objName+"_Phys", fWorldLogicalVolume, false, Mode[objName].ID, checkOverlaps);

    }

    ///////////////
    ////BetaPlastic////
    objName = "BetaPlastic";
    if( Mode[objName].Enable ) {
        /*
        BetaPlasticLogVol* fBetaPlastic = new BetaPlasticLogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* BetaPlastic_LogVol = fBetaPlastic->GetLogicalVolume();

        //仮としてその場に配置
        new G4PVPlacement(0, G4ThreeVector(0,0,0), BetaPlastic_LogVol, objName+"_Phys", fWorldLogicalVolume, false, Mode[objName].ID, checkOverlaps);
        */

        const int nObj = Mode[objName].nObj;
        BetaPlasticLogVol* fBetaPlastic = new BetaPlasticLogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* BetaPlastic_LogVol = fBetaPlastic->GetLogicalVolume();

        G4double AngleBetaPlastic[nObj]={
            -135* deg,
            180 * deg,
            135 * deg,
            45 * deg,
            0 * deg,
            -45 * deg,
            0 * deg,
            0 * deg,
            180 * deg,
            180 * deg
        };

        G4double distanceBetaPlastic[nObj]={
            78.0 * mm - 2.5/2 * mm, // Geに接触
            102.0 * mm,  // 隣接するGeとの接触を避けるため
            66.0 * mm + 2.5/2 * mm, //Magnetに接触
            78.5 * mm - 2.5/2 * mm, // Geに接触
            90.0 * mm,
            78.0 * mm - 2.5/2 * mm,  // Geに接触
            24.8 *mm  + 20.0 * mm + 1.0 * mm,
            24.8 *mm  + 20.0 * mm + 1.0 * mm + 2.5 * mm + 1.0 * mm,
            24.8 *mm  + 20.0 * mm + 1.0 * mm,
            24.8 *mm  + 20.0 * mm + 1.0 * mm + 2.5 * mm + 1.0 * mm,

        };

        G4RotationMatrix rotBetaPlastic[nObj];
        G4ThreeVector    vecBetaPlastic[nObj];

        for(int i=0; i<nObj; i++){
            if(i<6){
                //円環状への配置
                rotBetaPlastic[i].rotateX( 90 * deg );
                rotBetaPlastic[i].rotateZ( AngleBetaPlastic[i] );
                vecBetaPlastic[i] = G4ThreeVector(0,0,distanceBetaPlastic[i]);
                vecBetaPlastic[i].rotateX( 90 * deg );
                vecBetaPlastic[i].rotateZ( AngleBetaPlastic[i] );
            }
            else{
                rotBetaPlastic[i].rotateY( AngleBetaPlastic[i] );
                vecBetaPlastic[i] = G4ThreeVector(0,0,distanceBetaPlastic[i]);
                vecBetaPlastic[i].rotateY( AngleBetaPlastic[i] );
            }
            new G4PVPlacement(G4Transform3D( rotBetaPlastic[i], vecBetaPlastic[i] ),
                objName,BetaPlastic_LogVol,
                World_PhysVol, false, Mode[objName].ID+i, checkOverlaps);
        }
    }

    ///////////////
    //// Frame ////
    objName = "Frame";
    if( Mode[objName].Enable ){
        FrameLogVol* fFrame = new FrameLogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* Frame_LogVol = fFrame->GetLogicalVolume();
        G4Transform3D transform_Frame = Rotate(Axis::Y, -270*deg) ;
        new G4PVPlacement( transform_Frame, objName, Frame_LogVol, World_PhysVol, false, Mode[objName].ID, checkOverlaps);
    }

    ///////////////
    //// Shield ////

    objName = "Shield";
    if( Mode[objName].Enable ){
        const int nObj = Mode[objName].nObj;
        G4double Angle[nObj] = {
            -135*degree, -90*degree, -45*degree,
            45*degree,  90*degree, 135*degree
        };
        G4RotationMatrix rotShield[nObj];
        G4ThreeVector    vecShield[nObj];
        for(int i=0;i<Mode[objName].nObj;i++){
            rotShield[i].rotateX( Angle[i] );
            rotShield[i].rotateY( 90*degree );
            vecShield[i] = G4ThreeVector(0, 0, (i==1||i==4)?120*mm:105*mm);
            vecShield[i].rotateX( Angle[i] );
            vecShield[i].rotateY( 90*degree );
        }
        ShieldLogVol* fShield = new ShieldLogVol(objName, 0, checkOverlaps);
        G4LogicalVolume* Shield_LogVol;
        for(int i=0;i<nObj; i++){
            Shield_LogVol = fShield->GetLogicalVolume( (i==0)?1:(i==5)?2:0 );
            G4String objNamei = objName + std::to_string(i);
            new G4PVPlacement( G4Transform3D( rotShield[i], vecShield[i] ),
                objNamei, Shield_LogVol, World_PhysVol, false, Mode[objName].ID+i, checkOverlaps);
        }
    }

    ///////////////
    //// Floor ////
    objName = "Floor";
    if( Mode[objName].Enable ){
        G4double height    = 1.7*m;
        G4double thickness = 1.0*m;
        FloorLogVol* fFloor = new FloorLogVol(objName, 0, checkOverlaps, world_size, thickness, world_size);
        G4LogicalVolume* Floor_LogVol = fFloor->GetLogicalVolume();
        new G4PVPlacement( G4Transform3D( G4RotationMatrix(), G4ThreeVector( 0, -(height+0.5*thickness), 0)),
            objName, Floor_LogVol, World_PhysVol, false, Mode[objName].ID, checkOverlaps);
    }

    // =============================================================
    // Chamber / Stopper
    // =============================================================

    // StopperはChamber内に配置するため、Chamberの有効化を必須にする。
    if (Mode["Stopper"].Enable && !Mode["Chamber"].Enable) {
        G4Exception(
            "DetectorConstruction::Construct",
            "StopperRequiresChamber",
            FatalException,
            "Stopper requires Chamber to be enabled."
        );
        return nullptr;
    }

    G4LogicalVolume* chamberLogVol = nullptr;

    // Chamber：WorldにY軸まわり+90度で配置
    if (Mode["Chamber"].Enable) {
        auto* chamber = new ChamberLogVol(
            "Chamber",
            nullptr,
            checkOverlaps
        );

        chamberLogVol = chamber->GetLogicalVolume();

        const G4Transform3D chamberTransform =
            Rotate(Axis::Y, 90 * deg);

        new G4PVPlacement(
            chamberTransform,
            chamberLogVol,
            "Chamber",
            fWorldLogicalVolume,
            false,
            Mode["Chamber"].ID,
            checkOverlaps
        );
    }

    // Stopper：Chamberの真空の親ボリューム内に配置
    if (Mode["Stopper"].Enable) {
        auto* stopper = new StopperLogVol(
            "Stopper",
            nullptr,
            checkOverlaps
        );

        G4LogicalVolume* stopperLogVol =
            stopper->GetLogicalVolume();

        // Chamberの+90度回転を相殺する。
        // StopperのSolid内にある+45度の傾きはWorldでも維持される。
        const G4Transform3D stopperTransform =
            Rotate(Axis::Y, -90 * deg);

        new G4PVPlacement(
            stopperTransform,
            stopperLogVol,
            "Stopper",
            chamberLogVol,
            false,
            Mode["Stopper"].ID,
            checkOverlaps
        );
    }

    return World_PhysVol;

}

void DetectorConstruction::ConstructSDandField()
{
    auto* sdManager =
        G4SDManager::GetSDMpointer();

    if (sdManager == nullptr) {
        G4Exception(
            "DetectorConstruction::"
            "ConstructSDandField",
            "DetectorConstruction002",
            FatalException,
            "G4SDManager is null."
        );

        return;
    }

    auto* scintillatorSD =
        new ScintillatorSD(
            "ScintillatorSD",
            fAnalysisConfig
        );

    auto* photocathodeSD =
        new PhotocathodeSD(
            "PhotocathodeSD",
            fAnalysisConfig
        );

    sdManager->AddNewDetector(
        scintillatorSD
    );

    sdManager->AddNewDetector(
        photocathodeSD
    );

    /*
     * LiGlass
     */
    if (Mode["LiGlass"].Enable &&
        fLiGlass != nullptr) {

        fLiGlass
            ->GetScintiVolume()
            ->SetSensitiveDetector(
                scintillatorSD
            );

        fLiGlass
            ->GetCathodeVolume()
            ->SetSensitiveDetector(
                photocathodeSD
            );

        const DetectorKey detectorKey{
            Mode["LiGlass"].ID
        };

        /*
         * LiGlassのPMTは1チャンネル。
         */
        photocathodeSD->RegisterChannel(
            PmtChannelKey{
                detectorKey,
                0
            }
        );
    }

    /*
     * UROKO
     */
    if (Mode["UROKO"].Enable &&
        fUROKO != nullptr) {

        fUROKO
            ->GetScintiVolume()
            ->SetSensitiveDetector(
                scintillatorSD
            );

        fUROKO
            ->GetCathodeVolume()
            ->SetSensitiveDetector(
                photocathodeSD
            );

        for (G4int index = 0;
             index < Mode["UROKO"].nObj;
             ++index) {

            const DetectorKey detectorKey{
                Mode["UROKO"].ID + index
            };

            /*
             * UROKOは検出器1台につき
             * PMTチャンネル0と1を持つ。
             */
            photocathodeSD->RegisterChannel(
                PmtChannelKey{
                    detectorKey,
                    0
                }
            );

            photocathodeSD->RegisterChannel(
                PmtChannelKey{
                    detectorKey,
                    1
                }
            );
        }
    }

    /*
     * HILEは現時点では新しい解析出力の対象外。
     */
}
