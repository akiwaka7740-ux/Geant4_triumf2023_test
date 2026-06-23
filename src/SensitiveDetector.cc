#include "SensitiveDetector.hh"

SensitiveDetector::SensitiveDetector(G4String name) : G4VSensitiveDetector(name)
{

    fDetectorName = "unknown";
    fEnergyDeposit = 0.0;
    fIncidentEnergy = -1.0;
    fHitTime = -1.0;
    fHitTimeCapture = -1.0;
    fPosX = fPosY = fPosZ = -99999.0;
    fHitCounter = 0;
}

SensitiveDetector::~SensitiveDetector()
{
}

void SensitiveDetector::Initialize(G4HCofThisEvent *)
{
    fDetectorName = "unknown";
    fEnergyDeposit = 0.0;
    fIncidentEnergy = -1.0;
    fHitTime = -1.0;
    fHitTimeCapture = -1.0;
    fPosX = fPosY = fPosZ = -99999.0;
    fHitCounter = 0;
}


void SensitiveDetector::EndOfEvent(G4HCofThisEvent *)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();


    //捕獲反応が起こった場合のみ、書き込みを完了する

    // 列 (Column 0) に値をセット
    analysisManager->FillNtupleSColumn(0, 0, fDetectorName);
    analysisManager->FillNtupleDColumn(0, 1, fEnergyDeposit);
    analysisManager->FillNtupleDColumn(0, 2, fIncidentEnergy);
    analysisManager->FillNtupleDColumn(0, 3, fHitTime);
    //Li glassのみ捕獲反応を起こしうる。HILE、UROKOでは無関係なデータ
    analysisManager->FillNtupleDColumn(0, 4, fHitTimeCapture);
    analysisManager->FillNtupleDColumn(0, 5, fPosX);
    analysisManager->FillNtupleDColumn(0, 6, fPosY);
    analysisManager->FillNtupleDColumn(0, 7, fPosZ);
    analysisManager->FillNtupleIColumn(0, 8, fHitCounter);
    // 【重要】ここで「行を確定」してNtupleに書き込む！
     analysisManager->AddNtupleRow(0);


}


