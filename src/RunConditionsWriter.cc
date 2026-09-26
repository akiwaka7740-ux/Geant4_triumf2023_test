#include "RunConditionsWriter.hh"

#include "OutputConfig.hh"

#include "G4Exception.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{

[[noreturn]] void Fail(const std::string& message)
{
    G4Exception(
        "RunConditionsWriter::Write",
        "RunConditionsWriter001",
        FatalException,
        message.c_str()
    );

    throw std::runtime_error(message);
}

// JSON の文字列として引用符と制御文字を処理する
std::string Quote(const std::string& value)
{
    constexpr char hex[] = "0123456789abcdef";

    std::string result = "\"";

    for (const unsigned char character : value) {
        switch (character) {
        case '"':
            result += "\\\"";
            break;

        case '\\':
            result += "\\\\";
            break;

        default:
            if (character < 0x20) {
                result += "\\u00";
                result += hex[character >> 4];
                result += hex[character & 0x0f];
            }
            else {
                result += static_cast<char>(character);
            }
            break;
        }
    }

    result += '"';
    return result;
}

// JSON では NaN と無限大を数値として保存できない
G4double Number(G4double value)
{
    if (!std::isfinite(value)) {
        Fail("A condition contains a non-finite number.");
    }

    return value;
}

void WriteVector(
    std::ostream& stream,
    const G4ThreeVector& value,
    G4double unit
)
{
    stream
        << '['
        << Number(value.x() / unit) << ", "
        << Number(value.y() / unit) << ", "
        << Number(value.z() / unit)
        << ']';
}

const char* StatusName(RunConditionsWriter::Status status)
{
    using Status = RunConditionsWriter::Status;

    switch (status) {
    case Status::Running:
        return "running";
    case Status::Completed:
        return "completed";
    case Status::Aborted:
        return "aborted";
    case Status::Failed:
        return "failed";
    }

    Fail("Unknown run status.");
}

const char* NamingModeName(OutputNamingMode mode)
{
    switch (mode) {
    case OutputNamingMode::Timestamp:
        return "timestamp";
    case OutputNamingMode::Source:
        return "source";
    }

    Fail("Unknown output naming mode.");
}

}

namespace RunConditionsWriter
{

void Write(
    const OutputConfig& outputConfig,
    const RunConditions& conditions,
    const G4String& rootFilePath,
    G4int requestedEvents,
    G4int processedEvents,
    Status status
)
{
    namespace fs = std::filesystem;

    if (
        conditions.runId < 0 ||
        requestedEvents < 0 ||
        processedEvents < 0
    ) {
        Fail("Run ID and event counts must be non-negative.");
    }

    const fs::path executionDirectory{
        std::string(outputConfig.executionDirectory)
    };

    // 実行ディレクトリを移動しても対応が保たれるよう、
    // ROOT の場所は実行ディレクトリからの相対パスで保存する
    const auto relativeRootPath =
        fs::path(std::string(rootFilePath))
            .lexically_relative(executionDirectory);

    if (
        relativeRootPath.empty() ||
        relativeRootPath.is_absolute() ||
        *relativeRootPath.begin() == fs::path("..")
    ) {
        Fail("ROOT output must be inside the execution directory.");
    }

    std::ostringstream fileName;
    fileName.imbue(std::locale::classic());

    fileName
        << "run"
        << std::setfill('0')
        << std::setw(3)
        << conditions.runId
        << ".json";

    const auto finalPath =
        executionDirectory / "conditions" / fileName.str();

    auto temporaryPath = finalPath;
    temporaryPath += ".tmp";

    // ファイルに書く前に、JSON 全体を構築する
    std::ostringstream json;
    json.imbue(std::locale::classic());

    json
        << std::setprecision(
            std::numeric_limits<G4double>::max_digits10
        )
        << std::boolalpha;

    json
        << "{\n"
        << "  \"schema_version\": 1,\n"
        << "  \"execution_id\": "
        << Quote(outputConfig.executionId) << ",\n"
        << "  \"run_id\": " << conditions.runId << ",\n"
        << "  \"status\": "
        << Quote(StatusName(status)) << ",\n"
        << "  \"requested_events\": "
        << requestedEvents << ",\n"
        << "  \"processed_events\": "
        << processedEvents << ",\n"
        << "  \"root_naming_mode\": "
        << Quote(NamingModeName(outputConfig.namingMode))
        << ",\n"
        << "  \"root_file\": "
        << Quote(relativeRootPath.generic_string()) << ",\n";

    json
        << "  \"source_count\": "
        << conditions.sourceCount << ",\n"
        << "  \"particles_per_event\": "
        << conditions.particlesPerEvent << ",\n"
        << "  \"particle_name\": "
        << Quote(conditions.particleName) << ",\n"
        << "  \"pdg_code\": "
        << conditions.pdgCode << ",\n"
        << "  \"particle_charge_e\": "
        << Number(conditions.particleCharge / eplus) << ",\n"
        << "  \"is_ion\": "
        << conditions.isIon << ",\n";

    json << "  \"ion\": ";

    if (conditions.isIon) {
        json
            << "{"
            << "\"atomic_number\": "
            << conditions.atomicNumber << ", "
            << "\"mass_number\": "
            << conditions.massNumber << ", "
            << "\"excitation_energy_MeV\": "
            << Number(conditions.excitationEnergy / MeV)
            << "}";
    }
    else {
        json << "null";
    }

    json << ",\n";

    json
        << "  \"energy_distribution\": "
        << Quote(conditions.energyDistribution) << ",\n"
        << "  \"mono_energy_MeV\": "
        << Number(conditions.monoEnergy / MeV) << ",\n"
        << "  \"position_distribution\": "
        << Quote(conditions.positionDistribution) << ",\n"
        << "  \"position_centre_mm\": ";

    WriteVector(json, conditions.positionCentre, mm);
    json << ",\n";

    json
        << "  \"angular_distribution\": "
        << Quote(conditions.angularDistribution) << ",\n"
        << "  \"fixed_direction\": ";

    if (conditions.angularDistribution == "planar") {
        WriteVector(json, conditions.fixedDirection, 1.0);
    }
    else {
        json << "null";
    }

    json << ",\n  \"angular_limits_rad\": ";

    if (conditions.angularDistribution == "iso") {
        json
            << "{"
            << "\"min_theta\": "
            << Number(conditions.minTheta / rad) << ", "
            << "\"max_theta\": "
            << Number(conditions.maxTheta / rad) << ", "
            << "\"min_phi\": "
            << Number(conditions.minPhi / rad) << ", "
            << "\"max_phi\": "
            << Number(conditions.maxPhi / rad)
            << "}";
    }
    else {
        json << "null";
    }

    json << "\n}\n";

    // 一時ファイルへの書き込み
    std::ofstream output(
        temporaryPath,
        std::ios::out | std::ios::binary | std::ios::trunc
    );

    if (!output) {
        Fail(
            "Cannot open temporary conditions file: "
            + temporaryPath.string()
        );
    }

    output << json.str();
    output.close();

    if (!output) {
        Fail(
            "Cannot write conditions file: "
            + temporaryPath.string()
        );
    }

    // 同じディレクトリ内で正式なファイルへ置き換える
    std::error_code error;
    fs::rename(temporaryPath, finalPath, error);

    if (error) {
        Fail(
            "Cannot replace conditions file: "
            + finalPath.string()
            + ". Reason: "
            + error.message()
        );
    }
}

}