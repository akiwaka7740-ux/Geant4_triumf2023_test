#ifndef ShieldLogVol_hh
#define ShieldLogVol_hh 1

#include "globals.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4UserLimits.hh"

class G4UserLimits;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class ShieldLogVol
{
  private:
    G4VSolid* Solid0;
    G4VSolid* Solid1;
    G4VSolid* Solid2;
    G4LogicalVolume* LogVol0;
    G4LogicalVolume* LogVol1;
    G4LogicalVolume* LogVol2;

  public:
     ShieldLogVol(G4String Name="Shield", G4UserLimits* fStepLimit=0, G4bool checkOverlaps=true);
    ~ShieldLogVol();

    G4VSolid* GetSolid(int num=0)         
    { return (num==1) ? Solid1 : (num==2) ? Solid2 : Solid0;  }
    G4LogicalVolume* GetLogicalVolume(int num=0) 
    { return (num==1) ? LogVol1 : (num==2) ? LogVol2 : LogVol0;  }
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
