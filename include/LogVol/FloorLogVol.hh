#ifndef FloorLogVol_hh
#define FloorLogVol_hh 1

#include "globals.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4UserLimits.hh"

#include "G4SystemOfUnits.hh"

class G4UserLimits;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class FloorLogVol
{
  private:
    G4VSolid*        Solid;
    G4LogicalVolume* LogVol;

  public:
     FloorLogVol(G4String Name="Floor", G4UserLimits* fStepLimit=0, G4bool checkOverlaps=true,
         double wx=1*m, double wy=1*m, double wz=1*m);
    ~FloorLogVol();

    G4VSolid        *GetSolid()         { return Solid;  }
    G4LogicalVolume *GetLogicalVolume() { return LogVol; }
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
