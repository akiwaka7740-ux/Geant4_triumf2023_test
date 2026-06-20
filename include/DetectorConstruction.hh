#ifndef DetectorConstruction_hh
#define DetectorConstruction_hh 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class LigLogVol;
class UrokoLogVol;
class HILELogVol;
class HPGeLogVol;
class MagnetLogVol;
class FrameLogVol;
class FloorLogVol;
class ShieldLogVol;
class ChamberLogVol;
class StopperLogVol;
class BetaPlasticLogVol;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

  private:
    // 各検出器クラスのインスタンス
    LigLogVol* fLig;
    UrokoLogVol* fUroko;
    HILELogVol* fHile;
    HPGeLogVol* fHPGe;
    BetaPlasticLogVol* fBetaPlastic;
    MagnetLogVol* fMagnet;
    FrameLogVol* fFrame;
    FloorLogVol* fFloor;
    ShieldLogVol* fShield;
    ChamberLogVol* fChamber;
    StopperLogVol* fStopper;
    

    G4LogicalVolume* World_LogVol;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif