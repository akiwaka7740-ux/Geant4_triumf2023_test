#ifndef BetaPlasticLogVol_hh
#define BetaPlasticLogVol_hh 1

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4UserLimits.hh"
#include "G4OpticalSurface.hh"

class G4UserLimits;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class BetaPlasticLogVol
{
  private:
    G4VSolid*         Solid;
    G4LogicalVolume*  LogVol;

  public:
     BetaPlasticLogVol(G4String Name="BetaPlastic", G4UserLimits* fStepLimits=0, G4bool checkOverlaps=true);
    ~BetaPlasticLogVol();

    G4VSolid*         GetSolid()         { return Solid;  }
    G4LogicalVolume*  GetLogicalVolume() { return LogVol; }
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
