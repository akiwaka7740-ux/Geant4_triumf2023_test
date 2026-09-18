#include "CathodeSD.hh"
#include "OpticalPhotonTrackInfo.hh"

#include "G4Track.hh"
#include "G4SystemOfUnits.hh"



namespace {
    constexpr G4int kPmtDepth = 1;
    constexpr G4int kDetectorDepth = 2;
}

CathodeSD::CathodeSD(G4String name)
 : G4VSensitiveDetector(name)
{
}

void CathodeSD::Initialize(G4HCofThisEvent*) {
    fPmtData.clear();

    for (const auto& channelKey : fRegisteredChannels) {
        fPmtData.emplace(channelKey, PmtEventData{});
    }
}


G4bool CathodeSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {

   return false;   

}

G4bool CathodeSD::ProcessBoundaryHit(const G4Step* aStep){

    if(aStep == nullptr){
        return false;
    }

    const auto* track = aStep->GetTrack();
    const auto* postPoint = aStep->GetPostStepPoint();

    if(track == nullptr || postPoint == nullptr){
        return false;
    }

    //steppingActionでCathodeであることは判定済み
    /*
    auto* nextVol = postPoint->GetPhysicalVolume();

    //次に侵入するのがCathodeか判定
    if (nextVol == nullptr || nextVol->GetName() != "Cathode"){
        return false;
    }
    */

    const auto* touchable = postPoint->GetTouchable();

    if (touchable == nullptr){
        return false;
    }

    //GetHistoryDepthは参照可能な最大Depth番号を返す　→ 祖父である検出器が存在するかどうかを確認している
    if (touchable->GetHistoryDepth() < kDetectorDepth){
        return false;
    }

    //検出器とpmtのcopyNoを取得
    const G4int pmtCopyNo = touchable->GetCopyNumber(kPmtDepth);
    const G4int detectorCopyNo = touchable->GetCopyNumber(kDetectorDepth);

    if (detectorCopyNo < 0){
        return false;
    }

    if (pmtCopyNo < 0){
        return false;
    }

    const DetectorKey detectorKey{detectorCopyNo};
    const PmtChannelKey channelKey{detectorKey,pmtCopyNo};

    //fPmtDataからchannelKeyに対応するPmtEventDataを取得し、dataと結びつける
    const auto iter = fPmtData.find(channelKey);

    if (iter == fPmtData.end()) {
        return false;
    }

    PmtEventData& data = iter->second;

    ++data.arrivedPhotons;
    ++data.detectedPhotons;

    data.hitTimes.push_back(postPoint->GetGlobalTime() / ns); // ns単位に変換
    data.transportTimes.push_back(track->GetLocalTime() / ns); // ns単位に変換
    data.trackLengths.push_back(track->GetTrackLength() / mm); // mm単位に変換
    data.hitPositions.push_back(postPoint->GetPosition() /mm); // mm単位に変換

    /*
     * OpticalPhotonTrackInfoの読み出し
     *
     * TrackingActionで正しくTrackInfoが登録されていれば、
     * すべて0以上の値になる。
     *
     * -1はTrackInfoが取得できなかった場合の異常値。
     */
    G4int scintillatorBoundaryCount = -1;
    G4int lightGuideBoundaryCount = -1;

    G4int scintillatorReflectionCount = -1;
    G4int lightGuideReflectionCount = -1;

    const auto* trackInfo = dynamic_cast<const OpticalPhotonTrackInfo*>(
        track->GetUserInformation()
    );

    if (trackInfo != nullptr) {
        scintillatorBoundaryCount = 
            trackInfo->GetScintillatorBoundaryCount();

        lightGuideBoundaryCount =
            trackInfo->GetLightGuideBoundaryCount();

        scintillatorReflectionCount =
            trackInfo->GetScintillatorReflectionCount();

        lightGuideReflectionCount =
            trackInfo->GetLightGuideReflectionCount();
    }


    data.scintillatorBoundaryCounts.push_back(
        scintillatorBoundaryCount
    );

    data.lightGuideBoundaryCounts.push_back(
        lightGuideBoundaryCount
    );

    data.scintillatorReflectionCounts.push_back(
        scintillatorReflectionCount
    );

    data.lightGuideReflectionCounts.push_back(
        lightGuideReflectionCount
    );

    //光子は全て吸収されるものとする
    //aStep->GetTrack()->SetTrackStatus(fStopAndKill);

    return true;

}


const PmtEventData* CathodeSD::FindPmtData(const PmtChannelKey& channelKey) const
{
    const auto iter =
        fPmtData.find(channelKey);

    if (iter == fPmtData.end()) {
        return nullptr;
    }

    return &(iter->second);
}

void CathodeSD::RegisterChannel(const PmtChannelKey& channelKey)
{
    if (channelKey.detector.detectorCopyNo < 0 || channelKey.pmtCopyNo < 0) {
        return;
    }

    fRegisteredChannels.insert(channelKey);
}


void CathodeSD::EndOfEvent(G4HCofThisEvent*) {
}