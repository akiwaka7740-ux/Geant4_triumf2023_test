#include <iostream>
#include <random> 

#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4Threading.hh"

#include "PhysicsList.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "FTFP_BERT_HP.hh"


int main(int argc, char**argv){

    //毎回異なる結果を発生させるために乱数でシード値を取得
    std::random_device seed_gen;
    auto seed = seed_gen();
    G4Random::setTheEngine(new CLHEP::MTwistEngine);
    //シード値を出力しておくことで、後で同じ結果を再現することも可能
    G4cout << "Random Seed is " << seed << G4endl;
    G4Random::setTheSeed(seed);


    G4UIExecutive *ui = new G4UIExecutive(argc, argv);

    #ifdef G4MULTITHREADED
        G4MTRunManager *runManager = new G4MTRunManager;

        // ★修正ポイント2：PCの最大コア（スレッド）数を取得してセットする
        //G4int nThreads = G4Threading::G4GetNumberOfCores();
        
        // ※もしPCで他の作業も並行して行いたい場合は、以下のように1〜2コア余らせるのが安全です。
        G4int nThreads = G4Threading::G4GetNumberOfCores() - 4; 
        
        runManager->SetNumberOfThreads(nThreads);
        G4cout << "=== Running in Multithread mode with " << nThreads << " threads ===" << G4endl;


    #else
        G4RunManager *runManager = new G4RunManager;
        G4cout << "=== Running in Single thread mode ===" << G4endl;
    #endif

    //PhysicsList  (PhysicsList.ccは使用しない)
    G4VModularPhysicsList *physicsList = new FTFP_BERT_HP();
    runManager->SetUserInitialization(physicsList);
    
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
 

    ui->SessionStart();

    return 0;
}