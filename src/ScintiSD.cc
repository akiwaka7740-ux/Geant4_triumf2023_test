#include "ScintiSD.hh"

#include "G4OpticalPhoton.hh"
#include "G4EmSaturation.hh"
#include "G4LossTableManager.hh"
#include "G4Neutron.hh"
#include "G4VProcess.hh"
#include "G4ProcessType.hh"


namespace {
    //CathodeSD.ccと異なるので注意
    constexpr G4int kDetectorDepth = 1;
}

ScintiSD::ScintiSD(G4String name)
 : G4VSensitiveDetector(name)
{
}



void ScintiSD::Initialize(G4HCofThisEvent*) {
    fScintiData.clear();
}

G4bool ScintiSD::ProcessHits(G4Step* step, G4TouchableHistory*){

    // 1. G4Stepと関連オブジェクトを取得
    if (step == nullptr) {
        return false;
    }

    const auto* track = step->GetTrack();
    const auto* prePoint = step->GetPreStepPoint();
    const auto* postPoint = step->GetPostStepPoint();

    if (track == nullptr
        || prePoint == nullptr
        || postPoint == nullptr) {
        return false;
    }

    // 2. このstepが発生した検出器を識別
    const auto* touchable = prePoint->GetTouchable();

    if (touchable == nullptr) {
        return false;
    }

    if (touchable->GetHistoryDepth() < kDetectorDepth) {
        return false;
    }

    const G4int detectorCopyNo =touchable->GetCopyNumber(kDetectorDepth);

    if (detectorCopyNo < 0) {
        return false;
    }

    const DetectorKey detectorKey{detectorCopyNo};

    // 3. 対象検出器のイベントデータを取得
    ScintiEventData& data = fScintiData[detectorKey];

    // 4.　一次中性子のhadronic反応を判定
    const G4VProcess* process = postPoint->GetProcessDefinedStep();

    const G4bool isNeutron =
        track->GetDefinition()
        == G4Neutron::NeutronDefinition();

    const G4bool isPrimaryNeutron =
    isNeutron && track->GetParentID() == 0;

    const G4bool isHadronicInteraction =
        process != nullptr
        && process->GetProcessType() == fHadronic;

    
    if (isPrimaryNeutron && isHadronicInteraction) {

        // 最初の中性子hadronic反応位置
        if (data.neutronInteractionCount == 0) {
            data.firstHitPosGlobal = postPoint->GetPosition();
            data.firstHitTime= postPoint->GetGlobalTime();
                
            const G4AffineTransform& transform =
                touchable->GetHistory()->GetTopTransform();

            data.firstHitPosLocal =
                transform.TransformPoint(
                    data.firstHitPosGlobal
                );
        }

        ++data.neutronInteractionCount;
    }


    // 以下はエネルギー付与の処理
    const G4double edep =
        step->GetTotalEnergyDeposit();

    if (edep == 0.) {
        return false;
    }

    auto* saturation =
        G4LossTableManager::Instance()->EmSaturation();

    const G4double evis =
        saturation->VisibleEnergyDepositionAtAStep(step);

    data.totalEdep += edep;
    data.totalEvis += evis;

    // Scintillation光の取得
    const auto* secondaries =
        step->GetSecondaryInCurrentStep();

    if (secondaries) {
        for (const auto* secondary : *secondaries) {
            const auto* creator =
                secondary->GetCreatorProcess();

            if (secondary->GetDefinition()
                    == G4OpticalPhoton::OpticalPhotonDefinition()
                && creator
                && creator->GetProcessName()
                    == "Scintillation") {
                ++data.generatedPhotons;
            }
        }
    }

    return true;
}

const ScintiEventData* ScintiSD::FindScintiData(const DetectorKey& detectorKey) const{

    const auto iter = fScintiData.find(detectorKey);

    if (iter == fScintiData.end()){
        return nullptr;
    }

    return &(iter->second);
}

void ScintiSD::EndOfEvent(G4HCofThisEvent*) {
}