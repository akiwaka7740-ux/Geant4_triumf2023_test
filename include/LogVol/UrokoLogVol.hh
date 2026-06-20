#ifndef UROKOLogVol_hh
#define UROKOLogVol_hh 1

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4UserLimits.hh"

class UrokoLogVol
{
  private:
    G4VSolid* Solid;
    G4LogicalVolume* LogVol;
    G4LogicalVolume* LogVol_Scinti; // Scintillator (SD登録用)

  public:
     UrokoLogVol(G4String Name="Uroko", G4UserLimits* fStepLimits=0, G4bool checkOverlaps=true);
    ~UrokoLogVol();

    G4VSolid* GetSolid()         { return Solid;  }
    G4LogicalVolume* GetLogicalVolume() { return LogVol; }
    G4LogicalVolume* GetScintiVolume()  { return LogVol_Scinti; }
};

#endif