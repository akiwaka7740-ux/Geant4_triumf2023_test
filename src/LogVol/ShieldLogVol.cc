#include "Material/util/common.hh"

#include "LogVol/ShieldLogVol.hh"

#include "G4VSolid.hh"
#include "G4Sphere.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Tubs.hh"
#include "G4UnionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"

#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"

namespace ShieldUtil {
  
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
using namespace ShieldUtil;

ShieldLogVol::ShieldLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps) 
{
  if(logmode) G4cout << "-- ShieldLogVol::ShieldLogVol(G4String)\n";

  G4VSolid* world = new G4Box("world_tmp", 1*m, 1*m, 1*m);

  int n=-1;
  G4VSolid* tmp[10];
  tmp[++n] = new G4Box(Name+"_tmp0", 60/2.*mm, 94/2.*mm, 120/2.*mm );
  tmp[++n] = new G4Box(Name+"_tmp1", 60/2.*mm, 100.0*mm, 500/2.*mm );
  tmp[++n] = new G4Tubs(Name+"_tmp2", 0, 46*mm, 100*mm, 0, 2*CLHEP::pi);

  tmp[++n] = new G4SubtractionSolid(Name+"_tmp3", tmp[0], tmp[1], 
      Transform( Axis::Y, -20*degree, -(60-29+60/2.*(1-cos(20/180.*CLHEP::pi)))*mm, 0, -120/2.*mm ) );

  tmp[++n] = new G4IntersectionSolid(Name+"_tmp4",  world, tmp[3], 
      Transform( Axis::Z,  90*degree, 0, -(60/2.+15)*mm, 120/2.*mm ) );
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp5", tmp[4], tmp[2], 
      0, G4ThreeVector( 0, 0, 120/2.*mm ) );

  //1,2は片割れしかない、0は両方にある
  Solid1 = new G4IntersectionSolid(Name+"1",  world, tmp[5], Transform( Axis::Z,   0*degree, 0, 0, 0 ) );
  Solid2 = new G4IntersectionSolid(Name+"2",  world, tmp[5], Transform( Axis::Z, 180*degree, 0, 0, 0 ) );

  Solid0 = new G4UnionSolid(Name+"0", Solid1, Solid2, G4Transform3D() ); 
    
  //////////////////////////////////////////////////////
  //// Material
  auto Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  auto MatC = G4NistManager::Instance()->FindOrBuildMaterial("G4_Pb");

  //////////////////////////////////////////////////////
  //// Logical Volume
  LogVol0 = new G4LogicalVolume(Solid0, Mat, "LogVol_Support", 0, 0, fStepLimit);
  LogVol1 = new G4LogicalVolume(Solid1, Mat, "LogVol_Support", 0, 0, fStepLimit);
  LogVol2 = new G4LogicalVolume(Solid2, Mat, "LogVol_Support", 0, 0, fStepLimit);
  

  
  G4LogicalVolume* LogVol0C = new G4LogicalVolume(Solid0, MatC, "LogVol_SupportC", 0, 0, fStepLimit);
  G4LogicalVolume* LogVol1C = new G4LogicalVolume(Solid1, MatC, "LogVol_SupportC", 0, 0, fStepLimit);
  G4LogicalVolume* LogVol2C = new G4LogicalVolume(Solid2, MatC, "LogVol_SupportC", 0, 0, fStepLimit);

  G4VisAttributes* vis0 = new G4VisAttributes(TRUE, Color(ColID::Red, 1.0));
  vis0->SetForceSolid(true);
  LogVol0C->SetVisAttributes(vis0);

  G4VisAttributes* vis1 = new G4VisAttributes(TRUE, Color(ColID::Red, 1.0));
  vis1->SetForceSolid(true);
  LogVol1C->SetVisAttributes(vis1);

  G4VisAttributes* vis2 = new G4VisAttributes(TRUE, Color(ColID::Red, 1.0));
  vis2->SetForceSolid(true);
  LogVol2C->SetVisAttributes(vis2);
  
  //LogVol0 -> SetVisAttributes( G4VisAttributes::Invisible );
  //LogVol1 -> SetVisAttributes( G4VisAttributes::Invisible );
  //LogVol2 -> SetVisAttributes( G4VisAttributes::Invisible );
/*
  LogVol0C -> SetVisAttributes( new G4VisAttributes(TRUE,G4Colour::Red()) );
  LogVol1C -> SetVisAttributes( new G4VisAttributes(TRUE,G4Colour::Red()) );
  LogVol2C -> SetVisAttributes( new G4VisAttributes(TRUE,G4Colour::Red()) );
*/

  new G4PVPlacement(G4Transform3D(), LogVol0C, Name, LogVol0, false, 0, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol1C, Name, LogVol1, false, 0, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol2C, Name, LogVol2, false, 0, checkOverlaps);

  if(logmode) G4cout << "== ShieldLogVol::ShieldLogVol(G4String)\n";
}

ShieldLogVol::~ShieldLogVol() 
{
  if(logmode) G4cout << "-- ShieldLogVol::~ShieldLogVol()\n";
  delete Solid0;
  delete Solid1;
  delete Solid2;
  delete LogVol0;
  delete LogVol1;
  delete LogVol2;
  if(logmode) G4cout << "== ShieldLogVol::~ShieldLogVol()\n";
}
