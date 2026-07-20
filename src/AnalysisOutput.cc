#include "AnalysisOutput.hh"
#include "EventAction.hh"
#include "G4AnalysisManager.hh"

void AnalysisOutput::Book(EventAction* eventAction)
{
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetNtupleMerging(true);
    analysisManager->CreateNtuple("tree", "tree");

    fScintiEdep =
        analysisManager->CreateNtupleDColumn("Scinti_Edep");
    fScintiEvis =
        analysisManager->CreateNtupleDColumn("Scinti_Evis");
    fScintiPhotons =
        analysisManager->CreateNtupleDColumn("Scinti_Photons");
    fScintiInteractionCount =
        analysisManager->CreateNtupleIColumn("Scinti_InteractionCount");

    fScintiHitPosGlobal =
        analysisManager->CreateNtupleDColumn(
            "Scinti_HitPos_Global",
            eventAction->GetScintiPosGlobalRef()
        );
    fScintiHitPosLocal =
        analysisManager->CreateNtupleDColumn(
            "Scinti_HitPos_Local",
            eventAction->GetScintiPosLocalRef()
        );
    fScintiHitPosRadius =
        analysisManager->CreateNtupleDColumn("Scinti_HitPos_Radius");

    fPMTSumPhotons =
        analysisManager->CreateNtupleIColumn("PMT_SUM_Photons");
    fPMTSumEfficiency =
        analysisManager->CreateNtupleDColumn("PMT_SUM_Efficiency");
    fPMT1Photons =
        analysisManager->CreateNtupleIColumn("PMT1_Photons");
    fPMT1Efficiency =
        analysisManager->CreateNtupleDColumn("PMT1_Efficiency");
    fPMT2Photons =
        analysisManager->CreateNtupleIColumn("PMT2_Photons");
    fPMT2Efficiency =
        analysisManager->CreateNtupleDColumn("PMT2_Efficiency");

    fPMT1HitTimes =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitTimes",
            eventAction->GetHitTimeListRef(0)
        );
    fPMT1HitPosX =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitPos_X",
            eventAction->GetHitPosXListRef(0)
        );
    fPMT1HitPosY =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitPos_Y",
            eventAction->GetHitPosYListRef(0)
        );
    fPMT1HitPosZ =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitPos_Z",
            eventAction->GetHitPosZListRef(0)
        );

    fPMT2HitTimes =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitTimes",
            eventAction->GetHitTimeListRef(1)
        );
    fPMT2HitPosX =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitPos_X",
            eventAction->GetHitPosXListRef(1)
        );
    fPMT2HitPosY =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitPos_Y",
            eventAction->GetHitPosYListRef(1)
        );
    fPMT2HitPosZ =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitPos_Z",
            eventAction->GetHitPosZListRef(1)
        );

    analysisManager->FinishNtuple(0);
}

void AnalysisOutput::FillScinti(G4double edep, G4double evis, G4double generatedPhotons, G4int neutronInteractionCount)
{
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillNtupleDColumn(fScintiEdep, edep);
    analysisManager->FillNtupleDColumn(fScintiEvis, evis);
    analysisManager->FillNtupleDColumn(fScintiPhotons, generatedPhotons);
    analysisManager->FillNtupleIColumn(fScintiInteractionCount, neutronInteractionCount);
}

void AnalysisOutput::FillPMTPhotons(G4int pmt1Photons, G4int pmt2Photons)
{
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillNtupleIColumn(fPMTSumPhotons, pmt1Photons + pmt2Photons);

    analysisManager->FillNtupleIColumn(fPMT1Photons, pmt1Photons);
    analysisManager->FillNtupleIColumn(fPMT2Photons, pmt2Photons);
}

void AnalysisOutput::FillEventSummary(G4double scintiHitRadius, G4double pmt1Efficiency, G4double pmt2Efficiency)
{
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillNtupleDColumn(fScintiHitPosRadius, scintiHitRadius);

    analysisManager->FillNtupleDColumn(fPMTSumEfficiency, (pmt1Efficiency + pmt2Efficiency));
    analysisManager->FillNtupleDColumn(fPMT1Efficiency, pmt1Efficiency);
    analysisManager->FillNtupleDColumn(fPMT2Efficiency, pmt2Efficiency);
}

void AnalysisOutput::AddRow()
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->AddNtupleRow(0);
}