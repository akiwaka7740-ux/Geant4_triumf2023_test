#ifndef NEUTRON_SCATTER_RECORD_HH
#define NEUTRON_SCATTER_RECORD_HH

#include "GeometryObjectType.hh"

#include "globals.hh"

//ある散乱1回に対して記録するための構造体の定義

enum class NeutronScatterProcess : G4int {
    Unknown   = 0,
    Elastic   = 1,
    Inelastic = 2
};

struct NeutronScatterRecord {
    // この散乱を起こした中性子トラック
    G4int trackId = -1;
    G4int parentTrackId = -1;

    // 散乱した最上位装置
    GeometryObjectType objectType =
        GeometryObjectType::Unknown;

    G4int objectCopyNo = -1;

    // 散乱プロセス
    NeutronScatterProcess process =
        NeutronScatterProcess::Unknown;

    // Geant4内部単位で保持する
    G4double globalTime = -1.0;
    G4double kineticEnergyBefore = -1.0;
    G4double kineticEnergyAfter = -1.0;
};

#endif