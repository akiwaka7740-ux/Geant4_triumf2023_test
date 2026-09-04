#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TH2D.h>

/*
あるイベントに注目した時の、
1.光子の到達時間分布を描画する
2.光子の到達時間分布の累積分布を描画する
*/

void PMTAccumulation(TString filename){

    ROOT::RDataFrame df("tree",filename);

    //あるイベントでの光子の到達時間分布が見たい
    auto oneEvent = df
        .Define("entry","rdfentry_")
        //.Filter("Scinti_Edep>0.995 && Scinti_Edep<1.005") //for neutron 1.0 MeV
        .Filter("Scinti_Edep>0.795 && Scinti_Edep<0.805") //for gamma 1.0 MeV
        .Filter("PMT1_Photons > 0")
        .Range(1);

    /*
    //for gamma 1.0 MeV
    auto hPMTHitTimeOneEvent = oneEvent.Histo1D(
        {"hPMTHitTimeOneEvent","hPMTHitTimeOneEvent;Time [ns];Counts", 250, 100, 150},
        "PMT1_HitTimes"
    );
    */ 

    //for neutron 1.0 MeV
    auto hPMTHitTimeOneEvent = oneEvent.Histo1D(
        {"hPMTHitTimeOneEvent","hPMTHitTimeOneEvent;Time [ns];Counts", 200, 0, 50},
        "PMT1_HitTimes"
    );

    auto hPMTHitTimeOneEventAccumulated = hPMTHitTimeOneEvent->GetCumulative(kTRUE);

    const int lastBin = hPMTHitTimeOneEventAccumulated->GetNbinsX();
    const double total = hPMTHitTimeOneEventAccumulated->GetBinContent(lastBin);

    if (total > 0){
        hPMTHitTimeOneEventAccumulated->Scale(1/total);
    }

    
    TCanvas *c1 = new TCanvas("c1","c1", 800, 600);
    hPMTHitTimeOneEvent->DrawCopy();

    TCanvas *c2 = new TCanvas("c2","c2", 800, 600);
    hPMTHitTimeOneEventAccumulated->Draw();

}