#include "LogVol/UROKOLogVol.hh"

#include "Material/BC408Mat.hh"
#include "Material/PMTGlassMat.hh"
#include "Material/AirMat.hh"
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
#include "G4SubtractionSolid.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"
#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"

using namespace CLHEP;

namespace {
  enum Axis { X, Y, Z };
  G4Transform3D Rotate(int axis, double angle) {
    G4RotationMatrix rot;
    if( axis==Axis::X ) rot.rotateX( angle );
    if( axis==Axis::Y ) rot.rotateY( angle );
    if( axis==Axis::Z ) rot.rotateZ( angle );
    return G4Transform3D( rot, G4ThreeVector( 0,0,0 ) );
  }
  G4Transform3D Move(double dx, double dy, double dz) {
    return G4Transform3D( G4RotationMatrix(), G4ThreeVector( dx,dy,dz ) );
  }

  enum class ColID {
    White, Gray, Grey, Black, Brown, Red, Green, Blue, Cyan, Magenta, Yellow, DarkGrey,
  };

  struct RGB { double R; double G; double B; };
  static std::map<ColID, RGB> defaultRGB = {
    { ColID::White,    { 1.0, 1.0, 1.0 } },
    { ColID::Gray,     { 0.5, 0.5, 0.5 } },
    { ColID::Black,    { 0.0, 0.0, 0.0 } },
    { ColID::Blue,     { 0.0, 0.0, 1.0 } },
    { ColID::Magenta,  { 1.0, 0.0, 1.0 } },
    { ColID::Yellow ,  { 1.0, 1.0, 0.0 } },
  };

  G4Colour Color(ColID ID, double Opacity=1) {
    return G4Colour( defaultRGB[ID].R, defaultRGB[ID].G, defaultRGB[ID].B, Opacity );
  }
};

