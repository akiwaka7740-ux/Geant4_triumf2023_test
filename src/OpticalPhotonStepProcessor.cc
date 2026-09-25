#include "OpticalPhotonStepProcessor.hh"

#include "AnalysisConfig.hh"
#include "PhotocathodeSD.hh"

#include "G4Exception.hh"
#include "G4LogicalVolume.hh"
#include "G4OpticalPhoton.hh"
#include "G4ProcessManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4StepStatus.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"

#include <utility>


OpticalPhotonStepProcessor::
OpticalPhotonStepProcessor(
    std::shared_ptr<const AnalysisConfig> config
)
    : fAnalysisConfig(std::move(config))
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "OpticalPhotonStepProcessor::"
            "OpticalPhotonStepProcessor",
            "OpticalPhotonStepProcessor001",
            FatalException,
            "AnalysisConfig is null."
        );
    }
}


// 全体の処理を司る部分
void OpticalPhotonStepProcessor::Process(
    const G4Step* step
)
{
    /*
     * Offでは光学光子の輸送自体は継続するが、
     * 解析用情報は一切記録しない。
     */
    if (fAnalysisConfig == nullptr ||
        !fAnalysisConfig
            ->IsOpticalRecordingEnabled()) {
        return;
    }

    if (step == nullptr) {
        return;
    }

    auto* track = step->GetTrack();

    if (track == nullptr) {
        return;
    }

    if (track->GetDefinition() !=
        G4OpticalPhoton::OpticalPhotonDefinition()) {
        return;
    }

    const auto* prePoint =
        step->GetPreStepPoint();

    const auto* postPoint =
        step->GetPostStepPoint();

    if (prePoint == nullptr ||
        postPoint == nullptr) {
        return;
    }

    auto* boundary =
        FindBoundaryProcess(track);

    if (boundary == nullptr) {
        return;
    }

    const G4StepStatus stepStatus =
        postPoint->GetStepStatus();

    const G4OpBoundaryProcessStatus
        boundaryStatus =
            stepStatus == fGeomBoundary
            ? boundary->GetStatus()
            : NotAtBoundary;


    /*
     * Detailedの場合だけ、光子ごとの
     * 境界通過回数と反射回数を記録する。
     */
    if (fAnalysisConfig
            ->IsOpticalDetailEnabled() &&
        stepStatus == fGeomBoundary) {

        const OpticalPhotonRegion preRegion =
            ClassifyVolume(
                prePoint->GetPhysicalVolume()
            );

        if (preRegion !=
            OpticalPhotonRegion::Other &&
            boundaryStatus != Undefined &&
            boundaryStatus != NotAtBoundary &&
            boundaryStatus != StepTooSmall) {

            auto* trackInfo =
                dynamic_cast<
                    OpticalPhotonTrackInfo*
                >(
                    track->GetUserInformation()
                );

            if (trackInfo != nullptr) {
                trackInfo
                    ->RecordBoundaryInteraction(
                        preRegion,
                        IsReflection(boundaryStatus)
                    );
            }
        }
    }


    /*
    * SummaryとDetailedの両方で、
    * 光電面への到達数と検出数を記録する。
    *
    * Offの場合は関数冒頭で終了しているため、
    * PhotocathodeSDは呼ばれない。
    */
    ProcessPhotocathodeBoundary(
        step,
        boundaryStatus
    );
}


G4OpBoundaryProcess*
OpticalPhotonStepProcessor::FindBoundaryProcess(
    const G4Track* track
)
{
    if (fBoundary != nullptr) {
        return fBoundary;
    }

    if (track == nullptr) {
        return nullptr;
    }

    const auto* particleDefinition =
        track->GetDefinition();

    if (particleDefinition == nullptr) {
        return nullptr;
    }

    auto* processManager =
        particleDefinition->GetProcessManager();

    if (processManager == nullptr) {
        return nullptr;
    }

    auto* processList =
        processManager->GetProcessList();

    if (processList == nullptr) {
        return nullptr;
    }

    const G4int processCount =
        processManager->GetProcessListLength();

    for (G4int index = 0;
         index < processCount;
         ++index) {

        auto* process =
            (*processList)[index];

        if (process == nullptr) {
            continue;
        }

        if (process->GetProcessName() !=
            "OpBoundary") {
            continue;
        }

        fBoundary =
            dynamic_cast<G4OpBoundaryProcess*>(
                process
            );

        if (fBoundary != nullptr) {
            return fBoundary;
        }
    }

    return nullptr;
}


OpticalPhotonRegion
OpticalPhotonStepProcessor::ClassifyVolume(
    const G4VPhysicalVolume* volume
)
{
    if (volume == nullptr) {
        return OpticalPhotonRegion::Other;
    }

    const auto* logicalVolume =
        volume->GetLogicalVolume();

    if (logicalVolume == nullptr) {
        return OpticalPhotonRegion::Other;
    }

    const auto& logicalName =
        logicalVolume->GetName();


    // UROKOのシンチレータ
    if (logicalName == "UROKO_LV_Scinti") {
        return OpticalPhotonRegion::Scintillator;
    }

    // UROKOのライトガイド
    if (logicalName == "UROKO_LV_Guide") {
        return OpticalPhotonRegion::LightGuide;
    }

    // LiGlassのシンチレータ
    if (logicalName == "LiGlass_LogVol0") {
        return OpticalPhotonRegion::Scintillator;
    }

    return OpticalPhotonRegion::Other;
}


G4bool OpticalPhotonStepProcessor::IsReflection(
    G4OpBoundaryProcessStatus status
)
{
    switch (status) {
    case FresnelReflection:
    case TotalInternalReflection:
    case LambertianReflection:
    case LobeReflection:
    case SpikeReflection:
    case BackScattering:
        return true;

    default:
        return false;
    }
}


// PhotocathodeSDを呼び出すための判定を行う
void OpticalPhotonStepProcessor::
ProcessPhotocathodeBoundary(
    const G4Step* step,
    G4OpBoundaryProcessStatus boundaryStatus
)
{
    if (step == nullptr) {
        return;
    }

    const auto* postPoint =
        step->GetPostStepPoint();

    if (postPoint == nullptr) {
        return;
    }

    /*
     * 光学境界で発生したステップだけを対象にする。
     */
    if (postPoint->GetStepStatus() !=
        fGeomBoundary) {
        return;
    }

    /*
     * 有効な境界相互作用でない状態を除外する。
     */
    if (boundaryStatus == Undefined ||
        boundaryStatus == NotAtBoundary ||
        boundaryStatus == StepTooSmall) {
        return;
    }

    auto* postVolume =
        postPoint->GetPhysicalVolume();

    if (postVolume == nullptr) {
        return;
    }

    auto* logicalVolume =
        postVolume->GetLogicalVolume();

    if (logicalVolume == nullptr) {
        return;
    }

    auto* sensitiveDetector =
        logicalVolume->GetSensitiveDetector();

    auto* photocathodeSD =
        dynamic_cast<PhotocathodeSD*>(
            sensitiveDetector
        );

    /*
     * post volumeがPhotocathodeSDを持たない場合、
     * 通常の光学境界なので記録しない。
     */
    if (photocathodeSD == nullptr) {
        return;
    }

    const G4bool detected =
        boundaryStatus == Detection;

    photocathodeSD
        ->ProcessBoundaryInteraction(
            step,
            detected
        );
}