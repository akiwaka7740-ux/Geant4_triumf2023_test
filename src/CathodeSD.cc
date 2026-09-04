#include "CathodeSD.hh"

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

    const auto* postPoint = aStep->GetPostStepPoint();

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

    data.hitTimes.push_back(postPoint->GetGlobalTime());

    data.hitPositions.push_back(postPoint->GetPosition());

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