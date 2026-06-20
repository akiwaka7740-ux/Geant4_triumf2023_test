#include "Material/util/common.hh"

#include "LogVol/FrameLogVol.hh"

#include "G4VSolid.hh"
#include "G4Sphere.hh"
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

namespace FrameUtil {
  
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
    DarkGreen,
  };

  struct RGB { double R; double G; double B; };
  static std::map<ColID, RGB> defaultRGB = {
    { ColID::White,     { 1.0, 1.0, 1.0 } },
    { ColID::Gray,      { 0.5, 0.5, 0.5 } },
    { ColID::Grey,      { 0.5, 0.5, 0.5 } },
    { ColID::Black,     { 0.0, 0.0, 0.0 } },
    { ColID::Brown,     { 0.45,0.25,0.0 } },
    { ColID::Red,       { 1.0, 0.0, 0.0 } },
    { ColID::Green,     { 0.0, 1.0, 0.0 } },
    { ColID::Blue,      { 0.0, 0.0, 1.0 } },
    { ColID::Cyan,      { 0.0, 1.0, 1.0 } },
    { ColID::Magenta,   { 1.0, 0.0, 1.0 } },
    { ColID::Yellow ,   { 1.0, 1.0, 0.0 } },
    { ColID::DarkGrey,  { 0.25,0.25,0.25} },
    { ColID::DarkGreen, { 0.0, 0.25,0.0 } },
  };

  G4Colour Color(ColID ID, double Opacity=1) {
    return G4Colour( 
        defaultRGB[ID].R,
        defaultRGB[ID].G,
        defaultRGB[ID].B,
        Opacity );
  }
};
using namespace FrameUtil;

