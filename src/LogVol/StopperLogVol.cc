#include "Material/util/common.hh"
#include "Material/MgOMat.hh"


#include "LogVol/StopperLogVol.hh"

#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Tubs.hh"
#include "G4EllipticalTube.hh"
#include "G4UnionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"

#include "G4SystemOfUnits.hh"

#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"

namespace StopperUtil {
  
  enum Axis { X, Y, Z };
  G4Transform3D Rotate(int axis, double angle) {
    G4RotationMatrix rot;
    if( axis==Axis::X ) rot.rotateX( angle );
    if( axis==Axis::Y ) rot.rotateY( angle );
    if( axis==Axis::Z ) rot.rotateZ( angle );
    return G4Transform3D( rot, G4ThreeVector( 0,0,0 ) );
  }
  /* G4Transform3D Transform(int axis, double angle, double dx, double dy, double dz) { */
  /*   G4RotationMatrix rot; */
  /*   if( axis==Axis::X ) rot.rotateX( angle ); */
  /*   if( axis==Axis::Y ) rot.rotateY( angle ); */
  /*   if( axis==Axis::Z ) rot.rotateZ( angle ); */
  /*   return G4Transform3D( rot, G4ThreeVector( dx, dy, dz ) ); */
  /* } */

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
using namespace StopperUtil;

StopperLogVol::StopperLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  if(logmode) G4cout << "-- StopperLogVol::StopperLogVol(G4String)\n";

  ////////////////////////////////////////////////////////////////////
  //// Solid 
  G4VSolid* tmp0   = new G4Box(Name+"_tmp1",1*m,1*m,1*m);
  G4VSolid* tmp1   = new G4Box(Name+"_tmp0",16./2 *mm,24./2 *mm,0.5/2 *mm);
  G4VSolid* Solid0 = new G4IntersectionSolid(Name+"_Solid", tmp0, tmp1, Rotate( Axis::Y, 45*degree ) );

  Solid = Solid0->Clone();

  ////////////////////////////////////////////////////////////////////
  //// Material
  MgOMat* fMgOMat = new MgOMat();

  G4Material* Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  G4Material* Mat0 = fMgOMat->GetMaterial();

  ////////////////////////////////////////////////////////////////////
  //// Logical Volume
  LogVol = new G4LogicalVolume(Solid, Mat, Name+"_LogVol", 0, 0, fStepLimit, false);
  LogVol -> SetVisAttributes( new G4VisAttributes(TRUE,Color(ColID::Yellow,0.0) ));
    
  G4LogicalVolume* LogVol0 = new G4LogicalVolume(Solid0, Mat0, Name+"0_LogVol", 0, 0, fStepLimit, false);
  G4VisAttributes* vis0 = new G4VisAttributes(TRUE, Color(ColID::Cyan, 1.0));
  vis0 -> SetForceSolid(true);
  LogVol0 -> SetVisAttributes( vis0 );



  ////////////////////////////////////////////////////////////////////
  //// Physical Volume
  new G4PVPlacement(G4Transform3D(), LogVol0, "Stopper", LogVol, false, 0, checkOverlaps);
    
  if(logmode) G4cout << "== StopperLogVol::StopperLogVol(G4String)\n";
}

StopperLogVol::~StopperLogVol() 
{
  if(logmode) G4cout << "-- StopperLogVol::~StopperLogVol()\n";
  delete Solid;
  delete LogVol;
  if(logmode) G4cout << "== StopperLogVol::~StopperLogVol()\n";
}
