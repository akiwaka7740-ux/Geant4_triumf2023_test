#ifndef LigLogVol_hh
#define LigLogVol_hh 1

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4UserLimits.hh"
#include "G4OpticalSurface.hh"

class G4UserLimits;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class LigLogVol
{
  private:
    G4VSolid* Solid;
    G4LogicalVolume* LogVol;
    G4LogicalVolume* LogVol0; // Scintillator (SD登録用)
    G4LogicalVolume* LogVol1; // Cathode (SD登録用)
    G4LogicalVolume* LogVol2; // PMT (SD登録用)

  public:
     LigLogVol(G4String Name="Lig", G4UserLimits* fStepLimits=0, G4bool checkOverlaps=true);
    ~LigLogVol();

    G4VSolid* GetSolid()         { return Solid;  }
    G4LogicalVolume* GetLogicalVolume() { return LogVol; }
    G4LogicalVolume* GetScintiVolume()  { return LogVol0; } 
    G4LogicalVolume* GetCathodeVolume() { return LogVol1; } 
    G4LogicalVolume* GetPMTVolume() {return LogVol2; }
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
