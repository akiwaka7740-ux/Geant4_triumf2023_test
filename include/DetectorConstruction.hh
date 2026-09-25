#ifndef DETECTOR_CONSTRUCTION_HH
#define DETECTOR_CONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

#include <memory>


class AnalysisConfig;

class G4LogicalVolume;
class G4VPhysicalVolume;

class LiGlassLogVol;
class UROKOLogVol;
class HILELogVol;
class HPGeLogVol;
class BetaPlasticLogVol;
class MagnetLogVol;
class FrameLogVol;
class FloorLogVol;
class ShieldLogVol;
class ChamberLogVol;
class StopperLogVol;


class DetectorConstruction
    : public G4VUserDetectorConstruction {
public:
    explicit DetectorConstruction(
        std::shared_ptr<const AnalysisConfig> config
    );

    ~DetectorConstruction() override = default;


    G4VPhysicalVolume* Construct() override;

    void ConstructSDandField() override;


private:
    std::shared_ptr<const AnalysisConfig>
        fAnalysisConfig;


    // 各検出器クラスのインスタンス
    LiGlassLogVol* fLiGlass = nullptr;
    UROKOLogVol* fUROKO = nullptr;
    HILELogVol* fHile = nullptr;
    HPGeLogVol* fHPGe = nullptr;
    BetaPlasticLogVol* fBetaPlastic = nullptr;
    MagnetLogVol* fMagnet = nullptr;
    FrameLogVol* fFrame = nullptr;
    FloorLogVol* fFloor = nullptr;
    ShieldLogVol* fShield = nullptr;
    ChamberLogVol* fChamber = nullptr;
    StopperLogVol* fStopper = nullptr;


    G4LogicalVolume* fWorldLogicalVolume =
        nullptr;
};

#endif