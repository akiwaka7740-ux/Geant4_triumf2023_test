#include <ROOT/RDataFrame.hxx>

#include <TCanvas.h>
#include <TF1.h>
#include <TFitResultPtr.h>
#include <TH1.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TString.h>
#include <TStyle.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kNumberOfBins = 200;
constexpr double kTimeMinimum = 0.0;
constexpr double kTimeMaximum = 20.0;

constexpr double kFitMinimum = 4.0;
constexpr double kFitMaximum = 10.0;

// BC408に設定されている減衰時定数
constexpr double kBc408DecayTime = 2.1;

// 平均に使用する最大イベント数
constexpr ULong64_t kMaximumEvents = 800;

// 解析対象のPMT
constexpr int kTargetPmtCopyNo = 0;


struct DatasetConfig {
    TString filename;
    TString label;

    double energyMinimum;
    double energyMaximum;

    int color;
};


struct AverageHistogram {
    std::unique_ptr<TH1D> normalizedShape;
    ULong64_t numberOfEvents = 0;
    double visibleFraction = 0.0;
};


AverageHistogram makeAverageHistogram(
    const DatasetConfig& dataset,
    const TString& histogramName
)
{
    ROOT::RDataFrame dataframe(
        "tree",
        dataset.filename
    );

    const std::string energyCut =
        TString::Format(
            "ScintiEdep > %.6f && ScintiEdep < %.6f",
            dataset.energyMinimum,
            dataset.energyMaximum
        ).Data();

    auto selectedEvents = dataframe

        .Filter(
            energyCut
        )

        .Filter(
            [](int pmtCopyNo) {
                return pmtCopyNo == kTargetPmtCopyNo;
            },
            {"PmtCopyNo"}
        )

        .Filter(
            [](const std::vector<double>& hitTimes) {
                return !hitTimes.empty();
            },
            {"HitTimes"}
        )

        .Range(
            kMaximumEvents
        )

        /*
        各イベントについて、
        最初にPMTへ到達した光子を0 nsに揃える。
        */
        .Define(
            "RelativeHitTimes",
            [](const std::vector<double>& hitTimes) {
                const double firstHitTime =
                    *std::min_element(
                        hitTimes.begin(),
                        hitTimes.end()
                    );

                std::vector<double> relativeHitTimes;
                relativeHitTimes.reserve(
                    hitTimes.size()
                );

                for (const double hitTime : hitTimes) {
                    relativeHitTimes.push_back(
                        hitTime - firstHitTime
                    );
                }

                return relativeHitTimes;
            },
            {"HitTimes"}
        )

        /*
        各イベントの光子分布を積分値1にする。

        光子数Nのイベントでは、
        それぞれの光子へ1/Nの重みを与える。
        */
        .Define(
            "EventNormalizedWeights",
            [](const std::vector<double>& hitTimes) {
                const double weight =
                    1.0 /
                    static_cast<double>(
                        hitTimes.size()
                    );

                return std::vector<double>(
                    hitTimes.size(),
                    weight
                );
            },
            {"HitTimes"}
        );

    auto numberOfEventsResult =
        selectedEvents.Count();

    auto histogramResult =
        selectedEvents.Histo1D(
            {
                histogramName.Data(),
                "Mean normalized photon arrival-time distribution;"
                "Relative arrival time [ns];"
                "Mean fraction of photons / event / bin",
                kNumberOfBins,
                kTimeMinimum,
                kTimeMaximum
            },
            "RelativeHitTimes",
            "EventNormalizedWeights"
        );

    const ULong64_t numberOfEvents =
        *numberOfEventsResult;

    if (numberOfEvents == 0) {
        throw std::runtime_error(
            "No events passed the selection for "
            + std::string(dataset.label.Data())
        );
    }

    auto histogram =
        std::unique_ptr<TH1D>(
            static_cast<TH1D*>(
                histogramResult->Clone(
                    (
                        histogramName
                        + "_owned"
                    ).Data()
                )
            )
        );

    histogram->SetDirectory(nullptr);

    /*
    全イベントの重み付き分布を
    イベント数で割り、イベント平均にする。
    */
    histogram->Scale(
        1.0 /
        static_cast<double>(
            numberOfEvents
        )
    );

    const double visibleFraction =
        histogram->Integral(
            1,
            histogram->GetNbinsX()
        );

    AverageHistogram result;

    result.normalizedShape =
        std::move(histogram);

    result.numberOfEvents =
        numberOfEvents;

    result.visibleFraction =
        visibleFraction;

    return result;
}


