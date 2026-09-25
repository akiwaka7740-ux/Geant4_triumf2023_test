#ifndef OPTICAL_PHOTON_STEP_PROCESSOR_HH
#define OPTICAL_PHOTON_STEP_PROCESSOR_HH

#include "OpticalPhotonTrackInfo.hh"

#include "G4OpBoundaryProcess.hh"
#include "globals.hh"

#include <memory>


class AnalysisConfig;
class G4Step;
class G4Track;
class G4VPhysicalVolume;


class OpticalPhotonStepProcessor {
public:
    explicit OpticalPhotonStepProcessor(
        std::shared_ptr<const AnalysisConfig> config
    );

    ~OpticalPhotonStepProcessor() = default;

    void Process(
        const G4Step* step
    );


private:
    /*
     * 光学光子に登録されているOpBoundaryプロセスを探す。
     * 最初に見つけたポインタはfBoundaryへ保持する。
     */
    G4OpBoundaryProcess* FindBoundaryProcess(
        const G4Track* track
    );


    /*
     * 現在の物理ボリュームを、
     * シンチレータ、ライトガイド、その他に分類する。
     */
    static OpticalPhotonRegion ClassifyVolume(
        const G4VPhysicalVolume* volume
    );


    /*
     * OpBoundaryの状態が反射に該当するか判定する。
     */
    static G4bool IsReflection(
        G4OpBoundaryProcessStatus status
    );


    /*
     * Cathode表面に到達した際に、
     * PhotocathodeSDへ処理を委譲する → detectionの判定も一緒に渡す
     */
    static void ProcessPhotocathodeBoundary(
        const G4Step* step,
        G4OpBoundaryProcessStatus boundaryStatus
    );


    std::shared_ptr<const AnalysisConfig>
        fAnalysisConfig;

    G4OpBoundaryProcess* fBoundary = nullptr;
};

#endif