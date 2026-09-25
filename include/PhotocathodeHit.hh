#ifndef PHOTOCATHODE_HIT_HH
#define PHOTOCATHODE_HIT_HH

#include "DetectorChannelKey.hh"

#include "G4Allocator.hh"
#include "G4THitsCollection.hh"
#include "G4ThreeVector.hh"
#include "G4VHit.hh"
#include "globals.hh"

#include <cstddef>
#include <vector>


class PhotocathodeHit : public G4VHit {
public:
    explicit PhotocathodeHit(
        const PmtChannelKey& channelKey
    );

    PhotocathodeHit(
        const PhotocathodeHit& other
    ) = default;

    ~PhotocathodeHit() override = default;

    PhotocathodeHit& operator=(
        const PhotocathodeHit& other
    ) = default;

    G4bool operator==(
        const PhotocathodeHit& other
    ) const;


    void* operator new(std::size_t);
    void operator delete(void* hit);


    void Print() override;


    const PmtChannelKey&
    GetChannelKey() const
    {
        return fChannelKey;
    }


    /*
     * 光電面へ到達した光子数。
     */
    void IncrementArrivedPhotons()
    {
        ++fArrivedPhotons;
    }

    G4int GetArrivedPhotons() const
    {
        return fArrivedPhotons;
    }


    /*
     * 光電面でDetectionとなった光子数。
     */
    void IncrementDetectedPhotons()
    {
        ++fDetectedPhotons;
    }

    G4int GetDetectedPhotons() const
    {
        return fDetectedPhotons;
    }


    /*
     * Detailedモードでのみ呼び出す。
     *
     * 同じ添字が同じ光学光子を表すように、
     * すべてのvectorへ1回の関数呼び出しで
     * データを追加する。
     */
    void AppendDetectedPhotonDetail(
        G4double hitTime,
        G4double transportTime,
        G4double trackLength,
        const G4ThreeVector& hitPosition,
        G4int scintillatorBoundaryCount,
        G4int lightGuideBoundaryCount,
        G4int scintillatorReflectionCount,
        G4int lightGuideReflectionCount
    )
    {
        fHitTimes.push_back(hitTime);
        fTransportTimes.push_back(transportTime);
        fTrackLengths.push_back(trackLength);
        fHitPositions.push_back(hitPosition);

        fScintillatorBoundaryCounts.push_back(
            scintillatorBoundaryCount
        );

        fLightGuideBoundaryCounts.push_back(
            lightGuideBoundaryCount
        );

        fScintillatorReflectionCounts.push_back(
            scintillatorReflectionCount
        );

        fLightGuideReflectionCounts.push_back(
            lightGuideReflectionCount
        );
    }


    const std::vector<G4double>&
    GetHitTimes() const
    {
        return fHitTimes;
    }

    const std::vector<G4double>&
    GetTransportTimes() const
    {
        return fTransportTimes;
    }

    const std::vector<G4double>&
    GetTrackLengths() const
    {
        return fTrackLengths;
    }

    const std::vector<G4ThreeVector>&
    GetHitPositions() const
    {
        return fHitPositions;
    }

    const std::vector<G4int>&
    GetScintillatorBoundaryCounts() const
    {
        return fScintillatorBoundaryCounts;
    }

    const std::vector<G4int>&
    GetLightGuideBoundaryCounts() const
    {
        return fLightGuideBoundaryCounts;
    }

    const std::vector<G4int>&
    GetScintillatorReflectionCounts() const
    {
        return fScintillatorReflectionCounts;
    }

    const std::vector<G4int>&
    GetLightGuideReflectionCounts() const
    {
        return fLightGuideReflectionCounts;
    }


    /*
     * Detailed用vectorがすべて同じ長さか確認する。
     * EventActionまたはデバッグ時の整合性確認に使用する。
     */
    G4bool HasConsistentDetailSizes() const
    {
        const auto size = fHitTimes.size();

        return
            fTransportTimes.size() == size &&
            fTrackLengths.size() == size &&
            fHitPositions.size() == size &&
            fScintillatorBoundaryCounts.size() == size &&
            fLightGuideBoundaryCounts.size() == size &&
            fScintillatorReflectionCounts.size() == size &&
            fLightGuideReflectionCounts.size() == size;
    }


private:
    PmtChannelKey fChannelKey;


    // SummaryとDetailedの両方で使用する集計値
    G4int fArrivedPhotons = 0;
    G4int fDetectedPhotons = 0;


    // Detailedモードだけで使用する光子ごとの情報
    std::vector<G4double> fHitTimes;
    std::vector<G4double> fTransportTimes;
    std::vector<G4double> fTrackLengths;

    std::vector<G4ThreeVector> fHitPositions;

    std::vector<G4int>
        fScintillatorBoundaryCounts;

    std::vector<G4int>
        fLightGuideBoundaryCounts;

    std::vector<G4int>
        fScintillatorReflectionCounts;

    std::vector<G4int>
        fLightGuideReflectionCounts;
};


using PhotocathodeHitsCollection =
    G4THitsCollection<PhotocathodeHit>;


extern G4ThreadLocal
G4Allocator<PhotocathodeHit>*
    PhotocathodeHitAllocator;


inline void* PhotocathodeHit::operator new(
    std::size_t
)
{
    if (PhotocathodeHitAllocator == nullptr) {
        PhotocathodeHitAllocator =
            new G4Allocator<PhotocathodeHit>;
    }

    return PhotocathodeHitAllocator
        ->MallocSingle();
}


inline void PhotocathodeHit::operator delete(
    void* hit
)
{
    PhotocathodeHitAllocator->FreeSingle(
        static_cast<PhotocathodeHit*>(hit)
    );
}

#endif