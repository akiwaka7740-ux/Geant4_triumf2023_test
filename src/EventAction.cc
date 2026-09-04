#include "EventAction.hh"
#include "ScintiSD.hh"
#include "CathodeSD.hh"
#include "AnalysisOutput.hh"

#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"

#include <map>

namespace {
    struct PmtTotals {
        G4int sumArrivedPhotons = 0;
        G4int sumDetectedPhotons = 0;
    };
}

void EventAction::BeginOfEventAction(const G4Event*) {
   //初期化はSD自身が担当
}

void EventAction::EndOfEventAction(const G4Event* event) {

    if (event == nullptr || fAnalysisOutput == nullptr) {
        return;
    }

    auto* sdManager = G4SDManager::GetSDMpointer();

    auto scintiSD = dynamic_cast<ScintiSD*>(sdManager->FindSensitiveDetector("ScintiSD"));
    auto cathodeSD = dynamic_cast<CathodeSD*>(sdManager->FindSensitiveDetector("CathodeSD"));

    if (scintiSD == nullptr || cathodeSD == nullptr) {
        return;
    }

    const G4int eventId = event->GetEventID();
    G4int runId = -1;

    const auto* currentRun = G4RunManager::GetRunManager()->GetCurrentRun();

    if (currentRun != nullptr) {
        runId = currentRun->GetRunID();
    }

    const auto& pmtDataMap = cathodeSD->GetPmtData();

    // PMTごとの到達光子数と検出光子数の合計を計算
    std::map<DetectorKey, PmtTotals> totalsByDetector;

    for (const auto& [channelKey, pmtData] : pmtDataMap) {
        auto& totals = totalsByDetector[channelKey.detector];
        totals.sumArrivedPhotons += pmtData.arrivedPhotons;
        totals.sumDetectedPhotons += pmtData.detectedPhotons;
    }

    // Scintiデータが存在しない場合に使う初期値？？？　
    const ScintiEventData emptyScintiData;


    //1 PMTchannelにつき１行出力する
    for (const auto& [channelKey, pmtData] : pmtDataMap) {
        
        const ScintiEventData* foundScintiData = scintiSD->FindScintiData(channelKey.detector);

        const ScintiEventData& scintiData = foundScintiData ? *foundScintiData : emptyScintiData;

        const PmtTotals& totals = totalsByDetector.at(channelKey.detector);

        G4double arrivalEfficiency = 0.0;
        G4double detectionEfficiency = 0.0;
        G4double sumArrivalEfficiency = 0.0;
        G4double sumDetectionEfficiency = 0.0;

        if (scintiData.generatedPhotons > 0) {
            arrivalEfficiency = static_cast<G4double>(pmtData.arrivedPhotons) / scintiData.generatedPhotons;
            detectionEfficiency = static_cast<G4double>(pmtData.detectedPhotons) / scintiData.generatedPhotons;

            sumArrivalEfficiency = static_cast<G4double>(totals.sumArrivedPhotons) / scintiData.generatedPhotons;
            sumDetectionEfficiency = static_cast<G4double>(totals.sumDetectedPhotons) / scintiData.generatedPhotons;

        }

        fAnalysisOutput->FillChannelRow(
            runId,
            eventId,
            channelKey,
            scintiData,
            pmtData,
            totals.sumArrivedPhotons,
            totals.sumDetectedPhotons,
            arrivalEfficiency,
            detectionEfficiency,
            sumArrivalEfficiency,
            sumDetectionEfficiency
        );


    }
}