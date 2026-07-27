#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TH2D.h>



TString MakePdfPath(TString filename){

    TString base = gSystem->BaseName(filename);
    base.ReplaceAll(".root","");

    return "logs/" + base + "/" + "results.pdf";
}

std::vector<ROOT::RDF::RResultPtr<TH1D>> gHistograms1D;
std::vector<ROOT::RDF::RResultPtr<TH2D>> gHistograms2D;
std::vector<ROOT::RDF::RResultPtr<TH3D>> gHistograms3D;


void PlotFigure(TString filename){
    ROOT::RDataFrame df("tree",filename);

    auto selected = df
        .Filter("Scinti_Edep!=0");

    
    auto hScintiEdep = selected.Histo1D(
        {"hScintiEdep","hScintiEdep;Energy [MeV];Counts",1200,0,1.2},
        "Scinti_Edep"
    );

    gHistograms1D.push_back(hScintiEdep);
    
    
    auto hScintiEvis = selected.Histo1D(
        {"hScintiEvis","hScintiEvis;Energy [MeV];Counts", 200, 0, 0.2},
        "Scinti_Evis"
    );

    gHistograms1D.push_back(hScintiEvis);

    auto hScintiPhotons = selected.Histo1D(
        {"hScintiPhotons","hScintiPhotons;Counts ;Counts", 2000,0,2000},
        "Scinti_Photons"
    );

    gHistograms1D.push_back(hScintiPhotons);

    auto hScintiHitTime = selected.Histo1D(
        {"hScintiHitTime","hScintiHitTimes;Time [ns];Counts", 400, 100, 140},
        "Scinti_HitTime"
    );
    
    gHistograms1D.push_back(hScintiHitTime);
    
    auto hScintiInteractionCount = selected.Histo1D(
        {"hScintiInteractionCount","hScintiInteractionCount; Interaction Counts; Counts", 100, 0, 100},
        "Scinti_InteractionCount"
    );

    gHistograms1D.push_back(hScintiInteractionCount);


    auto selected_1MeV = df
        .Filter("Scinti_Edep>0.995 && Scinti_Edep<1.005");

    auto hScintiPhotons_vs_InteractionCount = selected_1MeV.Histo2D(
        {"hScintiPhotons_vs_InteractionCount","hScintiPhotons_vs_InteractionCount;Photons;Interaction Count", 175, 600, 2000, 50, 0, 50},
        "Scinti_Photons",
        "Scinti_InteractionCount"
    );

    gHistograms2D.push_back(hScintiPhotons_vs_InteractionCount);
    
    auto hPMT1Photons = selected.Histo1D(
        {"hPMT1Photons","hPMT1Photons;Counts;Counts", 200, 0, 200},
        "PMT1_Photons"
    );

    
    gHistograms1D.push_back(hPMT1Photons);


    auto hPMT1Efficiency = selected.Histo1D(
        {"hPMT1Efficiency","hPMT1Efficiency;Efficiency;Counts", 200, 0, 0.2},
        "PMT1_Efficiency"
    );

    gHistograms1D.push_back(hPMT1Efficiency);


    auto hPMT1Efficiency_PMT1Photons = selected.Histo2D(
        {"hPMT1Effiency_PMT1Photons","hPMT1Effiency_PMT1Photons;Efficiency;Counts", 200, 0, 0.2, 250, 0, 250},
        "PMT1_Efficiency",
        "PMT1_Photons"
    );

    gHistograms2D.push_back(hPMT1Efficiency_PMT1Photons);


    auto hPMT1Efficiency_ScintiHitPosRadius = selected.Histo2D(
        {"hPMT1Efficiency_ScintiHitPosRadius","hPMT1Efficiency_ScintiHitPosRadius;Radius [mm];Efficiency", 150, 0, 150, 100, 0, 1.0},
        "Scinti_HitPos_Radius",
        "PMT1_Efficiency"
    );

    gHistograms2D.push_back(hPMT1Efficiency_ScintiHitPosRadius);
    
    auto pmt1Selected = df
        .Filter("PMT1_Photons!=0")
        .Define("PMT1_FirstHitTime", "PMT1_HitTimes[0]");

    auto hPMT1FirstHitTime = pmt1Selected.Histo1D(
        {"hPMT1FirstHitTime","hPMT1FirstHitTime;Time [ns];Counts", 200, 80, 280},
        "PMT1_FirstHitTime"
    );

    gHistograms1D.push_back(hPMT1FirstHitTime);

    //あるイベントでの光子の到達時間分布が見たい
    auto oneEvent = df
        .Define("entry","rdfentry_")
        .Filter("Scinti_Edep>0.995 && Scinti_Edep<1.005")
        .Filter("PMT1_Photons > 0")
        .Range(1);

    auto hPMTHitTimeOneEvent = oneEvent.Histo1D(
        {"hPMTHitTimeOneEvent","hPMTHitTimeOneEvent;Time [ns];Counts", 80, 100, 140},
        "PMT1_HitTimes"
    );

    gHistograms1D.push_back(hPMTHitTimeOneEvent);

    /*
    auto h3D = selected.Histo3D(
        {"h3D","h3D; Energy [MeV]; Photons; Interaction Count", 100, 0, 1.0, 200, 0, 0.2, 30, 0, 30},
        "Scinti_Edep",
        "Scinti_Photons",
        "Scinti_InteractionCount"
    );

    gHistograms3D.push_back(h3D);
    */

    auto report = selected.Report();
    report->Print();

    TString pdfPath = MakePdfPath(filename);

    TCanvas *c1 = new TCanvas("c1","c1", 800, 600);
    hScintiEdep->Draw();
    c1->SetLogy();
    c1->Print(pdfPath + "(");

    TCanvas *c2 = new TCanvas("c2","c2", 800, 600);
    hScintiEvis->Draw();
    c2->Print(pdfPath);

    TCanvas *c3 = new TCanvas("c3","c3", 800, 600);
    hScintiPhotons->Draw();
    c3->Print(pdfPath);

    TCanvas *c4 = new TCanvas("c4","c4", 800, 600);
    hScintiInteractionCount->Draw();
    c4->SetLogy();
    c4->Print(pdfPath);

    TCanvas *c5 = new TCanvas("c5","c5", 800, 600);
    hScintiHitTime->Draw();
    c5->Print(pdfPath);

    TCanvas *c6 = new TCanvas("c6","c6", 800, 600);
    hScintiPhotons_vs_InteractionCount->Draw("colz");
    c6->Print(pdfPath);

    TCanvas *c7 = new TCanvas("c7","c7", 800, 600);
    hPMT1Photons->Draw();
    c7->Print(pdfPath);

    TCanvas *c8 = new TCanvas("c8","c8", 800, 600);
    hPMT1Efficiency->Draw();
    c8->Print(pdfPath);

    TCanvas *c9 = new TCanvas("c9","c9", 800, 600);
    hPMT1Efficiency_PMT1Photons->Draw("colz");
    c9->Print(pdfPath);

    TCanvas *c10 = new TCanvas("c10","c10", 800, 600);
    hPMT1Efficiency_ScintiHitPosRadius->Draw("colz");
    c10->Print(pdfPath);

    TCanvas *c11 = new TCanvas("c11","c11", 800, 600);
    hPMT1FirstHitTime->Draw();
    c11->Print(pdfPath);

    TCanvas *c12 = new TCanvas("c12","c12", 800, 600);
    hPMTHitTimeOneEvent->Draw();
    c12->Print(pdfPath + ")");

}




    