#include "Material/util/common.hh"

#include "LogVol/ChamberLogVol.hh"

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

namespace ChamberUtil {
  
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
using namespace ChamberUtil;

ChamberLogVol::ChamberLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  if(logmode) G4cout << "-- ChamberLogVol::ChamberLogVol(G4String)\n";

  ////////////////////////////////////////////////////////////////////
  //// Solid 
  const int nZ = 6;
  double zPlane[nZ] = {
    -277*mm,       (-277+193)*mm,
    (-277+193)*mm, 70*mm,
    70*mm,         71*mm
  };
  double rInner[nZ] = {
    (40/2.-2)*mm, (40/2.-2)*mm,
    (38/2.-1)*mm, (38/2.-1)*mm,
            0*mm,         0*mm 
  };
  double rOuter[nZ] = {
    40/2.*mm, 40/2.*mm,
    38/2.*mm, 38/2.*mm,
    38/2.*mm, 38/2.*mm 
  };
  G4VSolid* Solid0 = new G4Polycone(Name+"_Solid0", 0, 2*CLHEP::pi, nZ, zPlane, rInner, rOuter);

  Solid = Solid0->Clone();
  Solid -> SetName(Name+"_Solid");

  ////////////////////////////////////////////////////////////////////
  //// Material
  G4Material* GFRP = new G4Material("GFRP",1.80*g/cm3,8);
  G4Material* SiO2  = G4NistManager::Instance()->FindOrBuildMaterial("G4_SILICON_DIOXIDE");
  G4Material* Al2O3 = G4NistManager::Instance()->FindOrBuildMaterial("G4_ALUMINUM_OXIDE");
  G4Material* TiO2  = G4NistManager::Instance()->FindOrBuildMaterial("G4_TITANIUM_DIOXIDE");
  G4Material* B2O3  = G4NistManager::Instance()->FindOrBuildMaterial("G4_BORON_OXIDE");
  G4Material* CaO   = G4NistManager::Instance()->FindOrBuildMaterial("G4_CALCIUM_OXIDE");
  G4Material* MgO   = G4NistManager::Instance()->FindOrBuildMaterial("G4_MAGNESIUM_OXIDE");
  G4Material* Na2O  = G4NistManager::Instance()->FindOrBuildMaterial("G4_SODIUM_MONOXIDE");
  G4Material* K2O   = G4NistManager::Instance()->FindOrBuildMaterial("G4_POTASSIUM_OXIDE");
  GFRP->AddMaterial(SiO2, 55.*perCent);
  GFRP->AddMaterial(Al2O3,14.*perCent);
  GFRP->AddMaterial(TiO2, 0.2*perCent);
  GFRP->AddMaterial(B2O3,  7.*perCent);
  GFRP->AddMaterial(CaO,  22.*perCent);
  GFRP->AddMaterial(MgO,   1.*perCent);
  GFRP->AddMaterial(Na2O, 0.5*perCent);
  GFRP->AddMaterial(K2O,  0.3*perCent);

  G4Material* Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  G4Material* Mat0 = GFRP;

  ////////////////////////////////////////////////////////////////////
  //// Logical Volume
  LogVol = new G4LogicalVolume(Solid, Mat, Name+"_LogVol", 0, 0, fStepLimit, false);
    
  G4LogicalVolume* LogVol0 = new G4LogicalVolume(Solid0, Mat0, Name+"_LogVol0", 0, 0, fStepLimit, false);

  //LogVol  -> SetVisAttributes( G4VisAttributes::Invisible );
  //LogVol0 -> SetVisAttributes( new G4VisAttributes(TRUE,Color(ColID::White,0.5) ));

  G4VisAttributes* vis0 = new G4VisAttributes(TRUE, Color(ColID::Yellow, 0.5));
  vis0->SetForceSolid(true);
  LogVol0->SetVisAttributes(vis0);

  ////////////////////////////////////////////////////////////////////
  //// Physical Volume
  new G4PVPlacement(G4Transform3D(), LogVol0, Name, LogVol, false, 0, checkOverlaps);

  if(logmode) G4cout << "== ChamberLogVol::ChamberLogVol(G4String)\n";
}

ChamberLogVol::~ChamberLogVol() 
{
  if(logmode) G4cout << "-- ChamberLogVol::~ChamberLogVol()\n";
  delete Solid;
  delete LogVol;
  if(logmode) G4cout << "== ChamberLogVol::~ChamberLogVol()\n";
}
