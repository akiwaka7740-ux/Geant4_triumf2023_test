#ifndef HILELogVol_hh
#define HILELogVol_hh 1

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4UserLimits.hh"

class HILELogVol
{
  private:
    G4VSolid* Solid;
    G4LogicalVolume* LogVol;
    G4LogicalVolume* LogVol_Scinti;  // シンチレータ（SD登録用）
    G4LogicalVolume* LogVol_Cathode; // カソード（SD登録用）

  public:
     HILELogVol(G4String Name="HILE", G4UserLimits* fStepLimits=0, G4bool checkOverlaps=true);
    ~HILELogVol();

    G4VSolid* GetSolid() { return Solid; }
    G4LogicalVolume* GetLogicalVolume() { return LogVol; }
    G4LogicalVolume* GetScintiVolume()  { return LogVol_Scinti; }
    G4LogicalVolume* GetCathodeVolume() { return LogVol_Cathode; }
};

#endif