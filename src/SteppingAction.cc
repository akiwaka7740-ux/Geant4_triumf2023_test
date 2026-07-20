#include "SteppingAction.hh"
#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"

SteppingAction::SteppingAction()
 : G4UserSteppingAction()
{
}

void SteppingAction::UserSteppingAction(const G4Step* step) {
    G4Track* track = step->GetTrack();

    /*
    // 1. 今飛んでいるのが「光学光子」であるかチェック
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {

        // 境界反射などで移動距離が 1ピコメートル(1e-12 mm)以下の「足踏みステップ」を検出
        if (step->GetStepLength() <= 1e-12 * mm) {
            
            // マルチスレッド(G4WT)環境でも安全に記録できる thread_local 変数を使用
            static thread_local G4int lastTrackID = -1;
            static thread_local G4int zeroStepCount = 0;
            
            // 同じ光子(Track)が連続で足踏みしているかカウント
            if (track->GetTrackID() == lastTrackID) {
                zeroStepCount++;
            } else {
                lastTrackID = track->GetTrackID();
                zeroStepCount = 1;
            }

            // Geant4カーネルが処刑(25回でAbort)する遥か手前、
            // 「4回連続」で足踏みしたら角っこトラップとみなして即座に消滅させる！
            if (zeroStepCount >= 24) {
                track->SetTrackStatus(fStopAndKill);
                return;
            }
        }

        
        // 2. 【グローバル・キルスイッチ】
        // 今いる部屋が Mother(真空箱) だろうと Guide だろうと関係なく、
        // 500ステップを超えたら無限ピンポン反射トラップとみなして強制キル(生き残る確率は1.36x10-19)
        if (track->GetCurrentStepNumber() > 500) {
            track->SetTrackStatus(fStopAndKill);
            
            // ※必要に応じてデバッグ確認用にコメントアウトを外してください
            // G4cout << "[SteppingAction] Stuck photon killed at step " 
            //        << track->GetCurrentStepNumber() << " in volume : " 
            //        << track->GetVolume()->GetName() << G4endl;
            return;
        }
        
    }
    */

}