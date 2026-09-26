#ifndef RUNCONDITIONS_HH
#define RUNCONDITIONS_HH

#include "globals.hh"
#include "G4ThreeVector.hh"

struct RunConditions {
    G4int runId = -1;

    // 粒子情報
    G4String particleName;
    G4int pdgCode = 0;
    G4double particleCharge = 0.0;

    // イオンの場合に使用
    G4bool isIon = false;
    G4int atomicNumber = 0;
    G4int massNumber = 0;
    G4double excitationEnergy = 0.0;

    // 初期実装では単一 source を対象とする
    G4int sourceCount = 0;
    G4int particlesPerEvent = 0;

    // エネルギー分布
    G4String energyDistribution;
    G4double monoEnergy = 0.0;

    // 位置分布
    G4String positionDistribution;
    G4ThreeVector positionCentre;

    // 角度分布
    G4String angularDistribution;
    G4ThreeVector fixedDirection;

    G4double minTheta = 0.0;
    G4double maxTheta = 0.0;
    G4double minPhi = 0.0;
    G4double maxPhi = 0.0;
};

#endif