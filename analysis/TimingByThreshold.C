#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TString.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

/*
光子の到達時間分布に対して、到達光子数の閾値を設定し、その閾値に達した時刻をヒストグラム化する。
20~80%の閾値を設定して、分布の変化を検証する。
*/


void TimingByThreshold(TString filename){

    TFile inputFile(filename, "READ");

    if (!inputFile.IsOpen()) {
        std::cerr << "Error: Could not open input file: " << filename << std::endl;
        return;
    }

    TTree* tree = dynamic_cast<TTree*>(inputFile.Get("tree"));

    if (!tree) {
        std::cerr << "Error: Could not find TTree 'tree' in file: " << filename << std::endl;
        return;
    }

    std::vector<double>* hitTimes = nullptr;

    if(tree->SetBranchAddress("PMT1_HitTimes", &hitTimes) != 0) {
        std::cerr << "Error: Could not set branch address for 'PMT1_HitTimes'" << std::endl;
        return;
    }

    const double threshold01 = 0.2; // 20% threshold
    const double threshold02 = 0.4; // 40% threshold
    const double threshold03 = 0.6; // 60% threshold
    const double threshold04 = 0.8; // 80% threshold

    auto histogram01 = new TH1D("histogram01", "Histogram of PMT1 Hit Times ", 200, 100.0, 140.0);
    auto histogram02 = new TH1D("histogram02", "Histogram of PMT1 Hit Times ", 200, 100.0, 140.0);
    auto histogram03 = new TH1D("histogram03", "Histogram of PMT1 Hit Times ", 200, 100.0, 140.0);
    auto histogram04 = new TH1D("histogram04", "Histogram of PMT1 Hit Times ", 200, 100.0, 140.0);


    //所有権を明示的に放棄して、ファイルを閉じてもヒストグラムが保持されるようにする
    histogram01->SetDirectory(nullptr);
    histogram02->SetDirectory(nullptr);
    histogram03->SetDirectory(nullptr);
    histogram04->SetDirectory(nullptr);


    const int nEntries = tree->GetEntries();

    //イベントごとに処理を実行
    for (int event = 0; event < nEntries; ++event) {

        tree->GetEntry(event);

        //到達光子がないイベントはスキップ
        if (!hitTimes || hitTimes->empty()) {
            continue;
        }

        const std::size_t numberOfPhotons = hitTimes->size();

        //保存順が時間順とは限らないので、時間順にソート
        std::vector<double> sortedHitTimes = *hitTimes;
        std::sort(sortedHitTimes.begin(), sortedHitTimes.end());

        const std::size_t thresholdPhotonNumber01 = static_cast<std::size_t>(std::ceil(threshold01 * numberOfPhotons));
        const std::size_t thresholdIndex01 = thresholdPhotonNumber01 - 1;

        const std::size_t thresholdPhotonNumber02 = static_cast<std::size_t>(std::ceil(threshold02 * numberOfPhotons));
        const std::size_t thresholdIndex02 = thresholdPhotonNumber02 - 1;

        const std::size_t thresholdPhotonNumber03 = static_cast<std::size_t>(std::ceil(threshold03 * numberOfPhotons));
        const std::size_t thresholdIndex03 = thresholdPhotonNumber03 - 1;

        const std::size_t thresholdPhotonNumber04 = static_cast<std::size_t>(std::ceil(threshold04 * numberOfPhotons));
        const std::size_t thresholdIndex04 = thresholdPhotonNumber04 - 1;

        const double thresholdTime01 = sortedHitTimes[thresholdIndex01];
        const double thresholdTime02 = sortedHitTimes[thresholdIndex02];
        const double thresholdTime03 = sortedHitTimes[thresholdIndex03];
        const double thresholdTime04 = sortedHitTimes[thresholdIndex04];

        histogram01->Fill(thresholdTime01);
        histogram02->Fill(thresholdTime02);
        histogram03->Fill(thresholdTime03);
        histogram04->Fill(thresholdTime04);

    }

    inputFile.Close();

    histogram01->GetXaxis()->SetTitle("Time [ns]");
    histogram01->GetYaxis()->SetTitle("Counts");
    histogram01->SetLineColor(kRed);

    histogram02->GetXaxis()->SetTitle("Time [ns]");
    histogram02->GetYaxis()->SetTitle("Counts");
    histogram02->SetLineColor(kOrange+2);

    histogram03->GetXaxis()->SetTitle("Time [ns]");
    histogram03->GetYaxis()->SetTitle("Counts");
    histogram03->SetLineColor(kGreen+2);

    histogram04->GetXaxis()->SetTitle("Time [ns]");
    histogram04->GetYaxis()->SetTitle("Counts");
    histogram04->SetLineColor(kBlue);


    auto canvas = new TCanvas("canvas", "", 800, 600);
    gStyle->SetOptStat(0); // 統計ボックスを非表示にする

    histogram01->Draw();
    histogram02->Draw("SAME");
    histogram03->Draw("SAME");
    histogram04->Draw("SAME");


    auto legend = new TLegend(0.6, 0.6, 0.9, 0.9);
    legend->AddEntry(histogram01, "Threshold 0.2", "l");
    legend->AddEntry(histogram02, "Threshold 0.4", "l");
    legend->AddEntry(histogram03, "Threshold 0.6", "l");
    legend->AddEntry(histogram04, "Threshold 0.8", "l");
    legend->Draw("SAME");

}
