#include "Material/util/common.hh"

#include "LogVol/HPGeLogVol.hh"

#include "G4VSolid.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Tubs.hh"
#include "G4Polycone.hh"
#include "G4UnionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"
#include "G4SystemOfUnits.hh"

#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"

namespace HPGeUtil {
  
  enum Axis { X, Y, Z };
  G4Transform3D Transform(int axis, double angle, double dx, double dy, double dz) {
    G4RotationMatrix rot;
    if( axis==Axis::X ) rot.rotateX( angle );
    if( axis==Axis::Y ) rot.rotateY( angle );
    if( axis==Axis::Z ) rot.rotateZ( angle );
    return G4Transform3D( rot, G4ThreeVector( dx, dy, dz ) );
  }

  enum class ColID {
    White,
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
        defaultRGB[ID].R,
        defaultRGB[ID].G,
        defaultRGB[ID].B,
        Opacity );
  }
};
using namespace HPGeUtil;

#include <map>
namespace HPGeGeom {

  struct GeVal { double   CryD,    CryL,    KubiD,    KubiL,  AidaL,  DewarD, DewarL, PlaD; };
  std::map<G4String, GeVal> GePar = {
    { "Handai60",    { 67.1*mm, 85.4*mm,  90.0*mm, 246.0*mm,  60*mm,  222*mm, 267*mm, 50.0*mm } }, 
    { "Handai55",    { 63.7*mm, 87.3*mm,  83.0*mm, 416.0*mm,  60*mm,  222*mm, 267*mm, 70.0*mm } }, //結晶サイズ以外はmanualに沿って入れた（合ってるかの確証はない）
    { "Handai50",    { 65.0*mm, 63.0*mm,  90.0*mm, 346.0*mm,  60*mm,  222*mm, 267*mm, 70.0*mm } }, 
    { "Handai45",    { 62.0*mm, 65.0*mm,  80.0*mm, 285.0*mm,  60*mm,  222*mm, 267*mm, 70.0*mm } }, 
    { "Kyudai80",    { 58.0*mm, 75.0*mm,  90.0*mm, 317.0*mm,  60*mm,  222*mm, 267*mm, 70.0*mm } },//結晶サイズだけわからないので、Kyudai40と同じ値を仮に入れている
    { "Kyudai40",    { 58.0*mm, 75.0*mm,  70.0*mm, 351.0*mm,  60*mm,  222*mm, 267*mm, 70.0*mm } }, 
    { "KyudaiLEPS",  { 51.0*mm, 20.1*mm,  70.0*mm, 238.0*mm,  60*mm,  222*mm, 267*mm, 70.0*mm } }, 
    { "Ishinomaki",  { 63.6*mm, 71.8*mm,  83.0*mm, 311.0*mm,  60*mm,  222*mm, 267*mm, 70.0*mm } }, 
    //// SUNYは値がわからなかったので適当
    { "SUNY_MARSHA", { 45.0*mm, 40.0*mm,  70.0*mm, 234.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } },
    { "SUNY_GREG",   { 45.0*mm, 40.0*mm,  70.0*mm, 234.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } },
    { "SUNY_JAN",    { 45.0*mm, 40.0*mm,  70.0*mm, 234.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } },
    { "SUNY_CINDY",  { 45.0*mm, 40.0*mm,  70.0*mm, 234.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } },
    { "SUNY_CAROL",  { 45.0*mm, 40.0*mm,  70.0*mm, 234.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } },
    { "SUNY_MIKE",   { 45.0*mm, 40.0*mm,  70.0*mm, 234.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } },
    { "SUNY_PETER",  { 45.0*mm, 40.0*mm,  70.0*mm, 234.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } },
    { "SUNY_ALICE",  { 45.0*mm, 40.0*mm,  70.0*mm, 191.0*mm,  60*mm,  152*mm, 228*mm, 85.0*mm } },
    { "SUNY_LEPS",   { 45.0*mm, 40.0*mm,  70.0*mm, 191.0*mm,  60*mm,  152*mm, 228*mm, 70.0*mm } } 
  };

};
using namespace HPGeGeom;

