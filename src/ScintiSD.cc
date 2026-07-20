#include "ScintiSD.hh"

#include "G4OpticalPhoton.hh"
#include "G4EventManager.hh"
#include "G4EmSaturation.hh"
#include "G4LossTableManager.hh"
#include "G4Neutron.hh"
#include "G4VProcess.hh"
#include "G4ProcessType.hh"

#include "EventAction.hh"
#include "AnalysisOutput.hh"



ScintiSD::ScintiSD(G4String name)
 : G4VSensitiveDetector(name),
   fTotalEdep(0.0),
   fTotalEvis(0.0),
   fGeneratedPhotons(0.0),
   fFirstHitPosGlobal(-99999.0, -99999.0, -99999.0), 
   fFirstHitPosLocal(-99999.0, -99999.0, -99999.0),
   fNeutronInteractionCount(0)
{
}



void ScintiSD::Initialize(G4HCofThisEvent*) {
    fTotalEdep = 0.0;
    fTotalEvis = 0.0;
    fGeneratedPhotons = 0.0;
    fFirstHitPosGlobal = G4ThreeVector(-99999.0, -99999.0, -99999.0);
    fFirstHitPosLocal = G4ThreeVector(-99999.0, -99999.0, -99999.0);
    fNeutronInteractionCount = 0;
}

G4bool ScintiSD::ProcessHits(G4Step* step, G4TouchableHistory*){

    auto* track = step->GetTrack();
    auto* prePoint = step->GetPreStepPoint();
    auto* postPoint = step->GetPostStepPoint();

    const G4VProcess* process =
        postPoint->GetProcessDefinedStep();

    const G4bool isNeutron =
        track->GetDefinition()
        == G4Neutron::NeutronDefinition();

    const G4bool isPrimaryNeutron =
    isNeutron && track->GetParentID() == 0;

    const G4bool isHadronicInteraction =
        process != nullptr
        && process->GetProcessType() == fHadronic;

    
    if (isPrimaryNeutron && isHadronicInteraction) {

        // 最初の中性子hadronic反応位置
        if (fNeutronInteractionCount == 0) {
            // preとpostの中点を反応点とする
            fFirstHitPosGlobal = (prePoint->GetPosition() + postPoint->GetPosition())/2.0;
                
            auto touchable =
                prePoint->GetTouchable();

            const G4AffineTransform& transform =
                touchable->GetHistory()->GetTopTransform();

            fFirstHitPosLocal =
                transform.TransformPoint(
                    fFirstHitPosGlobal
                );
        }

        ++fNeutronInteractionCount;
    }


    // 以下はエネルギー付与の処理
    const G4double edep =
        step->GetTotalEnergyDeposit();

    if (edep == 0.) {
        return false;
    }

    auto* saturation =
        G4LossTableManager::Instance()->EmSaturation();

    const G4double evis =
        saturation->VisibleEnergyDepositionAtAStep(step);

    fTotalEdep += edep;
    fTotalEvis += evis;

    // Scintillation光の取得
    const auto* secondaries =
        step->GetSecondaryInCurrentStep();

    if (secondaries) {
        for (const auto* secondary : *secondaries) {
            const auto* creator =
                secondary->GetCreatorProcess();

            if (secondary->GetDefinition()
                    == G4OpticalPhoton::OpticalPhotonDefinition()
                && creator
                && creator->GetProcessName()
                    == "Scintillation") {
                ++fGeneratedPhotons;
            }
        }
    }

    return true;
}

void ScintiSD::EndOfEvent(G4HCofThisEvent*) {

     auto eventAction =
        static_cast<EventAction*>(
            G4EventManager::GetEventManager()->GetUserEventAction()
        );

    if(!eventAction) return;

    auto output = eventAction->GetAnalysisOutput();
    if(!output) return;

    output->FillScinti(fTotalEdep, fTotalEvis, fGeneratedPhotons, fNeutronInteractionCount);
}