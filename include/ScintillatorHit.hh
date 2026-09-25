#ifndef SCINTILLATOR_HIT_HH
#define SCINTILLATOR_HIT_HH

#include "DetectorChannelKey.hh"
#include "IncidentNeutronData.hh"

#include "G4Allocator.hh"
#include "G4THitsCollection.hh"
#include "G4ThreeVector.hh"
#include "G4VHit.hh"
#include "globals.hh"

#include <cstddef>


class ScintillatorHit : public G4VHit {
public:
    explicit ScintillatorHit(
        const DetectorKey& detectorKey
    );

    ScintillatorHit(
        const ScintillatorHit& other
    ) = default;

    ~ScintillatorHit() override = default;

    ScintillatorHit& operator=(
        const ScintillatorHit& other
    ) = default;

    G4bool operator==(
        const ScintillatorHit& other
    ) const;


    void* operator new(std::size_t);
    void operator delete(void* hit);


    void Print() override;


    const DetectorKey& GetDetectorKey() const
    {
        return fDetectorKey;
    }


    void AddEnergyDeposit(
        G4double energyDeposit,
        G4double visibleEnergyDeposit
    )
    {
        fTotalEdep += energyDeposit;
        fTotalEvis += visibleEnergyDeposit;
    }

    G4double GetTotalEdep() const
    {
        return fTotalEdep;
    }

    G4double GetTotalEvis() const
    {
        return fTotalEvis;
    }


    void IncrementGeneratedPhotons()
    {
        ++fGeneratedPhotons;
    }

    G4int GetGeneratedPhotons() const
    {
        return fGeneratedPhotons;
    }


    G4bool HasFirstHit() const
    {
        return fFirstHitTime >= 0.0;
    }

    void SetFirstHit(
        G4double time,
        const G4ThreeVector& globalPosition,
        const G4ThreeVector& localPosition
    )
    {
        if (HasFirstHit()) {
            return;
        }

        fFirstHitTime = time;
        fFirstHitPosGlobal = globalPosition;
        fFirstHitPosLocal = localPosition;
    }

    G4double GetFirstHitTime() const
    {
        return fFirstHitTime;
    }

    const G4ThreeVector&
    GetFirstHitPosGlobal() const
    {
        return fFirstHitPosGlobal;
    }

    const G4ThreeVector&
    GetFirstHitPosLocal() const
    {
        return fFirstHitPosLocal;
    }

/*
 * parentID == 0の一次中性子による
 * hadronic反応回数。
 */
void IncrementPrimaryNeutronInteractionCount()
{
    ++fPrimaryNeutronInteractionCount;
}

G4int
GetPrimaryNeutronInteractionCount() const
{
    return fPrimaryNeutronInteractionCount;
}


/*
 * NeutronTrackInfoを共有する中性子系譜全体の
 * hadronic反応回数。
 *
 * 一次中性子と、その一次中性子から生成された
 * 二次以降の中性子を含む。
 */
void IncrementNeutronLineageInteractionCount()
{
    ++fNeutronLineageInteractionCount;
}

G4int
GetNeutronLineageInteractionCount() const
{
    return fNeutronLineageInteractionCount;
}



    void MarkNeutronCapture()
    {
        fHasNeutronCapture = true;
    }

    G4bool HasNeutronCapture() const
    {
        return fHasNeutronCapture;
    }


    void SetIncidentNeutronData(
        const IncidentNeutronData& data
    )
    {
        /*
         * 最初に採用された中性子履歴を維持する。
         */
        if (fIncidentNeutron.valid) {
            return;
        }

        fIncidentNeutron = data;
    }

    G4bool HasIncidentNeutronData() const
    {
        return fIncidentNeutron.valid;
    }

    const IncidentNeutronData&
    GetIncidentNeutronData() const
    {
        return fIncidentNeutron;
    }


private:
    DetectorKey fDetectorKey;


    // シンチレータ内のエネルギー付与
    G4double fTotalEdep = 0.0;
    G4double fTotalEvis = 0.0;


    // 生成されたシンチレーション光子数
    G4int fGeneratedPhotons = 0;


    // 最初の追跡対象中性子反応
    G4double fFirstHitTime = -1.0;

    G4ThreeVector fFirstHitPosGlobal{
        -99999.0,
        -99999.0,
        -99999.0
    };

    G4ThreeVector fFirstHitPosLocal{
        -99999.0,
        -99999.0,
        -99999.0
    };


    // parentID == 0の一次中性子だけ
    G4int fPrimaryNeutronInteractionCount = 0;

    // NeutronTrackInfoを共有する一次・二次以降の中性子
    G4int fNeutronLineageInteractionCount = 0;

    G4bool fHasNeutronCapture = false;


    /*
     * このシンチレータ応答に対応付けられた
     * 入射中性子情報。
     */
    IncidentNeutronData fIncidentNeutron;
};


using ScintillatorHitsCollection =
    G4THitsCollection<ScintillatorHit>;


extern G4ThreadLocal
G4Allocator<ScintillatorHit>*
    ScintillatorHitAllocator;


inline void* ScintillatorHit::operator new(
    std::size_t
)
{
    if (ScintillatorHitAllocator == nullptr) {
        ScintillatorHitAllocator =
            new G4Allocator<ScintillatorHit>;
    }

    return ScintillatorHitAllocator
        ->MallocSingle();
}


inline void ScintillatorHit::operator delete(
    void* hit
)
{
    ScintillatorHitAllocator->FreeSingle(
        static_cast<ScintillatorHit*>(hit)
    );
}

#endif