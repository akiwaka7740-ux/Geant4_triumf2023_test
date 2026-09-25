#ifndef NEUTRON_TRACK_INFO_HH
#define NEUTRON_TRACK_INFO_HH

#include "NeutronScatterRecord.hh"

#include "G4VUserTrackInformation.hh"
#include "globals.hh"

#include <vector>

//1回1回の散乱に対応するNeutronScatterRecordから情報を吸い上げてまとめる

class NeutronTrackInfo final
    : public G4VUserTrackInformation {
public:
    NeutronTrackInfo(
        G4int rootPrimaryTrackId
    );

    NeutronTrackInfo(
        const NeutronTrackInfo& other
    );

    ~NeutronTrackInfo() override = default;

    void RecordScatter(
        const NeutronScatterRecord& record
    );

    void MarkTargetEntry(
        G4int detectorCopyNo,
        G4double entryTime,
        G4double entryEnergy
    );

    void SetLastScatterOutgoingEnergy(
        G4double energy
    );

    G4int GetRootPrimaryTrackId() const
    {
        return fRootPrimaryTrackId;
    }

    G4bool HasEnteredTarget() const
    {
        return fHasEnteredTarget;
    }

    G4int GetEnteredDetectorCopyNo() const
    {
        return fEnteredDetectorCopyNo;
    }

    G4double GetDetectorEntryTime() const
    {
        return fDetectorEntryTime;
    }

    G4double GetDetectorEntryEnergy() const
    {
        return fDetectorEntryEnergy;
    }

    const std::vector<NeutronScatterRecord>&
    GetScatterHistory() const
    {
        return fScatterHistory;
    }

    void Print() const override;

private:
    G4int fRootPrimaryTrackId = -1;

    G4bool fHasEnteredTarget = false;
    G4int fEnteredDetectorCopyNo = -1;

    G4double fDetectorEntryTime = -1.0;

    //検出器入射時のエネルギー（各物体での散乱前後のエネルギーはNeutronScatterRecordが所持している）
    G4double fDetectorEntryEnergy = -1.0;

    std::vector<NeutronScatterRecord>
        fScatterHistory;
};

#endif