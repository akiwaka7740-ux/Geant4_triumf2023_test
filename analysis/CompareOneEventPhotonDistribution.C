#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TString.h>
#include <TStyle.h>

#include <algorithm>
#include <memory>
#include <vector>

/*
あるイベントに着目した際の、中性子とγ線での光子の到達数を比較する
*/

// 到達光子のヒストグラムを作成する関数
ROOT::RDF::RResultPtr<TH1D> makeArrivedPhotonsDistribution(TString filename, const TString& histName, double energyMin, double energyMax){

    ROOT::RDataFrame df("tree",filename);

    const std::string energyCut =
    TString::Format(
        "Scinti_Edep>%.3f && Scinti_Edep<%.3f",
        energyMin,
        energyMax
    ).Data();


    auto oneEvent = df
    .Filter(energyCut)
    .Filter(
        [](const std::vector<double>& hitTimes) {
            return !hitTimes.empty();
        },
        {"PMT1_HitTimes"}
    )
    // イベントごとに最初の到達時刻を引く
    .Define(
        "PMT1_RelativeHitTimes",
        [](const std::vector<double>& hitTimes) {
            const double firstHitTime =
                *std::min_element(hitTimes.begin(), hitTimes.end());

            std::vector<double> relativeHitTimes;
            relativeHitTimes.reserve(hitTimes.size());

            for (const double hitTime : hitTimes) {
                relativeHitTimes.push_back(hitTime - firstHitTime);
            }

            return relativeHitTimes;
        },
        {"PMT1_HitTimes"}
    )
    .Range(1);


    auto hArrivedPhotonsDistribution = oneEvent.Histo1D(
        {histName, "hArrivedPhotonsDistribution;Detected Photons;Counts", 160, 0.0, 40.0},
        "PMT1_RelativeHitTimes"
    );

    return hArrivedPhotonsDistribution;
}