UROKOLogVol::UROKOLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  // =============================================================
  // 1. 寸法と変数の定義
  // =============================================================
  double bumper_T = 0.01 * mm; //検出器前の極薄真空層（光学パラメータを適切に設定するために必要）
  double thickness_UROKO = 29.0 * mm;
  double PMT_W_UROKO = 26.2 * mm;
  double PMT_L_UROKO = 32.5 * mm;
  double PMT_C_UROKO = 30.0 * mm;
  double cathode_W_UROKO = 23.0 * mm;
  double cathode_T_UROKO = 0.8 * mm;
  double guide_L_UROKO = 40.0 * mm;
  double guide_S_UROKO = 10.0 * mm;
  double hexagon_r_UROKO = 100.0 * mm;
  double hexagon_rr_UROKO = hexagon_r_UROKO * std::sqrt(3.0) / 2.0;

  // UROKO全体の長さ（Z軸方向のスペース）を計算
  total_Z = bumper_T + thickness_UROKO + guide_L_UROKO + guide_S_UROKO + PMT_L_UROKO;

  // =============================================================
  // 2. Solid の作成
  // =============================================================
  
  // (A) Mother Volume（全体を包み込む真空の箱）
  // 余裕を持たせて X,Y は120mmのハーフサイズ、Z は全長の半分
  Solid = new G4Box(Name+"_Solid", 120.0*mm, 120.0*mm, total_Z / 2.0);

  // (B) Scintillator (六角柱)　＋　真空層
  double z_bump[] = {0.0, bumper_T};
  double z1_UROKO[] = {0.0, thickness_UROKO};
  double rI1_UROKO[] = {0.0, 0.0};
  double rO1_UROKO[] = {hexagon_rr_UROKO, hexagon_rr_UROKO};
  G4VSolid* solid_Bumper = new G4Polyhedra(Name+"_BumperSolid", 0*deg, 360*deg, 6, 2, z_bump, rI1_UROKO, rO1_UROKO); //真空層
  G4VSolid* solid_Scinti = new G4Polyhedra(Name+"_ScintiSolid", 0*deg, 360*deg, 6, 2, z1_UROKO, rI1_UROKO, rO1_UROKO);

  // (C) Light Guide
  double PMT_W2_UROKO = (PMT_W_UROKO + ((PMT_C_UROKO - PMT_W_UROKO) / 2.0)) * 2.0;
  double z2_UROKO[] = {-guide_L_UROKO / 2.0, guide_L_UROKO / 2.0};
  double rO2_UROKO[] = {hexagon_rr_UROKO, (PMT_W2_UROKO * std::sqrt(3.0) + PMT_W_UROKO) / 4.0};

  G4Trd* solid_Guide1 = new G4Trd(Name+"_Guide1", hexagon_r_UROKO, PMT_W2_UROKO / 2.0, hexagon_rr_UROKO, PMT_W_UROKO / 2.0, guide_L_UROKO / 2.0);
  G4Polyhedra* solid_Guide2 = new G4Polyhedra(Name+"_Guide2", 0*deg, 360*deg, 6, 2, z2_UROKO, rI1_UROKO, rO2_UROKO);
  G4IntersectionSolid* solid_Guide3 = new G4IntersectionSolid(Name+"_Guide3", solid_Guide1, solid_Guide2);

  G4Box* solid_Guide4 = new G4Box(Name+"_Guide4", PMT_W2_UROKO / 2.0, PMT_W_UROKO / 2.0, guide_S_UROKO / 2.0);
  G4ThreeVector pos_guide_box(0, 0, (guide_L_UROKO / 2.0) - guide_S_UROKO + (thickness_UROKO / 2.0));
  G4UnionSolid* solid_Guide = new G4UnionSolid(Name+"_GuideSolid", solid_Guide3, solid_Guide4, G4Transform3D(G4RotationMatrix(), pos_guide_box));

  // (D) PMT & Cathode
  G4Box* solid_PMT = new G4Box(Name+"_PMTSolid", PMT_W_UROKO / 2.0, PMT_W_UROKO / 2.0, PMT_L_UROKO / 2.0);
  G4Box* solid_Cathode = new G4Box(Name+"_CathodeSolid", cathode_W_UROKO / 2.0, cathode_W_UROKO / 2.0, cathode_T_UROKO / 2.0);

  // =============================================================
  // 3. Material と Logical Volume
  // =============================================================
  VacuumMat* fVacuum = new VacuumMat();
  BC408Mat* fBC408 = new BC408Mat();
  PMTGlassMat* fPMTGlass = new PMTGlassMat();
  AcrylicMat* fAcrylic = new AcrylicMat();

  auto matVacuum  = fVacuum->GetMaterial();
  auto matBC408   = fBC408->GetMaterial();
  auto matGlass   = fPMTGlass->GetMaterial();
  auto matAcrylic = fAcrylic->GetMaterial();
  //現時点ではAl（光電面）までの過程を考慮
  auto matAl      = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");

  // Mother Volume (透明)
  LogVol = new G4LogicalVolume(Solid, matVacuum, Name+"_LogVol", 0, 0, fStepLimit, false);
  LogVol->SetVisAttributes(new G4VisAttributes(TRUE, Color(ColID::White, 0.0))); 

  //真空層（非表示）
  G4LogicalVolume* LogVol_Bumper = new G4LogicalVolume(solid_Bumper, matVacuum, Name+"_LV_Bumper", 0, 0, fStepLimit, false);
  LogVol_Bumper->SetVisAttributes(new G4VisAttributes(false));

  // 子ボリューム群（塗りつぶし設定込み）
  LogVol_Scinti = new G4LogicalVolume(solid_Scinti, matBC408, Name+"_LV_Scinti", 0, 0, fStepLimit, false);
  G4VisAttributes* visScinti = new G4VisAttributes(TRUE, Color(ColID::Blue, 0.5));
  visScinti->SetForceSolid(true);
  LogVol_Scinti->SetVisAttributes(visScinti);

  G4LogicalVolume* LogVol_Guide = new G4LogicalVolume(solid_Guide, matAcrylic, Name+"_LV_Guide", 0, 0, fStepLimit, false);
  G4VisAttributes* visGuide = new G4VisAttributes(TRUE, Color(ColID::Gray, 0.4));
  visGuide->SetForceSolid(true);
  LogVol_Guide->SetVisAttributes(visGuide);

  // PMTは1つだけ作成して、後で2回使い回します
  G4LogicalVolume* LogVol_PMT = new G4LogicalVolume(solid_PMT, matGlass, Name+"_LV_PMT", 0, 0, fStepLimit, false);
  G4VisAttributes* visPMT = new G4VisAttributes(TRUE, Color(ColID::Magenta, 0.8));
  visPMT->SetForceSolid(true);
  LogVol_PMT->SetVisAttributes(visPMT);

  G4LogicalVolume* LogVol_Cathode = new G4LogicalVolume(solid_Cathode, matAl, Name+"_LV_Cathode", 0, 0, fStepLimit, false);
  G4VisAttributes* visCathode = new G4VisAttributes(TRUE, Color(ColID::Yellow, 1.0));
  visCathode->SetForceSolid(true);
  LogVol_Cathode->SetVisAttributes(visCathode);

  // =============================================================
  // 4. Physical Volume (内部配置：バンパーを最前面に置き、他を0.01mm後ろにずらす)
  // =============================================================
  double z_bump_pos = -total_Z / 2.0;
  G4VPhysicalVolume* physBumper = new G4PVPlacement(Move(0, 0, z_bump_pos), LogVol_Bumper, "Bumper", LogVol, false, 0, checkOverlaps);

  double z_Scinti = z_bump_pos + bumper_T;
  G4VPhysicalVolume* physScinti = new G4PVPlacement(Move(0, 0, z_Scinti), LogVol_Scinti, "Scinti", LogVol, false, 0, checkOverlaps);

  double z_guide = z_Scinti + thickness_UROKO + (guide_L_UROKO / 2.0);
  G4VPhysicalVolume* physGuide = new G4PVPlacement(Move(0, 0, z_guide), LogVol_Guide, "Guide", LogVol, false, 0, checkOverlaps);

  // PMT は並列に2つ配置
  double z_pmt = z_Scinti + thickness_UROKO - 1.0*mm + guide_L_UROKO + guide_S_UROKO + (PMT_L_UROKO / 2.0);
  G4VPhysicalVolume* physPMT1 = new G4PVPlacement(Move(-PMT_C_UROKO / 2.0, 0, z_pmt), LogVol_PMT, "PMT1", LogVol, false, 1, checkOverlaps);
  G4VPhysicalVolume* physPMT2 = new G4PVPlacement(Move( PMT_C_UROKO / 2.0, 0, z_pmt), LogVol_PMT, "PMT2", LogVol, false, 2, checkOverlaps);

  // Cathode は PMT の「内部」に配置 (※配置先が LogVol ではなく LogVol_PMT になるのがポイント)
  double z_cathode = -(PMT_L_UROKO / 2.0) + 3.0 * (cathode_T_UROKO / 2.0);
  new G4PVPlacement(Move(0, 0, z_cathode), LogVol_Cathode, "Cathode", LogVol_PMT, false, 0, checkOverlaps);

  // =============================================================
  // 5. Optical Surface (光学パラメータの設定＝テープを巻き、接合面にグリースを塗る)
  // =============================================================

  G4OpticalSurface* surfCathode = (new CathodeSurface())->GetSurface();
  G4OpticalSurface* surfDiel = (new DielectricSurface())->GetSurface();
  G4OpticalSurface* surfMirror = (new MirrorSurface())->GetSurface(); //鏡面反射
  G4OpticalSurface* surfDiffuse = (new DiffuseSurface())->GetSurface(); //乱反射


  //[SkinSurface] 全体コーティング
  new G4LogicalSkinSurface("ScintiSkin", LogVol_Scinti, surfMirror);
  new G4LogicalSkinSurface("GuideSkin", LogVol_Guide, surfMirror);
  new G4LogicalSkinSurface("CathodeSkin", LogVol_Cathode, surfCathode);

  //[BorderSurface] 接合面の上書き
  // ①【前面】シンチからバンパーへ進む光だけ、鏡面スキンを「乱反射」で上書き
  new G4LogicalBorderSurface("ScintiToBumper", physScinti, physBumper, surfDiffuse);

  // ②【後面】シンチ ⇄ ライトガイド間の光は、スキンを「グリス」で上書き（双方向）
  new G4LogicalBorderSurface("ScintiToGuide", physScinti, physGuide, surfDiel);
  new G4LogicalBorderSurface("GuideToScinti", physGuide, physScinti, surfDiel);

  // ③【出口】ライトガイド ⇄ PMTガラス間の光も「グリス」で上書き（双方向）
  new G4LogicalBorderSurface("GuideToPMT1", physGuide, physPMT1, surfDiel);
  new G4LogicalBorderSurface("PMT1ToGuide", physPMT1, physGuide, surfDiel);

  new G4LogicalBorderSurface("GuideToPMT2", physGuide, physPMT2, surfDiel);
  new G4LogicalBorderSurface("PMT2ToGuide", physPMT2, physGuide, surfDiel);

}

UROKOLogVol::~UROKOLogVol() 
{
  delete Solid;
  delete LogVol;
}
