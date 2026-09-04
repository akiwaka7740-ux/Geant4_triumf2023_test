#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TH2D.h>

#include <vector>

/*
中性子エネルギーに対する光子の発生数および到達数を比較する
*/

// 生成光子のヒストグラムを作成する関数
ROOT::RDF::RResultPtr<TH1D> hGeneratedPhotons(TString filename, const TString& histName){

    ROOT::RDataFrame df("tree",filename);

    auto selected = df.Filter("Scinti_Photons != 0");

    auto hGeneratedPhotons = selected.Histo1D(
        {histName, "hGeneratedPhotons;Generated Photons;Counts", 1250, 0, 5000},
        "Scinti_Photons"
    );

    return hGeneratedPhotons;
   
}

// 到達光子のヒストグラムを作成する関数
ROOT::RDF::RResultPtr<TH1D> hArrivedPhotons(TString filename, const TString& histName){

    ROOT::RDataFrame df("tree",filename);

    auto selected = df.Filter("PMT1_Photons != 0");

    auto hDetectedPhotons = selected.Histo1D(
        {histName, "hDetectedPhotons;Detected Photons;Counts", 800, 0, 800},
        "PMT1_Photons"
    );

    return hDetectedPhotons;
}

std::unique_ptr<TH1> hArrivedPhotonsAccumulated(TString filename, const TString& histName){

 ROOT::RDataFrame df("tree", filename);

    auto selected = df.Filter("PMT1_Photons != 0");

    auto hDetectedPhotons = selected.Histo1D(
        {histName, "hDetectedPhotons;Detected Photons;Counts", 800, 0, 800},
        "PMT1_Photons"
    );

    auto hDetectedPhotonsAccumulated =
        std::unique_ptr<TH1>(hDetectedPhotons->GetCumulative(kTRUE));

    hDetectedPhotonsAccumulated->SetDirectory(nullptr);

    const int lastBin = hDetectedPhotonsAccumulated->GetNbinsX();
    const double total = hDetectedPhotonsAccumulated->GetBinContent(lastBin);

    if (total > 0) {
        hDetectedPhotonsAccumulated->Scale(1.0 / total);
    }

    return hDetectedPhotonsAccumulated;
}