FrameLogVol::FrameLogVol(G4String Name, G4UserLimits* fStepLimit, G4bool checkOverlaps)
{
  if(logmode) G4cout << "-- FrameLogVol::FrameLogVol(G4String)\n";

  //////////////////////////////////////////////////////
  //// Solid
  G4VSolid* world = new G4Box("world_tmp", 2*m, 2*m, 2*m);

  int n=-1;
  G4VSolid* tmp[40];

  //// enkan & LR plate
  tmp[++n] = new G4Tubs(Name+"_tmp0", 460/2.*mm, 560/2.*mm, 30/2.*mm, 0, 2*CLHEP::pi);
  tmp[++n] = new G4Box(Name+"_tmp1", 100*mm, 560/2.*mm, 30*mm );

  tmp[++n] = new G4SubtractionSolid(Name+"_tmp2", tmp[0], tmp[1], 
      0, G4ThreeVector( -(248+100)*mm, 0, 0 ) );

  tmp[++n] = new G4Box(Name+"_tmp3", 375/2.*mm, 12/2.*mm, 635/2.*mm );
  tmp[++n] = new G4Box(Name+"_tmp4", 100/2.*mm, 20/2.*mm, 635/2.*mm );

  tmp[++n] = new G4SubtractionSolid(Name+"_tmp5", tmp[3], tmp[4], 
      0, G4ThreeVector( -(375/2.+100/2.-75.)*mm, 0, (45-30/2.)*mm ) );

  tmp[++n] = new G4UnionSolid(Name+"_tmp6", tmp[2], tmp[5], 
      Transform( Axis::X,   0*degree, -(375/2.-300/2.)*mm, -(175+12/2.)*mm,  (635/2.-45)*mm ) );
  tmp[++n] = new G4UnionSolid(Name+"_tmp7", tmp[2], tmp[5], 
      Transform( Axis::X, 180*degree, -(375/2.-300/2.)*mm, -(175+12/2.)*mm, -(635/2.-45)*mm ) );

  tmp[++n] = new G4IntersectionSolid(Name+"_tmp8",  world, tmp[6], 
      Transform( Axis::Y, -90*degree, -(250/2.-50/2.)*mm, 0, 0) );
  tmp[++n] = new        G4UnionSolid(Name+"_tmp9", tmp[8], tmp[7], 
      Transform( Axis::Y, -90*degree,  (250/2.-50/2.)*mm, 0, 0) );

  G4VSolid* Solid0 = tmp[n];
  Solid0 -> SetName(Name+"_Enkan_Solid");

  //// support frame for enkan & LR plate
  tmp[++n] = new G4Box(Name+"_tmp10", 500/2.*mm, 860/2.*mm, 300/2.*mm);
  tmp[++n] = new G4Box(Name+"_tmp11", (500/2.- 6)*mm, (860/2.- 6)*mm, (300/2.- 6)*mm);
  tmp[++n] = new G4Box(Name+"_tmp12", (500/2.+50)*mm, (860/2.-50)*mm, (300/2.-50)*mm);
  tmp[++n] = new G4Box(Name+"_tmp13", (500/2.-50)*mm, (860/2.+50)*mm, (300/2.-50)*mm);
  tmp[++n] = new G4Box(Name+"_tmp14", (500/2.-50)*mm, (860/2.-50)*mm, (300/2.+50)*mm);

  tmp[++n] = new G4SubtractionSolid(Name+"_tmp15", tmp[10], tmp[11], 0, G4ThreeVector( 0, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp16", tmp[15], tmp[12], 0, G4ThreeVector( 0, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp17", tmp[16], tmp[13], 0, G4ThreeVector( 0, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp18", tmp[17], tmp[14], 0, G4ThreeVector( 0, 0, 0 ));

  tmp[++n] = new G4IntersectionSolid(Name+"_tmp19",   world, tmp[18], 0, G4ThreeVector( -(500/2.+370/2.)*mm, -(860/2.+175+12+0.1)*mm, 0 ));
  tmp[++n] = new        G4UnionSolid(Name+"_tmp20", tmp[19], tmp[18], 0, G4ThreeVector(  (500/2.+370/2.)*mm, -(860/2.+175+12+0.1)*mm, 0 ));

  G4VSolid* Solid1 = tmp[n];
  Solid1 -> SetName(Name+"_Support0");

  //// movable plate for all setup
  tmp[++n] = new G4Box(Name+"_tmp21", 1370/2.*mm, 20/2.*mm, 600/2.*mm );
  tmp[++n] = new G4IntersectionSolid(Name+"_tmp22", world, tmp[21], 0, G4ThreeVector( 0, -(1048+20/2.+0.02)*mm, 0));

  G4VSolid* Solid2 = tmp[n];
  Solid2 -> SetName(Name+"_Support1");

  //// fundamental support frame
  tmp[++n] = new G4Box(Name+"_tmp23", 1450/2.*mm, 418/2.*mm, 1350/2.*mm);
  tmp[++n] = new G4Box(Name+"_tmp24", (1450/2.+50)*mm, (418/2.-75)*mm, (1350/2.- 75)*mm);
  tmp[++n] = new G4Box(Name+"_tmp25", ( 250/2.- 0)*mm, (418/2.+75)*mm, (1350/2.-150)*mm);
  tmp[++n] = new G4Box(Name+"_tmp26", ( 300/2.- 0)*mm, (418/2.+75)*mm, (1350/2.-150)*mm);
  tmp[++n] = new G4Box(Name+"_tmp27", ( 500/2.- 0)*mm, (418/2.-75)*mm, (1350/2.+150)*mm);

  tmp[++n] = new G4SubtractionSolid(Name+"_tmp28", tmp[23], tmp[24], 0, G4ThreeVector( 0, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp29", tmp[28], tmp[25], 0, G4ThreeVector( 0, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp30", tmp[29], tmp[26], 0, G4ThreeVector( -400*mm, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp31", tmp[30], tmp[26], 0, G4ThreeVector(  400*mm, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp32", tmp[31], tmp[27], 0, G4ThreeVector( -(150+500)/2.*mm, 0, 0 ));
  tmp[++n] = new G4SubtractionSolid(Name+"_tmp33", tmp[32], tmp[27], 0, G4ThreeVector(  (150+500)/2.*mm, 0, 0 ));

  tmp[++n] = new G4IntersectionSolid(Name+"_tmp34", world, tmp[33], 0, G4ThreeVector( 0, -(1048+60+418.4/2.+0.1)*mm, -(1350/2.-1000)*mm ));

  G4VSolid* Solid3 = tmp[n];
  Solid3 -> SetName(Name+"_Support3");

  tmp[++n] = new G4UnionSolid(Name+"_tmp35", Solid0,Solid1);
  tmp[++n] = new G4UnionSolid(Name+"_tmp36",tmp[35],Solid2);
  tmp[++n] = new G4UnionSolid(Name+"_tmp37",tmp[36],Solid3);
  Solid = tmp[n];
  Solid -> SetName(Name+"_Solid");

  //////////////////////////////////////////////////////
  //// Material
  auto Mat  = G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
  auto Mat0 = G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");
  auto Mat1 = G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");
  auto Mat2 = G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");
  auto Mat3 = G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");

  //////////////////////////////////////////////////////
  //// Logical Volume
  LogVol = new G4LogicalVolume(Solid, Mat, Name+"_LogVol", 0, 0, fStepLimit, false);

  G4LogicalVolume* LogVol0 = new G4LogicalVolume(Solid0, Mat0, Name+"0_LogVol", 0, 0, fStepLimit, false);
  G4LogicalVolume* LogVol1 = new G4LogicalVolume(Solid1, Mat1, Name+"1_LogVol", 0, 0, fStepLimit, false);
  G4LogicalVolume* LogVol2 = new G4LogicalVolume(Solid2, Mat2, Name+"2_LogVol", 0, 0, fStepLimit, false);
  G4LogicalVolume* LogVol3 = new G4LogicalVolume(Solid3, Mat3, Name+"3_LogVol", 0, 0, fStepLimit, false);
  
  //LogVol  -> SetVisAttributes( G4VisAttributes::Invisible );
  
  G4VisAttributes* vis0 = new G4VisAttributes(TRUE, Color(ColID::Grey, 1.0));
  vis0->SetForceSolid(true);
  //vis0->SetForceWireframe(true); 
  LogVol0->SetVisAttributes(vis0);

  G4VisAttributes* vis1 = new G4VisAttributes(TRUE, Color(ColID::Green, 1.0));
  vis1->SetForceSolid(true);
  //vis1->SetForceWireframe(true);
  LogVol1->SetVisAttributes(vis1);

  G4VisAttributes* vis2 = new G4VisAttributes(TRUE, Color(ColID::Grey, 1.0));
  vis2->SetForceSolid(true);
  //vis2->SetForceWireframe(true);
  LogVol2->SetVisAttributes(vis2);

  G4VisAttributes* vis3 = new G4VisAttributes(TRUE, Color(ColID::DarkGreen, 1.0));
  vis3->SetForceSolid(true);
  //vis3->SetForceWireframe(true);
  LogVol3->SetVisAttributes(vis3);
  

  /*
  LogVol0 -> SetVisAttributes( new G4VisAttributes(TRUE,G4Colour::Grey()) );
  LogVol1 -> SetVisAttributes( new G4VisAttributes(TRUE,G4Colour::Green()) );
  LogVol2 -> SetVisAttributes( new G4VisAttributes(TRUE,G4Colour::Grey()) );
  LogVol3 -> SetVisAttributes( new G4VisAttributes(TRUE,Color(ColID::DarkGreen) ));
    */

  new G4PVPlacement(G4Transform3D(), LogVol0, "Enkan",      LogVol, false, 0, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol1, "LRSupport",  LogVol, false, 1, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol2, "Plate",      LogVol, false, 2, checkOverlaps);
  new G4PVPlacement(G4Transform3D(), LogVol3, "Foundation", LogVol, false, 3, checkOverlaps);
    
  if(logmode) G4cout << "== FrameLogVol::FrameLogVol(G4String)\n";
}

FrameLogVol::~FrameLogVol() 
{
  if(logmode) G4cout << "-- FrameLogVol::~FrameLogVol()\n";
  delete Solid;
  delete LogVol;
  if(logmode) G4cout << "== FrameLogVol::~FrameLogVol()\n";
}

