#ifndef FrameLogVol_hh
#define FrameLogVol_hh 1

#include "globals.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4UserLimits.hh"

class G4UserLimits;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class FrameLogVol
{
  private:
    G4VSolid* Solid;
    G4LogicalVolume* LogVol;

  public:
     FrameLogVol(G4String Name="Frame", G4UserLimits* fStepLimit=0, G4bool checkOverlaps=true);
    ~FrameLogVol();

    G4VSolid*        GetSolid()         { return Solid;  }
    G4LogicalVolume* GetLogicalVolume() { return LogVol; }
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