void ComparePhotons(){

    std::vector<TString> filenames = {
        "/home/hashizume/Geant4-project/triumf2023_test/root/run_20260804_150952_run000.root",
        "/home/hashizume/Geant4-project/triumf2023_test/root/run_20260804_150508_run000.root",
        "/home/hashizume/Geant4-project/triumf2023_test/root/run_20260804_150125_run000.root",
        "/home/hashizume/Geant4-project/triumf2023_test/root/run_20260804_144905_run000.root"
    };
        
        
    std::vector<ROOT::RDF::RResultPtr<TH1D>> histogramsGeneratedPhotons;
    std::vector<ROOT::RDF::RResultPtr<TH1D>> histogramsArrivedPhotons;
    std::vector<std::unique_ptr<TH1>> histogramsArrivedPhotonsAccumulated;

    std::vector<double> neutronEnergies = {0.5, 1.0, 1.5, 2.0}; // MeV

    for (size_t i = 0; i < filenames.size(); ++i) {
        TString histName = "hGeneratedPhotons_" + TString::Format("%.1f", neutronEnergies[i]) + "MeV";
        histogramsGeneratedPhotons.push_back(hGeneratedPhotons(filenames[i], histName));      
        
        TString histNameArrived = "hArrivedPhotons_" + TString::Format("%.1f", neutronEnergies[i]) + "MeV";
        histogramsArrivedPhotons.push_back(hArrivedPhotons(filenames[i], histNameArrived));      

        TString histNameArrivedAccumulated = "hArrivedPhotonsAccumulated_" + TString::Format("%.1f", neutronEnergies[i]) + "MeV";
        histogramsArrivedPhotonsAccumulated.push_back(hArrivedPhotonsAccumulated(filenames[i], histNameArrivedAccumulated));      
    }

    gStyle->SetOptStat(0); // 統計ボックスを非表示にする

    // 生成光子のヒストグラムを描画
    auto canvas1 = new TCanvas("canvas1", "", 800, 600);
    
    histogramsGeneratedPhotons[0]->SetLineColorAlpha(kRed, 0.4);
    histogramsGeneratedPhotons[0]->SetLineWidth(2);
    auto* drawn0 = histogramsGeneratedPhotons[0]->DrawCopy("HIST");

    histogramsGeneratedPhotons[1]->SetLineColorAlpha(kOrange + 1, 0.4);
    histogramsGeneratedPhotons[1]->SetLineWidth(2);
    auto* drawn1 = histogramsGeneratedPhotons[1]->DrawCopy("HIST SAME");

    histogramsGeneratedPhotons[2]->SetLineColorAlpha(kGreen, 0.4);
    histogramsGeneratedPhotons[2]->SetLineWidth(2);
    auto* drawn2 = histogramsGeneratedPhotons[2]->DrawCopy("HIST SAME");

    histogramsGeneratedPhotons[3]->SetLineColorAlpha(kBlue, 0.4);
    histogramsGeneratedPhotons[3]->SetLineWidth(2);
    auto* drawn3 = histogramsGeneratedPhotons[3]->DrawCopy("HIST SAME");

    std::vector<TH1*> drawnHistograms = {
        drawn0, drawn1, drawn2, drawn3
    };

    auto legend = new TLegend(0.65, 0.65, 0.9, 0.9);

    for (size_t i = 0; i < drawnHistograms.size(); ++i) {
        legend->AddEntry(
            drawnHistograms[i],
            TString::Format("%.1f MeV", neutronEnergies[i]),
            "l"
        );
    }

    legend->Draw();

    canvas1->SetLogy();
    canvas1->Update();

    // 到達光子のヒストグラムを描画
    auto canvas2 = new TCanvas("canvas2", "", 800, 600);    

    histogramsArrivedPhotons[0]->SetLineColorAlpha(kRed, 0.4);
    histogramsArrivedPhotons[0]->SetLineWidth(2);
    auto* drawn4 = histogramsArrivedPhotons[0]->DrawCopy("HIST");

    histogramsArrivedPhotons[1]->SetLineColorAlpha(kOrange + 1, 0.4);
    histogramsArrivedPhotons[1]->SetLineWidth(2);
    auto* drawn5 = histogramsArrivedPhotons[1]->DrawCopy("HIST SAME");

    histogramsArrivedPhotons[2]->SetLineColorAlpha(kGreen, 0.4);
    histogramsArrivedPhotons[2]->SetLineWidth(2);
    auto* drawn6 = histogramsArrivedPhotons[2]->DrawCopy("HIST SAME");

    histogramsArrivedPhotons[3]->SetLineColorAlpha(kBlue, 0.4);
    histogramsArrivedPhotons[3]->SetLineWidth(2);
    auto* drawn7 = histogramsArrivedPhotons[3]->DrawCopy("HIST SAME");

    std::vector<TH1*> drawnHistogramsArrived = {
        drawn4, drawn5, drawn6, drawn7
    };

    auto legend2 = new TLegend(0.65, 0.65, 0.9, 0.9);       

    for (size_t i = 0; i < drawnHistogramsArrived.size(); ++i) {
        legend2->AddEntry(
            drawnHistogramsArrived[i],
            TString::Format("%.1f MeV", neutronEnergies[i]),
            "l"
        );
    }

    legend2->Draw();

    canvas2->SetLogy();
    canvas2->Update();

    // 到達光子の累積ヒストグラムを描画
    auto canvas3 = new TCanvas("canvas3", "", 800, 600);

    histogramsArrivedPhotonsAccumulated[0]->SetLineColorAlpha(kRed, 0.4);
    histogramsArrivedPhotonsAccumulated[0]->SetLineWidth(2);
    auto* drawn8 = histogramsArrivedPhotonsAccumulated[0]->DrawCopy("HIST");

    histogramsArrivedPhotonsAccumulated[1]->SetLineColorAlpha(kOrange + 1, 0.4);
    histogramsArrivedPhotonsAccumulated[1]->SetLineWidth(2);
    auto* drawn9 = histogramsArrivedPhotonsAccumulated[1]->DrawCopy("HIST SAME");

    histogramsArrivedPhotonsAccumulated[2]->SetLineColorAlpha(kGreen, 0.4);
    histogramsArrivedPhotonsAccumulated[2]->SetLineWidth(2);
    auto* drawn10 = histogramsArrivedPhotonsAccumulated[2]->DrawCopy("HIST SAME");  

    histogramsArrivedPhotonsAccumulated[3]->SetLineColorAlpha(kBlue, 0.4);
    histogramsArrivedPhotonsAccumulated[3]->SetLineWidth(2);
    auto* drawn11 = histogramsArrivedPhotonsAccumulated[3]->DrawCopy("HIST SAME");

    std::vector<TH1*> drawnHistogramsArrivedAccumulated = {
        drawn8, drawn9, drawn10, drawn11
    };

    auto legend3 = new TLegend(0.65, 0.65, 0.9, 0.9);

    for (size_t i = 0; i < drawnHistogramsArrivedAccumulated.size(); ++i) {
        legend3->AddEntry(
            drawnHistogramsArrivedAccumulated[i],
            TString::Format("%.1f MeV", neutronEnergies[i]),
            "l"
        );
    }
    legend3->Draw();

    canvas3->SetLogy();
    canvas3->Update();

}
