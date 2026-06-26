#include "DetectorConstruction.hh"
#include "LogVol/LigLogVol.hh"
#include "LogVol/UrokoLogVol.hh"
#include "LogVol/HILELogVol.hh"
#include "LogVol/HPGeLogVol.hh"
#include "LogVol/BetaPlasticLogVol.hh"
#include "LogVol/MagnetLogVol.hh"
#include "LogVol/FrameLogVol.hh"
#include "LogVol/FloorLogVol.hh"
#include "LogVol/ShieldLogVol.hh"
#include "LogVol/ChamberLogVol.hh"
#include "LogVol/StopperLogVol.hh"



#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4SDManager.hh"
#include "SensitiveDetector.hh"

#include <map>

// ======================================================================
// マスターコントロールパネル (検出器のON/OFFとID管理)
// ======================================================================
struct EnableAndID { G4bool Enable; G4int ID, nObj; };
/*
//本番用
std::map<G4String, EnableAndID> Mode = {
//  NAME         ENABLE   ID0   nObj
  {"LigGlass",  { false,    1,    1 } },
  {"Uroko",     { false,   10,    4 } },
  {"HILE",      { false,   20,    6 } },
  {"HPGe",      { false,   30,    7 } },
  {"BetaPlastic", { false,   40,    1 } },
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
//  NAME         ENABLE   ID0   nObj
  {"LigGlass",  { false,    1,    1 } },
  {"Uroko",     { true,   10,    1 } },
  {"HILE",      { false,   20,    1 } },
  {"HPGe",      { false,   30,    7 } },
  {"BetaPlastic", { false,   40,    1 } },
  {"Magnet",     { false,   50,    1 } },
  { "Frame",     { false,   60,    1 } },
  { "Floor",     { false,   70,    1 } },
  { "Shield",    { false,   80,    6} },
  { "Chamber",   { false,   90,    1 } },
  { "Stopper",   { false,   100,    1 } },
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

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(), fLig(nullptr), fUroko(nullptr), fHile(nullptr), World_LogVol(nullptr)
{
}

DetectorConstruction::~DetectorConstruction()
{}

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
  World_LogVol = new G4LogicalVolume(World_Solid, matAir, "World_LogVol");
  World_LogVol->SetVisAttributes(new G4VisAttributes(TRUE, G4Colour(1.0, 1.0, 1.0, 0.0)));
  G4VPhysicalVolume* World_PhysVol = new G4PVPlacement(0, G4ThreeVector(), World_LogVol, "World_PhysVol", 0, false, 0, checkOverlaps);

  G4String objName;

  // =============================================================
  // LigGlass Detector 
  // =============================================================
  objName = "LigGlass";
  if( Mode[objName].Enable ) {
    fLig = new LigLogVol(objName, 0, checkOverlaps);
    G4LogicalVolume* Lig_LogVol = fLig->GetLogicalVolume();

    G4double front_Lig = -132.0 * mm;
    G4double total_length_Lig = (28.5 + 246.0) * mm;
    
    G4RotationMatrix rot_Lig;
    rot_Lig.rotateX(-90.0 * deg);
    G4ThreeVector pos_Lig(0.0, front_Lig - (total_length_Lig / 2.0), 0.0);
    new G4PVPlacement(G4Transform3D(rot_Lig, pos_Lig), objName+"_Phys", Lig_LogVol, World_PhysVol, false, Mode[objName].ID, checkOverlaps);
  }

  // =============================================================
  // UROKO Detector (4台)
  // =============================================================
  objName = "Uroko";
  if( Mode[objName].Enable ) {
    fUroko = new UrokoLogVol(objName, 0, checkOverlaps);
    G4LogicalVolume* Uroko_LogVol = fUroko->GetLogicalVolume();

    /*
    //本番用
    const G4int nObj = Mode[objName].nObj;
    G4double updownDeg = 5.7 * deg;
    G4double sideDeg = 10.2 * deg;
    G4double margin_Uroko = 2.5 * mm;
    G4double hexagon_r = 100.0 * mm;
    G4double hexagon_rr = hexagon_r * std::sqrt(3.0) / 2.0;
    G4double total_Z_Uroko = 111.5 * mm; 

    G4double swing_updown = (total_Z_Uroko / 2.0) * std::sin(updownDeg);
    G4double swing_side   = (total_Z_Uroko / 2.0) * std::sin(sideDeg);

    G4ThreeVector pos_Uroko[4] = {
        G4ThreeVector(1000*mm,  (hexagon_rr + margin_Uroko + swing_updown), 0),
        G4ThreeVector(1000*mm, -(hexagon_rr + margin_Uroko + swing_updown), 0),
        G4ThreeVector(1000*mm, 0,  (1.5 * hexagon_r + margin_Uroko + swing_side)),
        G4ThreeVector(1000*mm, 0, -(1.5 * hexagon_r + margin_Uroko + swing_side))
    };

    G4RotationMatrix rot_Uroko[4];
    rot_Uroko[0].rotateY(90.0*deg); rot_Uroko[0].rotateZ(updownDeg);
    rot_Uroko[1].rotateY(90.0*deg); rot_Uroko[1].rotateZ(-updownDeg);
    rot_Uroko[2].rotateY(90.0*deg - sideDeg);
    rot_Uroko[3].rotateY(90.0*deg + sideDeg);

    for(G4int i=0; i<nObj; i++) {
      pos_Uroko[i].setX(pos_Uroko[i].getX() + (total_Z_Uroko / 2.0)); 
      new G4PVPlacement(G4Transform3D(rot_Uroko[i], pos_Uroko[i]), objName+"_Phys", Uroko_LogVol, World_PhysVol, false, Mode[objName].ID + i, checkOverlaps);
    }
    */

    //テスト用（1台だけ配置）
    const G4int nObj = Mode[objName].nObj;
    //ターゲット検出器間の距離は 1000 mm
    //G4ThreeVector pos_Uroko[1] = { G4ThreeVector(1000*mm, 0, 0) };

    //線源検出器間の距離は 10 mmと仮定（実験室では線源をベタ付している）
    G4ThreeVector pos_Uroko[1] = { G4ThreeVector(10.0*mm, 0, 0) };

    G4RotationMatrix rot_Uroko[1];
    rot_Uroko[0].rotateY(90.0*deg);
    G4double total_Z_Uroko = 111.5 * mm; 

    for(G4int i=0; i<nObj; i++) {
      pos_Uroko[i].setX(pos_Uroko[i].getX() + (total_Z_Uroko / 2.0)); 
      new G4PVPlacement(G4Transform3D(rot_Uroko[i], pos_Uroko[i]), objName+"_Phys", Uroko_LogVol, World_PhysVol, false, Mode[objName].ID + i, checkOverlaps);
    }
    
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
      new G4PVPlacement(trans_HILE[i], Hile_LogVol, objName+"_Phys", World_LogVol, false, Mode[objName].ID + i, checkOverlaps);
    }
    */

    //テスト用（1台だけ配置）
    const G4int nObj = Mode[objName].nObj;
    G4double rMin_HILE_Scinti = 700.0 * mm;
    G4double rMax_HILE_Scinti = 712.5 * mm;
    G4double h_HILE_Scinti    = 150.0 * mm;

    G4Transform3D transform_HILE= 
        Rotate(Axis::Z, 12.3/2 * deg) * G4Translate3D((rMin_HILE_Scinti + rMax_HILE_Scinti)/2, 0, 0) * Rotate(Axis::X, 180 * deg);

    new G4PVPlacement(transform_HILE, Hile_LogVol, objName+"_Phys", World_LogVol, false, Mode[objName].ID, checkOverlaps);
    
  }

  




  // =============================================================
  // HPGe Detector (7台)
  // =============================================================

  objName = "HPGe";
  if( Mode[objName].Enable){

    const int nObj = Mode[objName].nObj;

    G4String GeName[nObj]={
      "Handai55",   //Ge0 (実際は55%)
      "SUNY_LEPS",  //Ge1
      "Kyudai80",   //Ge2 (実際はKyudai80%)
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
      88.0 * mm,
      131.0 * mm,  //Ge1(LEPS)だけ他より遠い (66+65)
      73.5 * mm,
      78.5 * mm,
      78.0 * mm,  //Ge5(CINDY)は距離がわからないので暫定
      44.8 * mm, //磁石の図面から計算
      44.8 * mm   //磁石の図面から計算

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
    new G4PVPlacement(0, G4ThreeVector(0,0,0), Mag_LogVol, objName+"_Phys", World_LogVol, false, Mode[objName].ID, checkOverlaps);

  }


  ///////////////
  ////BetaPlastic////
  objName = "BetaPlastic";
  if( Mode[objName].Enable ) {
    BetaPlasticLogVol* fBetaPlastic = new BetaPlasticLogVol(objName, 0, checkOverlaps);
    G4LogicalVolume* BetaPlastic_LogVol = fBetaPlastic->GetLogicalVolume();

    //仮としてその場に配置
    new G4PVPlacement(0, G4ThreeVector(0,0,0), BetaPlastic_LogVol, objName+"_Phys", World_LogVol, false, Mode[objName].ID, checkOverlaps);
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

  /////////////////
  //// Chamber ////
  objName = "Chamber";
  if( Mode[objName].Enable ){
    ChamberLogVol* fChamber = new ChamberLogVol(objName, 0, checkOverlaps);
    G4LogicalVolume* Chamber_LogVol = fChamber->GetLogicalVolume();
    G4Transform3D transform_Chamber = Rotate(Axis::Y, 90*deg) ;
    new G4PVPlacement( transform_Chamber, objName, Chamber_LogVol, World_PhysVol, false, Mode[objName].ID, checkOverlaps);
  }


  /////////////////
  //// Stopper ////
  objName = "Stopper";
  if( Mode[objName].Enable ){
    StopperLogVol* fStopper = new StopperLogVol(objName, 0, checkOverlaps);
    G4LogicalVolume* Stopper_LogVol = fStopper->GetLogicalVolume();
    // 既に斜め45度に配置
    new G4PVPlacement( G4Transform3D(), objName, Stopper_LogVol, World_PhysVol, false, Mode[objName].ID, checkOverlaps);
  }


  return World_PhysVol;

}


void DetectorConstruction::ConstructSDandField()
{
  SensitiveDetector* sensDet = new SensitiveDetector("SensitiveDetector");
  G4SDManager::GetSDMpointer()->AddNewDetector(sensDet);

  //注意：現状のコードでは各検出器を一台ずつテストすることしか想定していない
  
  if( Mode["LigGlass"].Enable && fLig ) {
    fLig->GetScintiVolume()->SetSensitiveDetector(sensDet);
  }
  
  if( Mode["Uroko"].Enable && fUroko ) {
    fUroko->GetScintiVolume()->SetSensitiveDetector(sensDet);
  }

  if( Mode["HILE"].Enable && fHile ) {
    fHile->GetScintiVolume()->SetSensitiveDetector(sensDet);
    //fHile->GetCathodeVolume()->SetSensitiveDetector(sensDet);
  }
}