G4bool SensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory *ROhist)
{
    //全ての粒子のエネルギー損失を記録
    fEnergyDeposit += aStep->GetTotalEnergyDeposit() / keV;
    //G4cout << "EnergyDeposit: " << fEnergyDeposit << " keV" << G4endl;

    /*
    //debug用
    //エネルギー損失が 464 keV を超えた場合の情報を出力する)← UROKO、HILE では、464 keV を超えることはないはずなので、もし出力される場合は何かおかしい
    //どうやら、熱中性子の捕獲でガンマ線（2.2 MeV)が出て、comptで散乱して、UROKOやHILEで 464 keV を超えるエネルギー損失が起きることがあるらしい。
    if (fEnergyDeposit > 464.0) {
        //G4cout << "EnergyDeposit: " << fEnergyDeposit << " keV" << G4endl;
        if (aStep->GetTotalEnergyDeposit() / keV > 464.0) {
            G4cout << "EnergyDeposit in this step: " << aStep->GetTotalEnergyDeposit() / keV << " keV" << G4endl;
            G4cout << "Particle: " << aStep->GetTrack()->GetDefinition()->GetParticleName() << G4endl;

            if(aStep->GetTrack()->GetDefinition()->GetParticleName() == "e-") {
                G4String creatorProcessName = aStep->GetTrack()->GetCreatorProcess()->GetProcessName();
                G4cout << "Electron created by process: " << creatorProcessName << G4endl;

                if (creatorProcessName == "compt"){
                    G4int parentID = aStep->GetTrack()->GetParentID();
                    G4cout << "Parent ID: " << parentID << G4endl;
                }
            }
         
        }
   
    }
    */

    //粒子名の取得
    G4String particleName = aStep->GetTrack()->GetDefinition()->GetParticleName();

    //debug用：どの粒子が、今何 keV 落としたかを出力
    // どの粒子が、今何 keV 落としたかを出力
    /*
    G4cout << "Particle: " << particleName 
       << " | Edep: " << aStep->GetTotalEnergyDeposit() / keV << " keV" << G4endl;
    */

    /*
    if ( particleName == "gamma" && aStep->GetTotalEnergyDeposit() > 0.0 ) {
        G4cout << "Gamma detected! Energy Deposit: " << aStep->GetTotalEnergyDeposit() / keV << " keV" << G4endl;
    }

    if ( particleName == "deuteron" && aStep->GetTotalEnergyDeposit() > 0.0 ) {
        G4cout << "Deuteron detected! Energy Deposit: " << aStep->GetTotalEnergyDeposit() / keV << " keV" << G4endl;
    }
    */
    

    // 中性子以外の粒子は無視する
    if (particleName != "neutron") {
        return false; // 中性子以外（ガンマ線や二次粒子のアルファ線など）は無視して終了
    }

    //境界を跨いだかどうかの判定
    if (aStep->GetPreStepPoint()->GetStepStatus() == fGeomBoundary) {
        
        // まだエネルギーを記録していなければ記録（反射等で複数回入るのを防ぐ）
        if (fIncidentEnergy < 0.0) {
            // 運動エネルギー (KineticEnergy) を取得し、keV単位で保存
            fIncidentEnergy = aStep->GetPreStepPoint()->GetKineticEnergy() / keV;

            //debug用
            
            G4StepPoint *prePot = aStep->GetPreStepPoint();
            G4ThreeVector worldPos = prePot->GetPosition();
            G4cout << worldPos << G4endl;
            
            
            // 確認用出力
            // G4cout << "Neutron Entered! Energy: " << fIncidentEnergy << " MeV" << G4endl;
        }

        return false; // 中性子が境界を跨いだだけなので、ヒットとしてはカウントしない(以降の処理は行わない)
    }

    //プロセスを取得する前の安全対策
    // ジオメトリの境界を跨いだだけの時などは Process が Null になることがあるため、
    // 必ずポインタが存在するかチェック
    const G4VProcess* process = aStep->GetPostStepPoint()->GetProcessDefinedStep();
    if (!process) {
        return false; 
    }

    //以降、ヒットがあった場合の処理
    fHitCounter++;

    //特に初回のヒットのみ、時刻、位置、検出器名を記録
    if (fHitTime < 0){
        fHitTime = aStep->GetPostStepPoint()->GetGlobalTime();
        fDetectorName = aStep->GetPreStepPoint()->GetPhysicalVolume()->GetName();
        fPosX = aStep->GetPreStepPoint()->GetPosition().x() / mm;
        fPosY = aStep->GetPreStepPoint()->GetPosition().y() / mm;
        fPosZ = aStep->GetPreStepPoint()->GetPosition().z()/ mm;
    }


    //プロセス名の取得と判定
    G4String processName = process->GetProcessName();
    //G4cout << "ProcessName is : " << processName << G4endl;

    // 中性子の弾性散乱や非弾性散乱が起きたかどうかを判定
    if (processName == "hadElastic" || processName == "neutronInelastic"){
        ++fHitCounter;
    }

    // 中性子捕獲 ("nCapture") が起きたかどうかを判定
    if (processName == "nCapture") {
        
        ++fHitCounter;
        fHitTimeCapture = aStep->GetPostStepPoint()->GetGlobalTime();

        //G4cout << "ProcessName is : " << processName << G4endl;

        //確認用
        //G4cout << "HitTime : " << fHitTime << G4endl;
    
        // 一度捕獲反応が起きれば中性子は消滅するため、以降の追跡を強制終了させても良いです
        aStep->GetTrack()->SetTrackStatus(fStopAndKill);
    }

    return false;

}