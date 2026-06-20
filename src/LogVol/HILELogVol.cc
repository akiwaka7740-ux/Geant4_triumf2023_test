// util/common.hh がある場合はインクルード
// #include "util/common.hh" 

#include "LogVol/HILELogVol.hh"
#include "Material/BC408Mat.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4Trd.hh"
#include "G4UnionSolid.hh"
#include "G4IntersectionSolid.hh"
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
}

namespace {
  enum class ColID { White, Gray, Blue, Magenta, Yellow };
  struct RGB { double R; double G; double B; };
  static std::map<ColID, RGB> defaultRGB = {
    { ColID::White,   { 1.0, 1.0, 1.0 } },
    { ColID::Gray,    { 0.5, 0.5, 0.5 } },
    { ColID::Blue,    { 0.0, 0.0, 1.0 } },
    { ColID::Magenta, { 1.0, 0.0, 1.0 } },
    { ColID::Yellow,  { 1.0, 1.0, 0.0 } }
  };
  G4Colour Color(ColID ID, double Opacity=1) {
    return G4Colour( defaultRGB[ID].R, defaultRGB[ID].G, defaultRGB[ID].B, Opacity );
  }
};

HILELogVol::HILELogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  // =============================================================
  // 1. 寸法パラメータの定義
  // =============================================================
  
  // Scintillator (プラスチック)
  double rMin_Scinti = 700.0 * mm;
  double rMax_Scinti = 712.5 * mm;
  double h_Scinti    = 150.0 * mm;
  double dPhi_Scinti = 12.3 * deg;
  //double sPhi_Scinti = -dPhi_Scinti / 2.0;

  // Light Guide (アクリル)
  double h_GuideTaper = 45.0 * mm;
  double h_GuideCyl   = 5.0 * mm;

  // PMT (ガラス) と Cathode (アルミ)
  double d_PMT     = 60.0 * mm;
  double h_PMT     = 215.0 * mm;
  double t_Cathode = 1.0 * mm;


  double box_half_Z = 350.0 * mm;

  // =============================================================
  // 2. Solid の作成
  // =============================================================
  
  // (A) Mother Volume (余裕を持たせた真空の箱)
  Solid = new G4Box(Name+"_Solid", 50.0*mm, 100.0*mm, box_half_Z);

  // (B) Scintillator (湾曲板)
  G4Tubs* solid_Scinti = new G4Tubs(Name+"_ScintiSolid", rMin_Scinti, rMax_Scinti, h_Scinti/2.0, 0, dPhi_Scinti);

  // (C) Light Guide (テーパー ＋ 上部円柱)
  G4Cons* solid_Cone = new G4Cons(Name+"_Cone", 0, 150.0/2.0*mm, 0, 50.0/2.0*mm, h_GuideTaper/2.0, 0, 360*deg);
  G4Trd* solid_FlatCut = new G4Trd(Name+"_FlatCut", 80.0*mm, 80.0*mm, 12.5/2.0*mm, 50.0/2.0*mm, h_GuideTaper/2.0);
  G4IntersectionSolid* solid_Taper = new G4IntersectionSolid(Name+"_Taper", solid_Cone, solid_FlatCut);
  
  G4Tubs* solid_TopCyl = new G4Tubs(Name+"_TopCyl", 0, 50.0/2.0*mm, h_GuideCyl/2.0, 0, 360*deg);
  G4UnionSolid* solid_Guide = new G4UnionSolid(Name+"_GuideSolid", solid_Taper, solid_TopCyl, 0, G4ThreeVector(0, 0, h_GuideTaper/2.0 + h_GuideCyl/2.0));

  // (D) PMT & Cathode
  G4Tubs* solid_PMT = new G4Tubs(Name+"_PMTSolid", 0, d_PMT/2.0, h_PMT/2.0, 0, 360*deg);
  G4Tubs* solid_Cathode = new G4Tubs(Name+"_CathodeSolid", 0, (d_PMT/2.0)*0.95, t_Cathode/2.0, 0, 360*deg);


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

  // Scintillator
  LogVol_Scinti = new G4LogicalVolume(solid_Scinti, matBC408, Name+"_LV_Scinti", 0, 0, fStepLimit, false);
  G4VisAttributes* visScinti = new G4VisAttributes(TRUE, Color(ColID::Blue, 0.5));
  visScinti->SetForceSolid(true);
  LogVol_Scinti->SetVisAttributes(visScinti);

  // Light Guide
  G4LogicalVolume* LogVol_Guide = new G4LogicalVolume(solid_Guide, matAcrylic, Name+"_LV_Guide", 0, 0, fStepLimit, false);
  G4VisAttributes* visGuide = new G4VisAttributes(TRUE, Color(ColID::Gray, 0.4));
  visGuide->SetForceSolid(true);
  LogVol_Guide->SetVisAttributes(visGuide);

  // PMT
  G4LogicalVolume* LogVol_PMT = new G4LogicalVolume(solid_PMT, matGlass, Name+"_LV_PMT", 0, 0, fStepLimit, false);
  G4VisAttributes* visPMT = new G4VisAttributes(TRUE, Color(ColID::Magenta, 0.8));
  visPMT->SetForceSolid(true);
  LogVol_PMT->SetVisAttributes(visPMT);

  // Cathode
  LogVol_Cathode = new G4LogicalVolume(solid_Cathode, matAl, Name+"_LV_Cathode", 0, 0, fStepLimit, false);
  G4VisAttributes* visCathode = new G4VisAttributes(TRUE, Color(ColID::Yellow, 1.0));
  visCathode->SetForceSolid(true);
  LogVol_Cathode->SetVisAttributes(visCathode);


  // =============================================================
  // 4. Physical Volume (内部配置)
  // =============================================================
  // Z軸上の相対位置（高さ方向の積み重ね）
  double z_scinti = 0.0 * mm;
  double z_guide  = z_scinti + h_Scinti / 2.0 + h_GuideTaper / 2.0;
  double z_pmt    = z_scinti + h_Scinti / 2.0 + h_GuideTaper + h_GuideCyl + h_PMT / 2.0;

  //原点がシンチレータとライトガイドの繋ぎめに対応
  //厚さ方向の中心と原点が一致する
  G4RotationMatrix* rot90X = new G4RotationMatrix();
  rot90X->rotateX(90.0 * deg);

  double x_shift = -rMin_Scinti; // -700.0 mm

  G4Transform3D transform_Scinti = 
       Rotate(Axis::X, 0.0 * deg) * G4Translate3D(-(rMin_Scinti + rMax_Scinti)/2, 0, 0) ; 

  new G4PVPlacement(transform_Scinti, LogVol_Scinti, "Scinti", LogVol, false, 0, checkOverlaps);


  //ライトガイドの配置
  G4Transform3D transform_Guide = 
        Rotate(Axis::Y, 90.0 * deg) * Rotate(Axis::X, 90.0 * deg) * G4Translate3D(0, 0, (h_GuideTaper)/2) ; 

  new G4PVPlacement(transform_Guide, LogVol_Guide, "Guide", LogVol, false, 0, checkOverlaps);


  //PMTの配置
  G4Transform3D transform_PMT = 
       G4Translate3D(0, -(h_GuideCyl + h_GuideTaper), 0) * Rotate(Axis::X, 90.0 * deg) * G4Translate3D(0, 0, h_PMT/2) ; 

  new G4PVPlacement(transform_PMT, LogVol_PMT, "PMT", LogVol, false, 0, checkOverlaps);
  
  //カソードの配置(PMTの内部ローカル座標なので X=0, Y=0 でOK)
  //若干配置が怪しい、、、（光学過程まで考えないのであれば問題ない）
  double z_cathode = -h_PMT / 2.0 + t_Cathode / 2.0;
  new G4PVPlacement(0, G4ThreeVector(0, 0, z_cathode), LogVol_Cathode, "Cathode", LogVol_PMT, false, 0, checkOverlaps);
  
}

HILELogVol::~HILELogVol() 
{
  delete Solid;
  delete LogVol;
}