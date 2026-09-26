#include <iostream>
#include <random> 
#include <fstream>
#include <string>
#include <cstdlib>
#include <memory>
#include <filesystem>
#include <system_error>

#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4Threading.hh"
#include "G4HadronicParameters.hh" 
#include "G4UIcommandStatus.hh"

#include "PhysicsList.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "AnalysisConfig.hh"
#include "AnalysisMessenger.hh"
#include "OutputConfig.hh"



namespace{
std::string GetEnvString(const char*name)
{
    const char* value = std::getenv(name);
    if (value && value[0] != '\0') {
        return std::string(value);
    }
    return "";
}

std::shared_ptr<OutputConfig> CreateOutputConfig(   
)
{
    auto config = std::make_shared<OutputConfig>();

    // shell から実行 ID が渡されていれば、それを使う
    config->executionId =
        GetEnvString("G4_EXECUTION_ID");

    // 直接起動した場合はローカル時刻を秒まで使用する
    if (config->executionId.empty()) {
        const std::time_t now = std::time(nullptr);

        if (now == static_cast<std::time_t>(-1)) {
            G4cerr
                << "Cannot obtain the current time."
                << G4endl;

            return nullptr;
        }

        // この処理は worker 起動前の main で一度だけ行う
        const std::tm* localTime = std::localtime(&now);

        if (localTime == nullptr) {
            G4cerr
                << "Cannot convert the current time to local time."
                << G4endl;

            return nullptr;
        }

        std::ostringstream timestamp;
        timestamp.imbue(std::locale::classic());

        timestamp
            << std::put_time(
                localTime,
                "%Y%m%d_%H%M%S"
            );

        config->executionId = timestamp.str();
    }

    // 実行 ID はパスではなく、一つの識別子として扱う
    if (
        config->executionId == "." ||
        config->executionId == ".." ||
        config->executionId.find_first_not_of(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789_-."
        ) != G4String::npos
    ) {
        G4Exception(
            "CreateOutputConfig",
            "OutputConfig001",
            FatalException,
            "G4_EXECUTION_ID contains invalid characters."
        );

        return nullptr;
    }

    config->executionDirectory =
        GetEnvString("G4_OUTPUT_DIR");

    if (config->executionDirectory.empty()) {
        config->executionDirectory =
            "results/" + config->executionId;
    }

    const auto naming =
        GetEnvString("G4_OUTPUT_NAMING");

    if (naming.empty() || naming == "timestamp") {
        config->namingMode =
            OutputNamingMode::Timestamp;
    }
    else if (naming == "source") {
        config->namingMode =
            OutputNamingMode::Source;
    }
    else {
        G4Exception(
            "CreateOutputConfig",
            "OutputConfig002",
            FatalException,
            "G4_OUTPUT_NAMING must be timestamp or source."
        );

        return nullptr;
    }

    return config;
}


bool PrepareOutputDirectory(OutputConfig& config)
{
    namespace fs = std::filesystem;

    fs::path executionDirectory{
        std::string(config.executionDirectory)
    };
    const auto preparedValue =
        GetEnvString("G4_OUTPUT_PREPARED");

    if (
        !preparedValue.empty() &&
        preparedValue != "0" &&
        preparedValue != "1"
    ) {
        G4cerr
            << "G4_OUTPUT_PREPARED must be 0 or 1."
            << G4endl;

        return false;
    }

    const bool preparedByShell =
        preparedValue == "1";

    std::error_code error;

    const auto reportFailure =
        [&](const std::string& message) {
            G4cerr
                << message
                << "\nExecution directory: "
                << executionDirectory.string();

            if (error) {
                G4cerr
                    << "\nReason: "
                    << error.message();
            }

            G4cerr << G4endl;
            return false;
        };

    if (preparedByShell) {
        // shell が準備した保存先を明示的に受け取る
        if (
            GetEnvString("G4_EXECUTION_ID").empty() ||
            GetEnvString("G4_OUTPUT_DIR").empty()
        ) {
            return reportFailure(
                "Prepared output requires G4_EXECUTION_ID "
                "and G4_OUTPUT_DIR."
            );
        }

        const bool exists =
            fs::is_directory(executionDirectory, error);

        if (error || !exists) {
            return reportFailure(
                "Prepared execution directory is missing."
            );
        }
    }

    else {
        // ID と保存先の両方が自動の場合だけ、
        // 重複時に連番を付ける。
        // 明示された ID や保存先は勝手に変更しない。
        const bool automaticNaming =
            GetEnvString("G4_EXECUTION_ID").empty() &&
            GetEnvString("G4_OUTPUT_DIR").empty();

        const auto parentDirectory =
            executionDirectory.parent_path();

        if (!parentDirectory.empty()) {
            fs::create_directories(parentDirectory, error);

            if (error) {
                return reportFailure(
                    "Cannot create output parent directory."
                );
            }
        }

        const G4String baseId = config.executionId;
        const fs::path originalDirectory = executionDirectory;

        const int maximumSequence =
            automaticNaming ? 999 : 0;

        bool reserved = false;

        for (
            int sequence = 0;
            sequence <= maximumSequence;
            ++sequence
        ) {
            std::ostringstream candidateId;
            candidateId.imbue(std::locale::classic());
            candidateId << baseId;

            if (sequence > 0) {
                candidateId
                    << '_'
                    << std::setfill('0')
                    << std::setw(3)
                    << sequence;
            }

            const fs::path candidateDirectory =
                automaticNaming
                    ? parentDirectory / candidateId.str()
                    : originalDirectory;

            // 実際の作成に成功した場合だけ保存先を確定する
            const bool created =
                fs::create_directory(candidateDirectory, error);

            if (created) {
                config.executionId = candidateId.str();
                config.executionDirectory =
                    candidateDirectory.string();

                executionDirectory = candidateDirectory;
                reserved = true;
                break;
            }

            // 名前の重複以外の失敗は終了する
            if (
                error &&
                error != std::errc::file_exists
            ) {
                return reportFailure(
                    "Cannot create execution directory: "
                    + candidateDirectory.string()
                );
            }

            if (!automaticNaming) {
                return reportFailure(
                    "The specified execution directory already exists."
                );
            }
        }

        if (!reserved) {
            return reportFailure(
                "No available execution ID for this timestamp."
            );
        }
    }

    // shell 経由では存在確認、直接起動では新規作成
    for (const char* name :
         {"root", "conditions", "inputs", "logs"}) {
        const auto directory =
            executionDirectory / name;

        if (preparedByShell) {
            const bool exists =
                fs::is_directory(directory, error);

            if (error || !exists) {
                return reportFailure(
                    "Prepared subdirectory is missing: "
                    + directory.string()
                );
            }
        }
        else {
            const bool created =
                fs::create_directory(directory, error);

            if (error || !created) {
                return reportFailure(
                    "Cannot create subdirectory: "
                    + directory.string()
                );
            }
        }
    }

    if (preparedByShell) {
        // 入力とログは準備済みでもよいが、
        // シミュレーション結果はまだ存在してはいけない
        for (const char* name : {"root", "conditions"}) {
            const auto directory =
                executionDirectory / name;

            const bool empty =
                fs::is_empty(directory, error);

            if (error || !empty) {
                return reportFailure(
                    "Output subdirectory must be empty: "
                    + directory.string()
                );
            }
        }
    }

    // 同じ保存先で二度起動することを防ぐ。
    // ディレクトリ作成の成功を、使用開始の印にする。
    const auto startedMarker =
        executionDirectory / ".simulation_started";

    const bool claimed =
        fs::create_directory(startedMarker, error);

    if (error) {
        return reportFailure(
            "Cannot mark the execution directory as started."
        );
    }

    if (!claimed) {
        return reportFailure(
            "This execution directory has already been used. "
            "Create a new execution directory."
        );
    }

    return true;
}

void WriteRandomLog(const std::string& logDir, unsigned long seed)
{
    if (logDir.empty()) return;

    std::ofstream ofs(logDir + "/random.txt");
    ofs << "RANDOM_ENGINE=CLHEP::MTwistEngine\n";
    ofs << "MASTER_SEED=" << seed << "\n";
}

void WriteMTLog(
    const std::string& logDir,
    const std::string& mode,
    G4int nThreads,
    G4int nCores,
    const std::string& policy
)
{
    if (logDir.empty()) return;

    std::ofstream ofs(logDir + "/mt.txt");
    ofs << "MODE=" << mode << "\n";
    ofs << "NUMBER_OF_THREADS=" << nThreads << "\n";
    ofs << "NUMBER_OF_CORES=" << nCores << "\n";
    ofs << "THREAD_POLICY=" << policy << "\n";
}


}

