#include "TrackingAction.hh"

#include "AnalysisConfig.hh"
#include "NeutronTrackInfo.hh"
#include "OpticalPhotonTrackInfo.hh"

#include "G4Exception.hh"
#include "G4HadronicProcessType.hh"
#include "G4Neutron.hh"
#include "G4OpticalPhoton.hh"
#include "G4Track.hh"
#include "G4TrackingManager.hh"
#include "G4VProcess.hh"

#include <utility>

namespace {

    G4bool IsCreatedByNeutronInelastic(const G4Track* track){

        if (track == nullptr){
            return false;
        }

        const auto* creatorProcess = track->GetCreatorProcess();

        if(creatorProcess == nullptr){
            return false;
        }

        return creatorProcess->GetProcessSubType() == fHadronInelastic 
                                || creatorProcess->GetProcessName() == "neutronInelastic";

    }
}

TrackingAction::TrackingAction(std::shared_ptr<const AnalysisConfig> analysisConfig)
    : fAnalysisConfig(std::move(analysisConfig))
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
        "TrackingAction::TrackingAction",
        "TrackingAction001",
        FatalException,
        "AnalysisConfig is null."
        );
    }
}

void TrackingAction::PreUserTrackingAction(const G4Track* track){

    if (track == nullptr){
        return;
    }

    const auto* particle = track->GetDefinition();

    if (particle == G4Neutron::NeutronDefinition()){
        if (track->GetUserInformation() != nullptr){
            return;
        }

        //一次粒子に対してのみ新しい履歴を生成する
        if (track->GetParentID()!= 0){
            return;
        }

        //Neutronのtrackingを有効化
        fpTrackingManager->SetUserTrackInformation(
                            new NeutronTrackInfo(
                                track->GetTrackID()
                            )
        );

        return;
    }


    if (particle != G4OpticalPhoton::OpticalPhotonDefinition()){
        return;
    }

    if (!fAnalysisConfig->IsOpticalDetailEnabled()){
        return;
    }

    if (track->GetUserInformation()!= nullptr){
        return;
    }

    //光学光子のtrackingを有効化
    fpTrackingManager->SetUserTrackInformation(new OpticalPhotonTrackInfo());

}

//基本的に、2次中性子への情報の引き継ぎ処理
void TrackingAction::PostUserTrackingAction(
    const G4Track* track
)
{
    if (track == nullptr) {
        return;
    }

    const auto* parentInfo =
        dynamic_cast<const NeutronTrackInfo*>(
            track->GetUserInformation()
        );

    // NeutronTrackInfoを持たないトラックは対象外
    if (parentInfo == nullptr) {
        return;
    }

    auto* secondaries =
        fpTrackingManager->GimmeSecondaries();

    if (secondaries == nullptr) {
        return;
    }

    const auto& parentHistory =
        parentInfo->GetScatterHistory();

    for (auto* secondary : *secondaries) {
        if (secondary == nullptr) {
            continue;
        }

        // 中性子以外には中性子履歴を渡さない
        if (secondary->GetDefinition()
            != G4Neutron::NeutronDefinition()) {
            continue;
        }

        // 既存のUserInformationを上書きしない
        if (secondary->GetUserInformation()
            != nullptr) {
            continue;
        }

        // 親の散乱履歴を二次中性子へコピー
        auto* childInfo =
            new NeutronTrackInfo(
                *parentInfo
            );

        // 入射前の非弾性散乱によって生成された
        // 二次中性子の場合、その二次中性子の
        // 初期エネルギーを散乱後エネルギーとして保存
        const G4bool shouldUpdateOutgoingEnergy =
            !parentInfo->HasEnteredTarget()
            && IsCreatedByNeutronInelastic(
                secondary
            )
            && !parentHistory.empty()
            && parentHistory.back().process
                == NeutronScatterProcess::Inelastic;

        if (shouldUpdateOutgoingEnergy) {
            childInfo
                ->SetLastScatterOutgoingEnergy(
                    secondary->GetKineticEnergy()
                );
        }

        secondary->SetUserInformation(
            childInfo
        );
    }
}