HPGeLogVol::HPGeLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  if(logmode) G4cout << "-- HPGeLogVol::HPGeLogVol(G4String)\n";

  if( !GePar.count(Name) ) {
    G4cout << " #W : no matching name of Ge (" << Name << ")\n";
    return;
  }

  G4cout << Name << " : " 
    << GePar[Name].CryD  << "mm, " << GePar[Name].CryL  << "mm : "
    << GePar[Name].KubiD << "mm, " << GePar[Name].KubiL << "mm\n", fflush(stdout); 

  //////////////////////////////////////////////////////
  //// Solid

  //// Kubi
  const int nZ_Kubi = 8;
  double zP_Kubi[nZ_Kubi] = {
    0.0*mm,                                      0.5*mm,
    0.5*mm,                                     GePar[Name].KubiL-GePar[Name].AidaL-1*mm,
    GePar[Name].KubiL-GePar[Name].AidaL-1*mm,   GePar[Name].KubiL-GePar[Name].AidaL,
    GePar[Name].KubiL-GePar[Name].AidaL,        GePar[Name].KubiL 
  };
  double rI_Kubi[nZ_Kubi] = {
    0.0*mm,                      0.0*mm,
    GePar[Name].KubiD/2.-1*mm,   GePar[Name].KubiD/2.-1*mm,
    24*mm,                       24*mm,
    24*mm,                       24*mm 
  };
  double rO_Kubi[nZ_Kubi] = {
    GePar[Name].KubiD/2.,   GePar[Name].KubiD/2.,
    GePar[Name].KubiD/2.,   GePar[Name].KubiD/2.,
    GePar[Name].KubiD/2.,   GePar[Name].KubiD/2.,
    25*mm,                  25*mm 
  };
  G4VSolid* Solid0 = new G4Polycone(Name+"_Kubi", 0, 2*CLHEP::pi, nZ_Kubi, zP_Kubi, rI_Kubi, rO_Kubi);

  //// Dewar
  const int nZ_Dewar = 6;
  double zP_Dewar[nZ_Dewar] = {
    GePar[Name].KubiL,                          GePar[Name].KubiL+1*mm,
    GePar[Name].KubiL+1*mm,                     GePar[Name].KubiL+GePar[Name].DewarL-1*mm,
    GePar[Name].KubiL+GePar[Name].DewarL-1*mm,  GePar[Name].KubiL+GePar[Name].DewarL
  };
  double rI_Dewar[nZ_Dewar] = {
    24*mm,                       24*mm,
    GePar[Name].DewarD/2.-1*mm,  GePar[Name].DewarD/2.-1*mm,
    0.0*mm,                      0.0*mm 
  };
  double rO_Dewar[nZ_Dewar] = {
    GePar[Name].DewarD/2.,  GePar[Name].DewarD/2.,
    GePar[Name].DewarD/2.,  GePar[Name].DewarD/2.,
    GePar[Name].DewarD/2.,  GePar[Name].DewarD/2. 
  };
  G4VSolid* Solid1 = new G4Polycone(Name+"_Dewar", 0, 2*CLHEP::pi, nZ_Dewar, zP_Dewar, rI_Dewar, rO_Dewar);

  //// Crystal
  const int nZ_Cry = 2;
  double zP_Cry[nZ_Cry] = { 5*mm, 5*mm+GePar[Name].CryL };
  double rI_Cry[nZ_Cry] = { 0, 0 };
  double rO_Cry[nZ_Cry] = { GePar[Name].CryD/2., GePar[Name].CryD/2. };
  G4VSolid* Solid2 = new G4Polycone(Name+"_Crystal", 0, 2*CLHEP::pi, nZ_Cry, zP_Cry, rI_Cry, rO_Cry);
    
  //// liquid N2
  const int nZ_N2 = 2;
  double zP_N2[nZ_N2] = { GePar[Name].KubiL+(1-0.7)*GePar[Name].DewarL/2., GePar[Name].KubiL+(1+0.7)*GePar[Name].DewarL/2.};
  double rI_N2[nZ_N2] = { 0, 0 };
  double rO_N2[nZ_N2] = { 0.7*GePar[Name].DewarD/2., 0.7*GePar[Name].DewarD/2. };
  G4VSolid* Solid3 = new G4Polycone(Name+"_LiqN2", 0, 2*CLHEP::pi, nZ_N2, zP_N2, rI_N2, rO_N2);
    
  //// Plastic
  G4VSolid* Solid4_tmp0;
  G4VSolid* Solid4_tmp1;
  G4VSolid* Solid4;
  {
    const int nZ_Pla = 2;
    double zP_Pla[nZ_Pla] = { -(2+1.5/2)*mm, -(2-1.5/2.)*mm };
    double rI_Pla[nZ_Pla] = { 0, 0 };
    double rO_Pla[nZ_Pla] = { GePar[Name].PlaD/2., GePar[Name].PlaD/2. };
    Solid4_tmp0 = new G4Polycone(Name+"_Pla", 0, 2*CLHEP::pi, nZ_Pla, zP_Pla, rI_Pla, rO_Pla);
    Solid4 = Solid4_tmp0;
  }
  if( Name=="Handai60" )
  {
    const int nZ_Pla = 2;
    double zP_Pla[nZ_Pla] = { -(4+1.5/2)*mm, -(4-1.5/2.)*mm };
    double rI_Pla[nZ_Pla] = { 0, 0 };
    double rO_Pla[nZ_Pla] = { 21*mm, 21*mm };
    Solid4_tmp1 = new G4Polycone(Name+"_Pla", 0, 2*CLHEP::pi, nZ_Pla, zP_Pla, rI_Pla, rO_Pla);
    Solid4 = new G4UnionSolid(Name+"_Pla", Solid4_tmp0, Solid4_tmp1, G4Transform3D());
  }
    
  //// Merge All Volume = Mother Solid
  G4VSolid* tmp[4];
  tmp[0] = new G4UnionSolid(Name+"_tmp0", Solid0, Solid1, G4Transform3D());
  tmp[1] = new G4UnionSolid(Name+"_tmp1", tmp[0], Solid2, G4Transform3D());
  tmp[2] = new G4UnionSolid(Name+"_tmp2", tmp[1], Solid3, G4Transform3D());
  tmp[3] = new G4UnionSolid(Name+"_tmp3", tmp[2], Solid4, G4Transform3D());
  // Solid = tmp[2];
  Solid = tmp[3];

  //////////////////////////////////////////////////////
  //// Material
  auto Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  auto Mat0 = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");
  auto Mat1 = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");
  auto Mat2 = G4NistManager::Instance()->FindOrBuildMaterial("G4_Ge");
  auto Mat3 = G4NistManager::Instance()->FindOrBuildMaterial("LiquidNitrogen");
  auto Mat4 = G4NistManager::Instance()->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
  if( !Mat3 ) Mat3 = new G4Material( "LiquidNitrogen",
                                     7.,
                                     14.01*g/mole,
                                     0.808*g/cm3,
                                     kStateLiquid,
                                     77*kelvin);

  //////////////////////////////////////////////////////
  //// Logical Volume
  LogVol = new G4LogicalVolume(Solid, Mat, Name+"_LogVol", 0, 0, fStepLimit);

  G4LogicalVolume* LogVol0 = new G4LogicalVolume(Solid0, Mat0, Name+"_Kubi_LogVol",    0, 0, fStepLimit);
  G4LogicalVolume* LogVol1 = new G4LogicalVolume(Solid1, Mat1, Name+"_Dewar_LogVol",   0, 0, fStepLimit);
  G4LogicalVolume* LogVol2 = new G4LogicalVolume(Solid2, Mat2, Name+"_Crystal_LogVol", 0, 0, fStepLimit);
  G4LogicalVolume* LogVol3 = new G4LogicalVolume(Solid3, Mat3, Name+"_LiqN2_LogVol",   0, 0, fStepLimit);
  G4LogicalVolume* LogVol4 = new G4LogicalVolume(Solid4, Mat4, Name+"_Pla_LogVol",     0, 0, fStepLimit);

  // 可視化属性の定義と設定
  
  // Kubi (Grey, 50%透明)
  G4VisAttributes* vis0 = new G4VisAttributes(TRUE, Color(ColID::Grey, 0.5));
  vis0->SetForceSolid(true); // 塗りつぶしを強制
  LogVol0->SetVisAttributes(vis0);

  // Dewar (Brown, 50%透明)
  G4VisAttributes* vis1 = new G4VisAttributes(TRUE, Color(ColID::Brown, 0.5));
  vis1->SetForceSolid(true);
  LogVol1->SetVisAttributes(vis1);

  // Crystal (Magenta, 不透明)
  G4VisAttributes* vis2 = new G4VisAttributes(TRUE, Color(ColID::Magenta, 1.0));
  vis2->SetForceSolid(true);
  LogVol2->SetVisAttributes(vis2);

  // LiqN2 (Cyan, 100%不透明設定だが、中身が見えるよう必要に応じて下げても良い)
  G4VisAttributes* vis3 = new G4VisAttributes(TRUE, Color(ColID::Cyan, 1.0));
  vis3->SetForceSolid(true);
  LogVol3->SetVisAttributes(vis3);

  // Plastic (Cyan, 100%不透明)
  G4VisAttributes* vis4 = new G4VisAttributes(TRUE, Color(ColID::Cyan, 1.0));
  vis4->SetForceSolid(true);
  LogVol4->SetVisAttributes(vis4);

  new G4PVPlacement(G4Transform3D(), LogVol0, "Kubi"   , LogVol, false, 0, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol1, "Dewar"  , LogVol, false, 1, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol2, "Crystal", LogVol, false, 2, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol3, "LiqN2"  , LogVol, false, 3, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol4, "Pla"    , LogVol, false, 4, checkOverlaps);

  if(logmode) G4cout << "== HPGeLogVol::HPGeLogVol(G4String)\n";
}

HPGeLogVol::~HPGeLogVol() 
{
  if(logmode) G4cout << "-- HPGeLogVol::~HPGeLogVol()\n";
  delete Solid;
  delete LogVol;
  if(logmode) G4cout << "== HPGeLogVol::~HPGeLogVol()\n";
}