int main(int argc, char**argv){

    std::string logDir;

    //毎回異なる結果を発生させるために乱数でシード値を取得
    std::random_device seed_gen;
    auto seed = seed_gen();
    G4Random::setTheEngine(new CLHEP::MTwistEngine);
    //シード値を出力しておくことで、後で同じ結果を再現することも可能
    G4cout << "Random Seed is " << seed << G4endl;
    G4Random::setTheSeed(seed);

    auto pendingOutputConfig =
        CreateOutputConfig();

    if (pendingOutputConfig == nullptr) {
        return EXIT_FAILURE;
    }

    if (!PrepareOutputDirectory(*pendingOutputConfig)) {
        return EXIT_FAILURE;
    }

    // ID と保存先が確定した後は、読み取り専用で共有する
    const std::shared_ptr<const OutputConfig> outputConfig =
        std::move(pendingOutputConfig);

    logDir = (
        std::filesystem::path(
            std::string(outputConfig->executionDirectory)
        ) / "logs"
    ).string();

    G4cout
        << "Execution ID: "
        << outputConfig->executionId
        << "\nOutput directory: "
        << outputConfig->executionDirectory
        << G4endl;    

    WriteRandomLog(logDir, seed);

    G4UIExecutive *ui = nullptr;
    if(argc == 1){
        ui = new G4UIExecutive(argc, argv);
    }


    #ifdef G4MULTITHREADED
        G4MTRunManager *runManager = new G4MTRunManager;

        // ★修正ポイント2：PCの最大コア（スレッド）数を取得してセットする
        //G4int nThreads = G4Threading::G4GetNumberOfCores();
        
        // ※もしPCで他の作業も並行して行いたい場合は、以下のように1〜2コア余らせるのが安全です。
        G4int nCores = G4Threading::G4GetNumberOfCores();
        G4int nThreads = nCores - 4; 
        if (nThreads < 1){
            nThreads = 1;
        
        }
        
        runManager->SetNumberOfThreads(nThreads);
        WriteMTLog(logDir, "multithread", nThreads, nCores, "cores_minus_4_min_1");
        G4cout << "=== Running in Multithread mode with " << nThreads << " threads ===" << G4endl;

    #else
        G4RunManager *runManager = new G4RunManager;
        WriteMTLog(logDir, "singlethread", 1, 1, "fixed_1");   
        G4cout << "=== Running in Single thread mode ===" << G4endl;
    #endif 


    auto analysisConfig = std::make_shared<AnalysisConfig>();
    auto analysisMessenger = std::make_unique<AnalysisMessenger>(analysisConfig);


    //PhysicsList  
    PhysicsList* physicsList = new PhysicsList();
    runManager->SetUserInitialization(physicsList);

    //放射性崩壊の時間閾値を非常に大きな値に設定する
    G4HadronicParameters::Instance()->SetTimeThresholdForRadioactiveDecay( 1.0e+60*CLHEP::year );
    
    //DetectorConstruction
    runManager->SetUserInitialization(new DetectorConstruction(analysisConfig));

    //ActionInitializtion
    runManager->SetUserInitialization(new ActionInitialization(analysisConfig,outputConfig));

    /*
    if (argc == 1)
    {
        ui = new G4UIExecutive(argc, argv);
    }
        */



    G4VisManager *visManager = new G4VisExecutive();
    visManager->Initialize();

    G4UImanager* UImanager =
        G4UImanager::GetUIpointer();

    int exitStatus = EXIT_SUCCESS;

    if (ui != nullptr) {
        // 対話モード
        ui->SessionStart();

        delete ui;
        ui = nullptr;
    }
    else {
        // バッチモード
        const G4String fileName = argv[1];

        const G4String command =
            G4String("/control/execute ") + fileName;

        const G4int commandStatus =
            UImanager->ApplyCommand(command);

        if (commandStatus != fCommandSucceeded) {
            G4cerr
                << "Macro execution failed."
                << "\nMacro: " << fileName
                << "\nGeant4 command status: "
                << commandStatus
                << G4endl;

            exitStatus = EXIT_FAILURE;
        }
    }

    // 可視化マネージャーを先に破棄する
    delete visManager;
    delete runManager;

    return exitStatus;
    
}

