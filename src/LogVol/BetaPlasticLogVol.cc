#include "Material/util/common.hh"

#include "LogVol/BetaPlasticLogVol.hh"
#include "Material/BC408Mat.hh"

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

BetaPlasticLogVol::BetaPlasticLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  if(logmode) G4cout << "-- BetaPlasticLogVol::BetaPlasticLogVol(G4String)\n";

  ////////////////////////////////////////////////////////////////////
  //// Solid

  const G4double thick_scinti = 1.5 * mm;
  const G4double thick_cover  = 2.5 * mm;

  // シンチレータ：直径70 mm、厚さ1.5 mm
  G4VSolid* Solid0 = new G4Tubs(
      Name + "_Solid0",
      0.,
      35 * mm,
      thick_scinti / 2.,
      0.,
      2 * pi);

  // 親Volume：直径72 mm、厚さ2.5 mm
  Solid = new G4Tubs(
      Name + "_Solid",
      0.,
      36 * mm,
      thick_cover / 2.,
      0.,
      2 * pi);

  // カバー：外形からシンチレータ部分を除く
  G4VSolid* Solid1 = new G4SubtractionSolid(
      Name + "_Solid1",
      Solid,
      Solid0,
      G4Transform3D());

  ////////////////////////////////////////////////////////////////////
  //// Material
  BC408Mat* fBC408 = new BC408Mat();
  auto Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  auto Mat0 = fBC408->GetMaterial();
  auto Mat1 = G4NistManager::Instance()->FindOrBuildMaterial("G4_PLEXIGLASS");

  ////////////////////////////////////////////////////////////////////
  //// Logical Volume
  LogVol = new G4LogicalVolume(Solid, Mat, Name+"_LogVol", 0, 0, fStepLimit, false);
  G4VisAttributes* vis = new G4VisAttributes(TRUE, Color(ColID::Cyan, 0.5) );
  vis->SetForceSolid(true);
  LogVol->SetVisAttributes(vis);
    
  G4LogicalVolume* LogVol0 = new G4LogicalVolume(Solid0, Mat0, Name+"_LogVol0", 0, 0, fStepLimit, false);
  G4LogicalVolume* LogVol1 = new G4LogicalVolume(Solid1, Mat1, Name+"_LogVol1", 0, 0, fStepLimit, false);
  LogVol0 -> SetVisAttributes( new G4VisAttributes(TRUE,Color(ColID::Cyan    ,0.5) ));
  LogVol1 -> SetVisAttributes( new G4VisAttributes(TRUE,Color(ColID::DarkGrey,0.8) ));

  ////////////////////////////////////////////////////////////////////
  //// Physical Volume
  new G4PVPlacement(G4Transform3D(), LogVol0, "Scinti", LogVol, false, 0, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol1, "Cover",  LogVol, false, 0, checkOverlaps);
    
  if(logmode) G4cout << "== BetaPlasticLogVol::BetaPlasticLogVol(G4String)\n";
}

BetaPlasticLogVol::~BetaPlasticLogVol() 
{
  if(logmode) G4cout << "-- BetaPlasticLogVol::~BetaPlasticLogVol()\n";
  delete Solid;
  delete LogVol;
  if(logmode) G4cout << "== BetaPlasticLogVol::~BetaPlasticLogVol()\n";
}
