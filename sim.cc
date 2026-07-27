#include <iostream>
#include <random> 
#include <fstream>
#include <string>
#include <cstdlib>

#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4Threading.hh"
#include "G4HadronicParameters.hh" 

#include "PhysicsList.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

namespace{
std::string GetEnvString(const char*name)
{
    const char* value = std::getenv(name);
    if (value && value[0] != '\0') {
        return std::string(value);
    }
    return "";
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

    std::string logDir = GetEnvString("G4_LOG_DIR");

    //毎回異なる結果を発生させるために乱数でシード値を取得
    std::random_device seed_gen;
    auto seed = seed_gen();
    G4Random::setTheEngine(new CLHEP::MTwistEngine);
    //シード値を出力しておくことで、後で同じ結果を再現することも可能
    G4cout << "Random Seed is " << seed << G4endl;
    G4Random::setTheSeed(seed);

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

    //PhysicsList  
    PhysicsList* physicsList = new PhysicsList();
    runManager->SetUserInitialization(physicsList);

    //放射性崩壊の時間閾値を非常に大きな値に設定する
    G4HadronicParameters::Instance()->SetTimeThresholdForRadioactiveDecay( 1.0e+60*CLHEP::year );
    
    //DetectorConstruction
    runManager->SetUserInitialization(new DetectorConstruction());

    //ActionInitializtion
    runManager->SetUserInitialization(new ActionInitialization());


    /*
    if (argc == 1)
    {
        ui = new G4UIExecutive(argc, argv);
    }
        */



    G4VisManager *visManager = new G4VisExecutive();
    visManager->Initialize();

    G4UImanager *UImanager = G4UImanager::GetUIpointer();
    if (ui)
    {   
        ui->SessionStart();
        delete ui;
    }
    else
    {
        G4String command = "/control/execute "; 
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);
    }



    /*
    if(ui)
    {
        UImanager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
    }
    else
    {
        G4String command = "/control/execute" ;
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);
    }
    */
 

    return 0;
}

