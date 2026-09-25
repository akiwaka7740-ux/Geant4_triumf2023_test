#ifndef INCIDENT_NEUTRON_DATA_HH
#define INCIDENT_NEUTRON_DATA_HH

#include "NeutronScatterRecord.hh"

#include "globals.hh"

#include <vector>


/*
 * 検出器内で最初にhadronic反応を起こした
 * 追跡対象中性子の情報。
 *
 * このデータは独立したGeant4 Hitではなく、
 * ScintillatorHitに付随する情報として保持する。
 */
struct IncidentNeutronData {
    /*
     * 有効な中性子情報が保存されているか。
     *
     * 散乱履歴が空でも、無散乱で入射した有効な
     * 中性子である可能性があるため、vectorの
     * empty()だけでは有効性を判定しない。
     */
    G4bool valid = false;


    /*
     * 検出器内で最初にhadronic反応を起こした
     * 中性子のtrack情報。
     */
    G4int trackId = -1;
    G4int parentTrackId = -1;

    /*
     * この中性子系譜の起点となった
     * 一次中性子のtrack ID。
     */
    G4int rootPrimaryTrackId = -1;


    /*
     * 入射した対象検出器のcopyNo。
     */
    G4int detectorCopyNo = -1;


    /*
     * 対象検出器へ最初に入射した時刻と、
     * 入射時点での運動エネルギー。
     *
     * Geant4内部単位のまま保持し、
     * ROOT出力時にns、MeVへ変換する。
     */
    G4double detectorEntryTime = -1.0;
    G4double detectorEntryEnergy = -1.0;


    /*
     * 対象検出器へ入射するまでの散乱履歴。
     *
     * vector内の順序は散乱が発生した時間順。
     */
    std::vector<NeutronScatterRecord>
        preDetectorScatterHistory;
};

#endif