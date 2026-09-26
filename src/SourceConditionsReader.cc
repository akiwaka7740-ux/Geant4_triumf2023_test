#include "SourceConditionsReader.hh"

#include "G4Exception.hh"
#include "G4GeneralParticleSourceData.hh"
#include "G4ParticleDefinition.hh"
#include "G4PhysicalConstants.hh"
#include "G4SingleParticleSource.hh"
#include "G4SPSAngDistribution.hh"
#include "G4SPSEneDistribution.hh"
#include "G4SPSPosDistribution.hh"

#include <cmath>
#include <stdexcept>

namespace
{

void Require(
    G4bool condition,
    const G4String& message
)
{
    if (condition) {
        return;
    }

    G4Exception(
        "SourceConditionsReader::Read",
        "SourceConditionsReader001",
        FatalException,
        message.c_str()
    );

    // 例外ハンドラーが終了を抑制しても、
    // 不完全な条件を返さない
    throw std::runtime_error(message.c_str());
}

}

namespace SourceConditionsReader
{

RunConditions Read(G4int runId)
{
    Require(
        runId >= 0,
        "Run ID must be non-negative."
    );

    RunConditions conditions;
    conditions.runId = runId;

    // GPS の共有設定を参照する。
    // 新しい粒子生成器は作成しない。
    auto* gpsData =
        G4GeneralParticleSourceData::Instance();

    conditions.sourceCount =
        gpsData->GetSourceVectorSize();

    Require(
        conditions.sourceCount == 1,
        "Only one GPS source is currently supported."
    );

    // 引数なしの getter を使い、
    // GPS の current source を変更しない。
    const auto* source =
        gpsData->GetCurrentSource();

    Require(
        source != nullptr,
        "GPS source is null."
    );

    const auto* particle =
        source->GetParticleDefinition();

    Require(
        particle != nullptr,
        "GPS particle definition is null."
    );

    conditions.particleName =
        particle->GetParticleName();

    Require(
        conditions.particleName == "neutron" ||
        conditions.particleName == "e-" ||
        conditions.particleName == "gamma",
        "This initial reader supports neutron, e-, and gamma."
    );

    conditions.pdgCode =
        particle->GetPDGEncoding();

    conditions.particleCharge =
        particle->GetPDGCharge();

    conditions.particlesPerEvent =
        source->GetNumberOfParticles();

    Require(
        conditions.particlesPerEvent > 0,
        "The number of particles must be positive."
    );

    // エネルギー分布
    auto* energy = source->GetEneDist();

    Require(
        energy != nullptr,
        "GPS energy distribution is null."
    );

    conditions.energyDistribution =
        energy->GetEnergyDisType();

    Require(
        conditions.energyDistribution == "Mono",
        "Only Mono energy distribution is currently supported."
    );

    conditions.monoEnergy =
        energy->GetMonoEnergy();

    Require(
        std::isfinite(conditions.monoEnergy) &&
        conditions.monoEnergy >= 0.0,
        "Mono energy must be finite and non-negative."
    );

    // 位置分布
    const auto* position = source->GetPosDist();

    Require(
        position != nullptr,
        "GPS position distribution is null."
    );

    conditions.positionDistribution =
        position->GetPosDisType();

    Require(
        conditions.positionDistribution == "Point",
        "Only Point position distribution is currently supported."
    );

    Require(
        !position->GetConfined(),
        "Position confinement is not supported by this reader."
    );

    conditions.positionCentre =
        position->GetCentreCoords();

    // 角度分布
    auto* angular = source->GetAngDist();

    Require(
        angular != nullptr,
        "GPS angular distribution is null."
    );

    conditions.angularDistribution =
        angular->GetDistType();

    Require(
        conditions.angularDistribution == "iso" ||
        conditions.angularDistribution == "planar",
        "Only iso and planar angular distributions are supported."
    );

    if (conditions.angularDistribution == "iso") {
        conditions.minTheta = angular->GetMinTheta();
        conditions.maxTheta = angular->GetMaxTheta();
        conditions.minPhi = angular->GetMinPhi();
        conditions.maxPhi = angular->GetMaxPhi();

        constexpr G4double tolerance = 1.0e-12;

        Require(
            std::abs(conditions.minTheta) < tolerance &&
            std::abs(conditions.maxTheta - CLHEP::pi)
                < tolerance &&
            std::abs(conditions.minPhi) < tolerance &&
            std::abs(conditions.maxPhi - CLHEP::twopi)
                < tolerance,
            "Only full-sphere isotropic emission is currently supported."
        );
    }
    else {
        conditions.fixedDirection =
            angular->GetDirection();

        Require(
            std::isfinite(conditions.fixedDirection.mag2()) &&
            conditions.fixedDirection.mag2() > 0.0,
            "Fixed direction must be finite and non-zero."
        );
    }

    return conditions;
}

}