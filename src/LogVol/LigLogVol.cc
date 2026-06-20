#include "LogVol/LigLogVol.hh"
#include "Material/GS20Mat.hh"

#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Tubs.hh"
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
    { ColID::Grey,     { 0.5, 0.5, 0.5 } },
    { ColID::Black,    { 0.0, 0.0, 0.0 } },
    { ColID::Brown,    { 0.45,0.25,0.0 } },
    { ColID::Red,      { 1.0, 0.0, 0.0 } },
    { ColID::Green,    { 0.0, 1.0, 0.0 } },
    { ColID::Blue,     { 0.0, 0.0, 1.0 } },
    { ColID::Cyan,     { 0.0, 1.0, 1.0 } },
    { ColID::Magenta,  { 1.0, 0.0, 1.0 } },
    { ColID::Yellow ,  { 1.0, 1.0, 0.0 } },
    { ColID::DarkGrey, { 0.25,0.25,0.25} },
  };

  G4Colour Color(ColID ID, double Opacity=1) {
    return G4Colour( 
        defaultRGB[ID].R, defaultRGB[ID].G, defaultRGB[ID].B, Opacity );
  }
};

LigLogVol::LigLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  //if(logmode) G4cout << "-- LigLogVol::LigLogVol(G4String)\n";

  ////////////////////////////////////////////////////////////////////
  //// Solid 
  double diameter_Lig  = 50.0 * mm;
  double thickness_Lig = 10.0 * mm;
  double cathode_W_Lig = 46.0 * mm;
  double cathode_T_Lig  = 1.0 * mm;
  double PMT_W_Lig     = 53.0 * mm;
  double PMT_L_Lig     = 235.0 * mm;

  // 各パーツのSolid作成
  G4VSolid* tmp0 = new G4Tubs(Name+"_tmp0", 0, diameter_Lig/2., thickness_Lig/2., 0, 2*pi); // Scinti
  G4VSolid* tmp1 = new G4Tubs(Name+"_tmp1", 0, cathode_W_Lig/2., cathode_T_Lig/2., 0, 2*pi); // Cathode
  G4VSolid* tmp2 = new G4Tubs(Name+"_tmp2", 0, PMT_W_Lig/2., PMT_L_Lig/2., 0, 2*pi); // PMT

  // これらを全て包み込む「親ボリューム（Mother Volume）」の作成
  double total_length = thickness_Lig + cathode_T_Lig + PMT_L_Lig; // 246 mm
  double max_radius   = PMT_W_Lig / 2.; // 最大半径
  Solid = new G4Tubs(Name+"_Solid", 0, max_radius, total_length/2., 0, 2*pi);

  ////////////////////////////////////////////////////////////////////
  //// Material
  GS20Mat* fGS20 = new GS20Mat();
  auto Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic"); // 親空間は真空
  auto Mat0 = fGS20->GetMaterial();
  auto Mat1 = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");
  auto Mat2 = G4NistManager::Instance()->FindOrBuildMaterial("G4_Pyrex_Glass");

  ////////////////////////////////////////////////////////////////////
  //// Logical Volume
  LogVol = new G4LogicalVolume(Solid, Mat, Name+"_LogVol", 0, 0, fStepLimit, false);
  LogVol -> SetVisAttributes( new G4VisAttributes(TRUE,Color(ColID::White,0.0) )); // 親は見えないようにする
    
  LogVol0 = new G4LogicalVolume(tmp0, Mat0, Name+"_LogVol0", 0, 0, fStepLimit, false);
  G4LogicalVolume* LogVol1 = new G4LogicalVolume(tmp1, Mat1, Name+"_LogVol1", 0, 0, fStepLimit, false);
  G4LogicalVolume* LogVol2 = new G4LogicalVolume(tmp2, Mat2, Name+"_LogVol2", 0, 0, fStepLimit, false);
  
  ////////////////////////////////////////////////////////////////////
  //// Logical Volume 可視化設定の修正
  
  // Scintillator (LogVol0): 黄色、不透明度 0.5、塗りつぶし
  G4VisAttributes* visScinti = new G4VisAttributes(TRUE, Color(ColID::Yellow, 0.5));
  visScinti->SetForceSolid(true); 
  LogVol0->SetVisAttributes(visScinti);

  // Cathode (LogVol1): 青色、不透明度 0.5、塗りつぶし
  G4VisAttributes* visCathode = new G4VisAttributes(TRUE, Color(ColID::Blue, 0.5));
  visCathode->SetForceSolid(true); 
  LogVol1->SetVisAttributes(visCathode);

  // PMT (LogVol2): マゼンタ（紫）、不透明度 0.8、塗りつぶし
  G4VisAttributes* visPMT = new G4VisAttributes(TRUE, Color(ColID::Magenta, 0.8));
  visPMT->SetForceSolid(true); // 
  LogVol2->SetVisAttributes(visPMT);

  ////////////////////////////////////////////////////////////////////
  //// Physical Volume
  // 親ボリュームの中心(0,0,0)から見た、Z軸上の相対位置を計算して配置します。
  double z_scinti  = total_length/2. - thickness_Lig/2.; // 前面に配置
  double z_cathode = z_scinti - thickness_Lig/2. - cathode_T_Lig/2.;
  double z_pmt     = z_cathode - cathode_T_Lig/2. - PMT_L_Lig/2.;

  new G4PVPlacement(Move(0,0,z_scinti),  LogVol0, "Scinti",  LogVol, false, 0, checkOverlaps);
  new G4PVPlacement(Move(0,0,z_cathode), LogVol1, "Cathode", LogVol, false, 0, checkOverlaps);
  new G4PVPlacement(Move(0,0,z_pmt),     LogVol2, "PMT",     LogVol, false, 0, checkOverlaps);
    
  //if(logmode) G4cout << "== LigLogVol::LigLogVol(G4String)\n";
}

LigLogVol::~LigLogVol() 
{
  //if(logmode) G4cout << "-- LigLogVol::~LigLogVol()\n";
  delete Solid;
  delete LogVol;
  //if(logmode) G4cout << "== LigLogVol::~LigLogVol()\n";
}