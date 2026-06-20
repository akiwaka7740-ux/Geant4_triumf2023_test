#include "Material/util/common.hh"

#include "LogVol/FloorLogVol.hh"

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

namespace {
  
  /* enum Axis { X, Y, Z }; */
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
    { ColID::DarkGrey, { 0.35,0.35,0.35} },
  };

  G4Colour Color(ColID ID, double Opacity=1) {
    return G4Colour( 
        defaultRGB[ID].R,
        defaultRGB[ID].G,
        defaultRGB[ID].B,
        Opacity );
  }
};

FloorLogVol::FloorLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps,
                         double wx, double wy, double wz)
{
  if(logmode) G4cout << "-- FloorLogVol::FloorLogVol(G4String)\n";

  ////////////////////////////////////////////////////////////////////
  //// Solid 
  G4VSolid* Solid0 = new G4Box(Name+"_Solid0", wx/2., wy/2., wz/2.);

  Solid = Solid0->Clone();
  Solid -> SetName(Name+"_Solid");

  ////////////////////////////////////////////////////////////////////
  //// Material
  G4Material* Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  G4Material* Mat0 = G4NistManager::Instance()->FindOrBuildMaterial("G4_CONCRETE");
  /* G4Material* Mat0 = G4NistManager::Instance()->FindOrBuildMaterial("G4_PARAFFIN"); */

  ////////////////////////////////////////////////////////////////////
  //// Logical Volume
  LogVol = new G4LogicalVolume(Solid, Mat, Name+"_LogVol", 0, 0, fStepLimit, false);
    
  G4LogicalVolume* LogVol0 = new G4LogicalVolume(Solid0, Mat0, Name+"_LogVol0", 0, 0, fStepLimit, false);

  //LogVol  -> SetVisAttributes( G4VisAttributes::Invisible );
  G4VisAttributes* vis0 = new G4VisAttributes(TRUE, Color(ColID::Grey, 1.0));
  vis0->SetForceSolid(true);
  LogVol0->SetVisAttributes(vis0);

  ////////////////////////////////////////////////////////////////////
  //// Physical Volume
  new G4PVPlacement(G4Transform3D(), LogVol0, Name, LogVol, false, 0, checkOverlaps);

  if(logmode) G4cout << "== FloorLogVol::FloorLogVol(G4String)\n";
}

FloorLogVol::~FloorLogVol() 
{
  if(logmode) G4cout << "-- FloorLogVol::~FloorLogVol()\n";
  delete Solid;
  delete LogVol;
  if(logmode) G4cout << "== FloorLogVol::~FloorLogVol()\n";
}
