#ifndef MagnetLogVol_hh
#define MagnetLogVol_hh 1

#include "globals.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4UserLimits.hh"

class G4UserLimits;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class MagnetLogVol
{
  private:
    G4VSolid*        Solid;
    G4LogicalVolume* LogVol;

  public:
     MagnetLogVol(G4String Name="Magnet", G4UserLimits* fStepLimit=0, G4bool checkOverlaps=true);
    ~MagnetLogVol();

    G4VSolid*        GetSolid()         { return Solid;  }
    G4LogicalVolume* GetLogicalVolume() { return LogVol; }
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif