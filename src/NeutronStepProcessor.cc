#include "NeutronStepProcessor.hh"

#include "AnalysisConfig.hh"
#include "NeutronTrackInfo.hh"

#include "G4HadronicProcessType.hh"
#include "G4LogicalVolume.hh"
#include "G4Neutron.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4StepStatus.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"
#include "G4VTouchable.hh"
#include "G4Exception.hh"

#include <utility>

NeutronStepProcessor::NeutronStepProcessor(
    std::shared_ptr<const AnalysisConfig> config
)
    : fAnalysisConfig(std::move(config))
{
    if (fAnalysisConfig == nullptr) {
        G4Exception(
            "NeutronStepProcessor::"
            "NeutronStepProcessor",
            "NeutronStepProcessor001",
            FatalException,
            "AnalysisConfig is null."
        );
    }
}

void NeutronStepProcessor::Process(const G4Step* step) const
{
    if (step == nullptr){
        return;
    }

    const auto* track = step->GetTrack();
    const auto* prePoint = step->GetPreStepPoint();
    const auto* postPoint = step->GetPostStepPoint();

    if (track == nullptr ||
        prePoint == nullptr ||
        postPoint == nullptr) {
        return;
    }

    // 中性子以外は処理しない
    if (track->GetDefinition() != G4Neutron::NeutronDefinition()) {
        return;
    }

    auto* neutronInfo = dynamic_cast<NeutronTrackInfo*>(track->GetUserInformation());

    if (neutronInfo == nullptr) {
        return;
    }

    /*
    * 選択された対象検出器へすでに入射している場合、
    * それ以降の散乱を入射前履歴へ追加しない。
    *
    * 対象でない中性子検出器へ入射しただけでは、
    * HasEnteredTargetはtrueにならない。
    */
    if (neutronInfo->HasEnteredTarget()) {
        return;
    }

    const auto* preVolume = prePoint->GetPhysicalVolume();
    const auto* postVolume = postPoint->GetPhysicalVolume();

    /*
     * 中性子が標的内部から生成された場合や、
     * 境界入射ステップを何らかの理由で取得できなかった場合への保険。
     */
    if (IsSelectedTargetScintillator(preVolume)) {
        const auto target =
            FindTopLevelObject(prePoint->GetTouchable());

        neutronInfo->MarkTargetEntry(
            target.copyNo,
            prePoint->GetGlobalTime(),
            prePoint->GetKineticEnergy()
        );

        return;
    }

    /*
     * 今回のステップを発生させた物理過程を調べる。
     * TransportationなどはUnknownになり、散乱としては記録されない。
     */
    const auto scatterProcess =
        ClassifyProcess(postPoint->GetProcessDefinedStep());

    if (scatterProcess != NeutronScatterProcess::Unknown) {
        const auto object =
            FindTopLevelObject(prePoint->GetTouchable());

        NeutronScatterRecord record;

        record.trackId = track->GetTrackID();
        record.parentTrackId = track->GetParentID();

        record.objectType = object.type;
        record.objectCopyNo = object.copyNo;

        record.process = scatterProcess;

        record.globalTime = postPoint->GetGlobalTime();
        record.kineticEnergyBefore = prePoint->GetKineticEnergy();
        record.kineticEnergyAfter = postPoint->GetKineticEnergy();

        neutronInfo->RecordScatter(record);
    }

    /*
     * Li-glassまたはUROKOへの境界入射を記録する。
     */
    if (postPoint->GetStepStatus() != fGeomBoundary) {
        return;
    }

    if (!IsSelectedTargetScintillator(postVolume)) {
        return;
    }

    const auto target =
        FindTopLevelObject(postPoint->GetTouchable());

    neutronInfo->MarkTargetEntry(
        target.copyNo,
        postPoint->GetGlobalTime(),
        postPoint->GetKineticEnergy()
    );
}


G4bool
NeutronStepProcessor::IsSelectedTargetScintillator(
    const G4VPhysicalVolume* volume
) const
{
    if (volume == nullptr) {
        return false;
    }

    const auto* logicalVolume =
        volume->GetLogicalVolume();

    if (logicalVolume == nullptr) {
        return false;
    }

    if (fAnalysisConfig == nullptr) {
        return false;
    }

    const auto& logicalName =
        logicalVolume->GetName();

    const GeometryObjectType target =
        fAnalysisConfig->GetNeutronHistoryTarget();

    switch (target) {
    case GeometryObjectType::LiGlass:
        return logicalName ==
            "LiGlass_LogVol0";

    case GeometryObjectType::UROKO:
        return logicalName ==
            "UROKO_LV_Scinti";

    default:
        return false;
    }
}

NeutronScatterProcess NeutronStepProcessor::ClassifyProcess(const G4VProcess* process)
{
    if (process == nullptr) {
        return NeutronScatterProcess::Unknown;
    }

    const G4int processSubtype = process->GetProcessSubType();
    const auto& processName = process->GetProcessName();

    if (processSubtype == fHadronElastic ||
        processName == "hadElastic") {
        return NeutronScatterProcess::Elastic;
    }

    if (processSubtype == fHadronInelastic ||
        processName == "neutronInelastic") {
        return NeutronScatterProcess::Inelastic;
    }

    return NeutronScatterProcess::Unknown;
}

NeutronStepProcessor::ObjectIdentity
NeutronStepProcessor::FindTopLevelObject(const G4VTouchable* touchable)
{
    ObjectIdentity result;

    if (touchable == nullptr){
        return result;
    }

    const G4int historyDepth = touchable->GetHistoryDepth();

    const auto* currentVolume = touchable->GetVolume(0);

    if (ClassifyTopLevelVolume(currentVolume) == GeometryObjectType::World) {
        result.type = GeometryObjectType::World;
        result.copyNo = touchable->GetCopyNumber(0);
        return result;
    }

    // worldの一つ前が、今決定したいGeやMagnetのようなobjectに該当
    for (G4int depth = 0; depth < historyDepth; ++depth) {
        const auto* volume = touchable->GetVolume(depth);
        const auto* parentVolume = touchable->GetVolume(depth+1);

        if (ClassifyTopLevelVolume(parentVolume)!= GeometryObjectType::World){
            continue;
        }

        result.type = ClassifyTopLevelVolume(volume);
        result.copyNo = touchable->GetCopyNumber(depth);

        return result;
    }

    // for文で見つからなかった場合には、初期値を返す
    return result;

}


GeometryObjectType NeutronStepProcessor::ClassifyTopLevelVolume(
    const G4VPhysicalVolume* volume)
{
    if (volume == nullptr) {
        return GeometryObjectType::Unknown;
    }

    const auto& name = volume->GetName();

    if (name == "World_PhysVol") {
        return GeometryObjectType::World;
    }

    if (name == "LiGlass_Phys") {
        return GeometryObjectType::LiGlass;
    }

    if (name == "UROKO_Phys") {
        return GeometryObjectType::UROKO;
    }

    if (name == "HILE_Phys") {
        return GeometryObjectType::HILE;
    }

    /*
     * HPGeは複数の装置名を持つが、
     * GeometryObjectTypeとしてはすべてHPGeにまとめる。
     * 個体の区別にはcopyNoを使用する。
     */
    if (name == "Handai55" ||
        name == "SUNY_LEPS" ||
        name == "Kyudai80" ||
        name == "SUNY_ALICE" ||
        name == "SUNY_CINDY" ||
        name == "Handai60") {
        return GeometryObjectType::HPGe;
    }

    if (name == "BetaPlastic_Phys") {
        return GeometryObjectType::BetaPlastic;
    }

    if (name == "Magnet_Phys") {
        return GeometryObjectType::Magnet;
    }

    if (name == "Frame") {
        return GeometryObjectType::Frame;
    }

    if (name == "Floor") {
        return GeometryObjectType::Floor;
    }

    if (name.rfind("Shield", 0) == 0) {
        return GeometryObjectType::Shield;
    }

    if (name == "Chamber") {
        return GeometryObjectType::Chamber;
    }

    if (name == "Stopper") {
        return GeometryObjectType::Stopper;
    }

    return GeometryObjectType::Unknown;
}
