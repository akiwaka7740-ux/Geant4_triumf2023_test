#include "LogVol/UrokoLogVol.hh"
#include "Material/BC408Mat.hh"

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

UrokoLogVol::UrokoLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  // =============================================================
  // 1. 寸法と変数の定義
  // =============================================================
  double thickness_Uroko = 29.0 * mm;
  double PMT_W_Uroko = 26.2 * mm;
  double PMT_L_Uroko = 32.5 * mm;
  double PMT_C_Uroko = 30.0 * mm;
  double cathode_W_Uroko = 23.0 * mm;
  double cathode_T_Uroko = 0.8 * mm;
  double guide_L_Uroko = 40.0 * mm;
  double guide_S_Uroko = 10.0 * mm;
  double hexagon_r_Uroko = 100.0 * mm;
  double hexagon_rr_Uroko = hexagon_r_Uroko * std::sqrt(3.0) / 2.0;

  // UROKO全体の長さ（Z軸方向のスペース）を計算
  double total_Z = thickness_Uroko + guide_L_Uroko + guide_S_Uroko + PMT_L_Uroko;

  // =============================================================
  // 2. Solid の作成
  // =============================================================
  
  // (A) Mother Volume（全体を包み込む真空の箱）
  // 余裕を持たせて X,Y は120mmのハーフサイズ、Z は全長の半分
  Solid = new G4Box(Name+"_Solid", 120.0*mm, 120.0*mm, total_Z / 2.0);

  // (B) Scintillator (六角柱)
  double z1_Uroko[] = {0.0, thickness_Uroko};
  double rI1_Uroko[] = {0.0, 0.0};
  double rO1_Uroko[] = {hexagon_rr_Uroko, hexagon_rr_Uroko};
  G4VSolid* solid_Scinti = new G4Polyhedra(Name+"_ScintiSolid", 0*deg, 360*deg, 6, 2, z1_Uroko, rI1_Uroko, rO1_Uroko);

  // (C) Light Guide
  double PMT_W2_Uroko = (PMT_W_Uroko + ((PMT_C_Uroko - PMT_W_Uroko) / 2.0)) * 2.0;
  double z2_Uroko[] = {-guide_L_Uroko / 2.0, guide_L_Uroko / 2.0};
  double rO2_Uroko[] = {hexagon_rr_Uroko, (PMT_W2_Uroko * std::sqrt(3.0) + PMT_W_Uroko) / 4.0};

  G4Trd* solid_Guide1 = new G4Trd(Name+"_Guide1", hexagon_r_Uroko, PMT_W2_Uroko / 2.0, hexagon_rr_Uroko, PMT_W_Uroko / 2.0, guide_L_Uroko / 2.0);
  G4Polyhedra* solid_Guide2 = new G4Polyhedra(Name+"_Guide2", 0*deg, 360*deg, 6, 2, z2_Uroko, rI1_Uroko, rO2_Uroko);
  G4IntersectionSolid* solid_Guide3 = new G4IntersectionSolid(Name+"_Guide3", solid_Guide1, solid_Guide2);

  G4Box* solid_Guide4 = new G4Box(Name+"_Guide4", PMT_W2_Uroko / 2.0, PMT_W_Uroko / 2.0, guide_S_Uroko / 2.0);
  G4ThreeVector pos_guide_box(0, 0, (guide_L_Uroko / 2.0) - guide_S_Uroko + (thickness_Uroko / 2.0));
  G4UnionSolid* solid_Guide = new G4UnionSolid(Name+"_GuideSolid", solid_Guide3, solid_Guide4, G4Transform3D(G4RotationMatrix(), pos_guide_box));

  // (D) PMT & Cathode
  G4Box* solid_PMT = new G4Box(Name+"_PMTSolid", PMT_W_Uroko / 2.0, PMT_W_Uroko / 2.0, PMT_L_Uroko / 2.0);
  G4Box* solid_Cathode = new G4Box(Name+"_CathodeSolid", cathode_W_Uroko / 2.0, cathode_W_Uroko / 2.0, cathode_T_Uroko / 2.0);

  // =============================================================
  // 3. Material と Logical Volume
  // =============================================================
  BC408Mat* fBC408 = new BC408Mat();
  auto matVacuum  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  auto matBC408   = fBC408->GetMaterial();
  auto matAcrylic = G4NistManager::Instance()->FindOrBuildMaterial("G4_PLEXIGLASS");
  auto matGlass   = G4NistManager::Instance()->FindOrBuildMaterial("G4_Pyrex_Glass");
  auto matAl      = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");

  // Mother Volume (透明)
  LogVol = new G4LogicalVolume(Solid, matVacuum, Name+"_LogVol", 0, 0, fStepLimit, false);
  LogVol->SetVisAttributes(new G4VisAttributes(TRUE, Color(ColID::White, 0.0))); 

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
  // 4. Physical Volume (内部配置)
  // =============================================================
  
  // Z軸上の相対位置を計算 (Mother Volumeの中心が z=0)
  // Scintillatorの前面を Mother Volume の最前面 (-total_Z/2) に合わせる
  double z_scinti = -total_Z / 2.0;
  new G4PVPlacement(Move(0, 0, z_scinti), LogVol_Scinti, "Scinti", LogVol, false, 0, checkOverlaps);

  double z_guide = z_scinti + thickness_Uroko + (guide_L_Uroko / 2.0);
  new G4PVPlacement(Move(0, 0, z_guide), LogVol_Guide, "Guide", LogVol, false, 0, checkOverlaps);

  // PMT は並列に2つ配置
  double z_pmt = z_scinti + thickness_Uroko - 1.0*mm + guide_L_Uroko + guide_S_Uroko + (PMT_L_Uroko / 2.0);
  new G4PVPlacement(Move(-PMT_C_Uroko / 2.0, 0, z_pmt), LogVol_PMT, "PMT1", LogVol, false, 1, checkOverlaps);
  new G4PVPlacement(Move( PMT_C_Uroko / 2.0, 0, z_pmt), LogVol_PMT, "PMT2", LogVol, false, 2, checkOverlaps);

  // Cathode は PMT の「内部」に配置 (※配置先が LogVol ではなく LogVol_PMT になるのがポイント)
  double z_cathode = -(PMT_L_Uroko / 2.0) + 3.0 * (cathode_T_Uroko / 2.0);
  new G4PVPlacement(Move(0, 0, z_cathode), LogVol_Cathode, "Cathode", LogVol_PMT, false, 0, checkOverlaps);
}

UrokoLogVol::~UrokoLogVol() 
{
  delete Solid;
  delete LogVol;
}