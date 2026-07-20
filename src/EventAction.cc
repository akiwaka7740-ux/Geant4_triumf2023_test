#include "EventAction.hh"
#include "G4AnalysisManager.hh"
#include "G4SDManager.hh"
#include "ScintiSD.hh"
#include "CathodeSD.hh"

#include "AnalysisOutput.hh"


void EventAction::BeginOfEventAction(const G4Event*) {
    for (int i = 0; i < 2; i++) {
        fHitTimeList[i].clear();
        fHitPosXList[i].clear();
        fHitPosYList[i].clear();
        fHitPosZList[i].clear();
    }

    fScintiPosGlobal.clear();
    fScintiPosLocal.clear();
    fRadius = -99999.0;
    fEff[0] = 0.0;
    fEff[1] = 0.0;
}

void EventAction::EndOfEventAction(const G4Event*) {

    //Event毎にデータを確定させる
    auto analysisManager = G4AnalysisManager::Instance();

    auto sdManager = G4SDManager::GetSDMpointer();

    //efficiencyの計算
    auto scintiSD = static_cast<ScintiSD*>(sdManager->FindSensitiveDetector("ScintiSD"));
    auto cathodeSD = static_cast<CathodeSD*>(sdManager->FindSensitiveDetector("CathodeSD"));

    if (scintiSD && cathodeSD) {

        G4double generatedPhotons = scintiSD->GetGeneratedPhotons();
        G4double pmt1Photons = cathodeSD->GetArrivedPhotons(0);
        G4double pmt2Photons = cathodeSD->GetArrivedPhotons(1);

        if (generatedPhotons > 0) {
            fEff[0] = pmt1Photons / generatedPhotons;
            fEff[1] = pmt2Photons / generatedPhotons;
        } else {
            fEff[0] = 0.0;
            fEff[1] = 0.0;
        }

        G4ThreeVector posG = scintiSD->GetFirstHitPosGlobal();
        fScintiPosGlobal = { posG.x(), posG.y(), posG.z() }; 

        G4ThreeVector posL = scintiSD->GetFirstHitPosLocal();
        fScintiPosLocal  = { posL.x(), posL.y(), posL.z() };

        // 中心位置からの距離を算出（**重要** 検出器はz方向が奥行きになっている）
        fRadius = std::sqrt(posL.x() * posL.x() + posL.y() * posL.y());
    }

    if(fAnalysisOutput) {
        fAnalysisOutput->FillEventSummary(fRadius, fEff[0], fEff[1]);
        fAnalysisOutput->AddRow();
    }

}