void fitAndDraw(
    AverageHistogram& result,
    const DatasetConfig& dataset
)
{
    TH1D* histogram =
        result.normalizedShape.get();

    /*
    裾を次の単一指数関数でフィットする。

        f(t) = A exp(-t / tau)

    [0] = A
    [1] = tau
    */
    auto fitFunction =
        new TF1(
            "fitExponentialTail",
            "[0] * exp(-x / [1])",
            kFitMinimum,
            kFitMaximum
        );

    fitFunction->SetParNames(
        "Amplitude",
        "Decay time"
    );

    /*
    フィット初期値。

    Aはt=0に外挿した振幅なので、
    fit開始位置のbin値から概算する。
    */
    const int firstFitBin =
        histogram->FindBin(
            kFitMinimum
        );

    double initialBinContent =
        histogram->GetBinContent(
            firstFitBin
        );

    if (initialBinContent <= 0.0) {
        initialBinContent =
            histogram->GetMaximum();
    }

    const double initialAmplitude =
        initialBinContent
        * std::exp(
            kFitMinimum
            / kBc408DecayTime
        );

    fitFunction->SetParameter(
        0,
        initialAmplitude
    );

    fitFunction->SetParameter(
        1,
        kBc408DecayTime
    );

    fitFunction->SetParLimits(
        0,
        0.0,
        10.0
    );

    fitFunction->SetParLimits(
        1,
        0.01,
        100.0
    );

    /*
    R: TF1で指定した範囲だけを使用
    S: フィット結果をTFitResultPtrとして取得
    0: フィット時には自動描画しない
    */
    const TFitResultPtr fitResult =
        histogram->Fit(
            fitFunction,
            "RS0"
        );

    const int fitStatus =
        fitResult;

    if (fitStatus != 0) {
        std::cerr
            << "Warning: fit failed with status "
            << fitStatus
            << std::endl;
    }

    const double fittedDecayTime =
        fitFunction->GetParameter(1);

    const double fittedDecayTimeError =
        fitFunction->GetParError(1);

    const double chiSquare =
        fitFunction->GetChisquare();

    const int numberOfDegreesOfFreedom =
        fitFunction->GetNDF();

    std::cout
        << std::fixed
        << std::setprecision(4)
        << dataset.label.Data()
        << '\n'
        << "  PMT copy number       = "
        << kTargetPmtCopyNo
        << '\n'
        << "  selected events       = "
        << result.numberOfEvents
        << '\n'
        << "  fit range             = "
        << kFitMinimum
        << " - "
        << kFitMaximum
        << " ns"
        << '\n'
        << "  fitted decay time     = "
        << fittedDecayTime
        << " +/- "
        << fittedDecayTimeError
        << " ns"
        << '\n'
        << "  BC408 decay time      = "
        << kBc408DecayTime
        << " ns"
        << '\n'
        << "  chi-square / ndf      = "
        << chiSquare
        << " / "
        << numberOfDegreesOfFreedom;

    if (numberOfDegreesOfFreedom > 0) {
        std::cout
            << " = "
            << chiSquare
                / numberOfDegreesOfFreedom;
    }

    std::cout
        << '\n'
        << "  visible fraction      = "
        << result.visibleFraction
        << std::endl;

    gStyle->SetOptStat(0);

    auto canvas =
        new TCanvas(
            "canvasFitAveragePhotonDistribution",
            "",
            900,
            650
        );

    // 指数関数の裾を確認しやすくする
    canvas->SetLogy();

    histogram->SetTitle(
        dataset.label.Data()
    );

    histogram->SetLineColor(
        dataset.color
    );

    histogram->SetMarkerColor(
        dataset.color
    );

    histogram->SetMarkerStyle(20);
    histogram->SetMarkerSize(0.6);
    histogram->SetLineWidth(2);

    histogram->SetMinimum(
        std::max(
            1.0e-8,
            histogram->GetMaximum()
                * 1.0e-5
        )
    );

    TH1* drawnHistogram =
        histogram->DrawCopy("E");

    fitFunction->SetLineColor(
        kMagenta + 2
    );

    fitFunction->SetLineWidth(3);
    fitFunction->Draw("SAME");

    /*
    BC408の公称設定値2.1 nsを、
    フィット振幅と同じ振幅で重ねる。
    */
    auto bc408Reference =
        new TF1(
            "bc408DecayReference",
            "[0] * exp(-x / [1])",
            kFitMinimum,
            kFitMaximum
        );

    bc408Reference->SetParameter(
        0,
        fitFunction->GetParameter(0)
    );

    bc408Reference->SetParameter(
        1,
        kBc408DecayTime
    );

    bc408Reference->SetLineColor(
        kBlue + 1
    );

    bc408Reference->SetLineStyle(7);
    bc408Reference->SetLineWidth(3);
    bc408Reference->Draw("SAME");

    auto legend =
        new TLegend(
            0.53,
            0.67,
            0.89,
            0.88
        );

    legend->SetFillStyle(0);
    legend->SetTextSize(0.03);

    legend->AddEntry(
        drawnHistogram,
        "Mean normalized waveform",
        "lep"
    );

    legend->AddEntry(
        fitFunction,
        "Exponential fit",
        "l"
    );

    legend->AddEntry(
        bc408Reference,
        "BC408: #tau = 2.1 ns",
        "l"
    );

    legend->Draw();

    auto fitInformation =
        new TPaveText(
            0.55,
            0.48,
            0.89,
            0.64,
            "NDC"
        );

    fitInformation->SetFillStyle(0);
    fitInformation->SetBorderSize(0);
    fitInformation->SetTextAlign(12);
    fitInformation->SetTextSize(0.029);

    fitInformation->AddText(
        TString::Format(
            "Fit range: %.1f - %.1f ns",
            kFitMinimum,
            kFitMaximum
        )
    );

    fitInformation->AddText(
        TString::Format(
            "#tau_{fit} = %.3f #pm %.3f ns",
            fittedDecayTime,
            fittedDecayTimeError
        )
    );

    if (numberOfDegreesOfFreedom > 0) {
        fitInformation->AddText(
            TString::Format(
                "#chi^{2}/ndf = %.2f",
                chiSquare
                    / numberOfDegreesOfFreedom
            )
        );
    }

    fitInformation->Draw();

    canvas->Update();
}


} // namespace


void FitAveragePhotonDistribution()
{
    /*
    run_20260903_155016_run000.rootは、
    CompareAveragePhotonDistribution.Cでは
    gamma, Edep = 2.0 MeVとして使用されている。
    */
    const DatasetConfig dataset = {
        "/home/hashizume/Geant4-project/triumf2023_test/"
        "root/run_20260903_204029_run000.root",

        "gamma, Edep = 2.0 MeV",

        1.950,
        2.050,

        kRed + 1
    };

    auto result =
        makeAverageHistogram(
            dataset,
            "averagePhotonDistributionForFit"
        );

    fitAndDraw(
        result,
        dataset
    );
}