void CompareOneEventPhotonDistribution(){

    std::vector<TString> filenames = {
        "/home/hashizume/Geant4-project/triumf2023_test/root/run_20260805_185811_run000.root", //gamma 1.0 MeV
        "/home/hashizume/Geant4-project/triumf2023_test/root/run_20260805_185811_run000.root", //gamma 1.0 MeV
        "/home/hashizume/Geant4-project/triumf2023_test/root/run_20260804_150508_run000.root"  //neutron 1.0 MeV
    };

    std::vector<ROOT::RDF::RResultPtr<TH1D>> hArrivedPhotonsDistribution;
    std::vector<std::unique_ptr<TH1D>> hArrivedPhotonsDistributionNormalized;
    std::vector<std::unique_ptr<TH1D>> hArrivedPhotonsDistributionAccumulated;


    const std::vector<double> energyMin = {0.795, 0.095, 0.995}; // MeV
    const std::vector<double> energyMax = {0.805, 0.105, 1.005}; // MeV

    for (size_t i = 0; i < filenames.size(); ++i) {
        TString histName = "hArrivedPhotonsDistribution_" + TString::Format("%.1f", (energyMin[i] + energyMax[i]) / 2.0) + "MeV";
        hArrivedPhotonsDistribution.push_back(makeArrivedPhotonsDistribution(filenames[i], histName, energyMin[i], energyMax[i]));

        // ヒストグラムを正規化する
        auto copy = std::unique_ptr<TH1D>(
            static_cast<TH1D*>(
                hArrivedPhotonsDistribution[i]->Clone(
                    (histName + "_normalized").Data()
                )
            )
        );

        copy->SetDirectory(nullptr);

        const double total =
            copy->Integral(1, copy->GetNbinsX());

        if (total > 0.0) {
            copy->Scale(1.0 / total);
        }

        hArrivedPhotonsDistributionNormalized.push_back(
            std::move(copy)
        );

        // ヒストグラムを累積分布に変換する
        auto accumulatedCopy = std::unique_ptr<TH1D>(
            static_cast<TH1D*>(
                hArrivedPhotonsDistributionNormalized[i]->GetCumulative(kTRUE)
            )
        );

        accumulatedCopy->SetDirectory(nullptr);
        hArrivedPhotonsDistributionAccumulated.push_back(std::move(accumulatedCopy));

    }

    gStyle->SetOptStat(0); // 統計ボックスを非表示にする

    // 到達光子のヒストグラムを描画
    auto canvas1 = new TCanvas("canvas1", "", 800, 600);

    //auto colors = std::vector<int>{kRed, kOrange + 1, kBlue};

    const std::vector<int> colors = {
        kRed + 1,
        kOrange + 7,
        kBlue + 1
    };


    std::vector<TH1*> drawnHistograms;
    drawnHistograms.reserve(filenames.size());

    
    for (size_t i = 0; i < filenames.size(); ++i) {
        auto& histogram = hArrivedPhotonsDistribution[i];

        histogram->SetLineColorAlpha(colors[i], 0.2);
        histogram->SetLineWidth(2);

        const char* drawOption =
            (i == 0) ? "HIST" : "HIST SAME";

        TH1* drawnHistogram =
            histogram->DrawCopy(drawOption);

        drawnHistograms.push_back(drawnHistogram);
    }

    auto legend1 =
        new TLegend(0.57, 0.57, 0.9, 0.9);

    legend1->AddEntry(
        drawnHistograms[0],
        "gamma, Edep = 0.8 MeV",
        "l"
    );
    legend1->AddEntry(
        drawnHistograms[1],
        "gamma, Edep = 0.1 MeV",
        "l"
    );
    legend1->AddEntry(
        drawnHistograms[2],
        "neutron, Edep = 1.0 MeV",
        "l"
    );

    legend1->SetTextSize(0.03);
    legend1->Draw();


    // 到達光子のヒストグラム(正規化)を描画
    auto canvas2 = new TCanvas("canvas2", "", 800, 600);

    std::vector<TH1*> drawnNormalizedHistograms;
    drawnNormalizedHistograms.reserve(filenames.size());


    for (size_t i = 0; i < filenames.size(); ++i) {
        auto& histogram =
            hArrivedPhotonsDistributionNormalized[i];

        histogram->SetLineColorAlpha(colors[i], 0.2);
        histogram->SetLineWidth(2);


        if (i == 0) {
            histogram->SetMinimum(0.0);
            histogram->SetMaximum(0.1);
        }

        const char* drawOption =
            (i == 0) ? "HIST" : "HIST SAME";

        TH1* drawnHistogram =
            histogram->DrawCopy(drawOption);

        drawnNormalizedHistograms.push_back(drawnHistogram);
    }
    

    auto legend2 =
        new TLegend(0.57, 0.57, 0.9, 0.9);

    legend2->AddEntry(
        drawnNormalizedHistograms[0],
        "gamma, Edep = 0.8 MeV",
        "l"
    );
    legend2->AddEntry(
        drawnNormalizedHistograms[1],
        "gamma, Edep = 0.1 MeV",
        "l"
    );
    legend2->AddEntry(
        drawnNormalizedHistograms[2],
        "neutron, Edep = 1.0 MeV",
        "l"
    );

    legend2->SetTextSize(0.03);
    legend2->Draw();

    // 到達光子のヒストグラム(累積分布)を描画
    auto canvas3 = new TCanvas("canvas3", "", 800, 600);

    std::vector<TH1*> drawnAccumulatedHistograms;
    drawnAccumulatedHistograms.reserve(filenames.size());

    for (size_t i = 0; i < filenames.size(); ++i) {
        auto& histogram =
            hArrivedPhotonsDistributionAccumulated[i];

        histogram->SetLineColorAlpha(colors[i], 0.2);
        histogram->SetLineWidth(2);

        if (i == 0) {
            histogram->SetMinimum(0.0);
            histogram->SetMaximum(1.0);
        }
    
    
        const char* drawOption =
            (i == 0) ? "HIST" : "HIST SAME";

        TH1* drawnHistogram =
            histogram->DrawCopy(drawOption);

        drawnAccumulatedHistograms.push_back(drawnHistogram);
    }

    auto legend3 =
        new TLegend(0.57, 0.57, 0.9, 0.9);
    
    legend3->AddEntry(
        drawnAccumulatedHistograms[0],
        "gamma, Edep = 0.8 MeV",
        "l"
    );
    legend3->AddEntry(
        drawnAccumulatedHistograms[1],
        "gamma, Edep = 0.1 MeV",
        "l"
    );
    legend3->AddEntry(
        drawnAccumulatedHistograms[2],
        "neutron, Edep = 1.0 MeV",
        "l"
    );

    legend3->SetTextSize(0.03);
    legend3->Draw();


}