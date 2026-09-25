#ifndef NEUTRON_STEP_PROCESSOR_HH
#define NEUTRON_STEP_PROCESSOR_HH

#include "NeutronScatterRecord.hh"

#include "G4VTouchable.hh"
#include "globals.hh"

#include <memory>

class AnalysisConfig;
class G4Step;
class G4VPhysicalVolume;
class G4VProcess;


class NeutronStepProcessor {
public:
    explicit NeutronStepProcessor(
        std::shared_ptr<const AnalysisConfig> config
    );
    ~NeutronStepProcessor() = default;

    void Process(
        const G4Step* step
    ) const;

private:
    struct ObjectIdentity {
        GeometryObjectType type =
            GeometryObjectType::Unknown;

        G4int copyNo = -1;
    };
    
    /*
     * このrunで中性子履歴の終点として選択されている
     * LiGlassまたはUROKOかを判定する。
     */
    G4bool IsSelectedTargetScintillator(
        const G4VPhysicalVolume* volume
    ) const;
    

    static NeutronScatterProcess
    ClassifyProcess(
        const G4VProcess* process
    );

    static ObjectIdentity
    FindTopLevelObject(
        const G4VTouchable* touchable
    );

    static GeometryObjectType
    ClassifyTopLevelVolume(
        const G4VPhysicalVolume* volume
    );

    std::shared_ptr<const AnalysisConfig>
        fAnalysisConfig;

};

#endif