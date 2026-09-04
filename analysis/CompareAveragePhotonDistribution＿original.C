#include <ROOT/RDataFrame.hxx>

#include <TCanvas.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TString.h>
#include <TStyle.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kNumberOfBins = 160;
constexpr double kTimeMinimum = 0.0;
constexpr double kTimeMaximum = 40.0;

// 各条件で平均に使用する最大イベント数
constexpr ULong64_t kMaximumEvents = 1000;


struct DatasetConfig {
    TString filename;
    TString label;

    double energyMinimum;
    double energyMaximum;

    int color;
    int lineStyle;
};


struct AverageHistograms {
    // 1イベント当たりの平均到達光子数
    std::unique_ptr<TH1D> meanPhotonCounts;

    // 各イベントを個別に正規化してから平均した分布
    std::unique_ptr<TH1D> meanNormalizedShape;

    // meanNormalizedShapeから作成した正規化CDF
    std::unique_ptr<TH1D> cumulativeShape;

    ULong64_t numberOfEvents = 0;

    // 0～40 nsに入っている平均光子割合
    double visibleFraction = 0.0;
};


// 条件を満たす最大kMaximumEvents個のイベントから平均分布を作る
AverageHistograms makeAverageHistograms(
    const DatasetConfig& dataset,
    const TString& histogramBaseName
)
{
    ROOT::RDataFrame dataframe(
        "tree",
        dataset.filename
    );

    const std::string energyCut =
        TString::Format(
            "Scinti_Edep>%.3f && Scinti_Edep<%.3f",
            dataset.energyMinimum,
            dataset.energyMaximum
        ).Data();

    auto selectedEvents = dataframe

        // エネルギー条件
        .Filter(energyCut)

        // PMT1に光子が到達したイベントだけを選択
        .Filter(
            [](const std::vector<double>& hitTimes) {
                return !hitTimes.empty();
            },
            {"PMT1_HitTimes"}
        )

        /*
        フィルターを通過したイベントのうち、
        最大kMaximumEvents個だけを使用する
        */
        .Range(kMaximumEvents)

        // 各イベントの最初の到達光子を0 nsに揃える
        .Define(
            "PMT1_RelativeHitTimes",
            [](const std::vector<double>& hitTimes) {
                const double firstHitTime =
                    *std::min_element(
                        hitTimes.begin(),
                        hitTimes.end()
                    );

                std::vector<double> relativeHitTimes;
                relativeHitTimes.reserve(hitTimes.size());

                for (const double hitTime : hitTimes) {
                    relativeHitTimes.push_back(
                        hitTime - firstHitTime
                    );
                }

                return relativeHitTimes;
            },
            {"PMT1_HitTimes"}
        )

        /*
        各イベントの総重みを1にする。

        到達光子数がN個のイベントでは、
        各光子へ1/Nの重みを与える。
        */
        .Define(
            "PMT1_EventNormalizedWeights",
            [](const std::vector<double>& hitTimes) {
                const double weight =
                    1.0 /
                    static_cast<double>(hitTimes.size());

                return std::vector<double>(
                    hitTimes.size(),
                    weight
                );
            },
            {"PMT1_HitTimes"}
        );

    const TString meanCountsName =
        histogramBaseName + "_meanCounts";

    const TString meanShapeName =
        histogramBaseName + "_meanNormalizedShape";

    // 実際に選択されたイベント数
    auto numberOfEventsResult =
        selectedEvents.Count();

    /*
    重みなしヒストグラム。

    あとでイベント数で割ることで、
    1イベント当たりの平均到達光子数になる。
    */
    auto meanPhotonCountsResult =
        selectedEvents.Histo1D(
            {
                meanCountsName.Data(),
                "Mean photon arrival-time distribution;"
                "Relative arrival time [ns];"
                "Mean detected photons / event / bin",
                kNumberOfBins,
                kTimeMinimum,
                kTimeMaximum
            },
            "PMT1_RelativeHitTimes"
        );

    /*
    イベント内正規化用の重みを使用する。

    光子数が多いイベントと少ないイベントを、
    波形形状の平均に対して同等に扱う。
    */
    auto meanNormalizedShapeResult =
        selectedEvents.Histo1D(
            {
                meanShapeName.Data(),
                "Mean normalized photon arrival-time distribution;"
                "Relative arrival time [ns];"
                "Mean fraction of photons / event / bin",
                kNumberOfBins,
                kTimeMinimum,
                kTimeMaximum
            },
            "PMT1_RelativeHitTimes",
            "PMT1_EventNormalizedWeights"
        );

    /*
    CountとHisto1Dをすべて定義した後に値を取得する。
    同じRDataFrameのイベントループで処理される。
    */
    const ULong64_t numberOfEvents =
        *numberOfEventsResult;

    if (numberOfEvents == 0) {
        throw std::runtime_error(
            "No events passed the selection for "
            + std::string(dataset.label.Data())
        );
    }

    auto meanPhotonCounts =
        std::unique_ptr<TH1D>(
            static_cast<TH1D*>(
                meanPhotonCountsResult->Clone(
                    (meanCountsName + "_owned").Data()
                )
            )
        );

    auto meanNormalizedShape =
        std::unique_ptr<TH1D>(
            static_cast<TH1D*>(
                meanNormalizedShapeResult->Clone(
                    (meanShapeName + "_owned").Data()
                )
            )
        );

    meanPhotonCounts->SetDirectory(nullptr);
    meanNormalizedShape->SetDirectory(nullptr);

    /*
    全光子数をイベント数で割る。

    各ビンの値は、
    1イベント当たりの平均到達光子数になる。
    */
    meanPhotonCounts->Scale(
        1.0 /
        static_cast<double>(numberOfEvents)
    );

    /*
    重み付きヒストグラムをイベント数で割る。

    各ビンの値は、
    イベントごとに正規化した波形の平均になる。
    */
    meanNormalizedShape->Scale(
        1.0 /
        static_cast<double>(numberOfEvents)
    );

    /*
    表示範囲内に入っている光子割合。

    40 nsより遅い光子はオーバーフローに入るため、
    visibleFractionが1未満になる場合がある。
    */
    const double visibleFraction =
        meanNormalizedShape->Integral(
            1,
            meanNormalizedShape->GetNbinsX()
        );

    // 平均正規化分布から累積分布を作る
    auto cumulativeShape =
        std::unique_ptr<TH1D>(
            static_cast<TH1D*>(
                meanNormalizedShape->GetCumulative(
                    kTRUE,
                    "_cdf"
                )
            )
        );

    cumulativeShape->SetDirectory(nullptr);


    cumulativeShape->SetTitle(
        "Mean cumulative photon arrival-time distribution"
    );

    cumulativeShape->GetXaxis()->SetTitle(
        "Relative arrival time [ns]"
    );

    cumulativeShape->GetYaxis()->SetTitle(
        "Normalized cumulative fraction"
    );

    AverageHistograms result;

    result.meanPhotonCounts =
        std::move(meanPhotonCounts);

    result.meanNormalizedShape =
        std::move(meanNormalizedShape);

    result.cumulativeShape =
        std::move(cumulativeShape);

    result.numberOfEvents =
        numberOfEvents;

    result.visibleFraction =
        visibleFraction;

    return result;
}


void setHistogramStyle(
    TH1* histogram,
    const DatasetConfig& dataset
)
{
    histogram->SetLineColor(dataset.color);
    histogram->SetLineStyle(dataset.lineStyle);
    histogram->SetLineWidth(3);
}


TLegend* makeLegend(
    const std::vector<TH1*>& drawnHistograms,
    const std::vector<DatasetConfig>& datasets
)
{
    auto legend =
        new TLegend(0.53, 0.58, 0.90, 0.90);

    legend->SetTextSize(0.028);
    legend->SetFillStyle(0);

    for (std::size_t i = 0;
         i < drawnHistograms.size();
         ++i) {

        legend->AddEntry(
            drawnHistograms[i],
            datasets[i].label.Data(),
            "l"
        );
    }

    return legend;
}


// 複数ヒストグラムを同じキャンバスへ描画する
TCanvas* drawComparison(
    const char* canvasName,
    const std::vector<TH1D*>& histograms,
    const std::vector<DatasetConfig>& datasets,
    bool useFixedMaximum,
    double fixedMaximum
)
{
    auto canvas =
        new TCanvas(
            canvasName,
            "",
            900,
            650
        );

    std::vector<TH1*> drawnHistograms;
    drawnHistograms.reserve(histograms.size());

    double automaticMaximum = 0.0;

    for (const TH1D* histogram : histograms) {
        automaticMaximum =
            std::max(
                automaticMaximum,
                histogram->GetMaximum()
            );
    }

    for (std::size_t i = 0;
         i < histograms.size();
         ++i) {

        TH1D* histogram =
            histograms[i];

        setHistogramStyle(
            histogram,
            datasets[i]
        );

        if (i == 0) {
            histogram->SetMinimum(0.0);

            if (useFixedMaximum) {
                histogram->SetMaximum(
                    fixedMaximum
                );
            } else {
                histogram->SetMaximum(
                    automaticMaximum > 0.0
                        ? 1.15 * automaticMaximum
                        : 1.0
                );
            }
        }

        const char* drawOption =
            (i == 0)
                ? "HIST"
                : "HIST SAME";

        drawnHistograms.push_back(
            histogram->DrawCopy(drawOption)
        );
    }

    auto legend =
        makeLegend(
            drawnHistograms,
            datasets
        );

    legend->Draw();
    canvas->Update();

    return canvas;
}

} // namespace


void CompareAveragePhotonDistribution()
{
    /*
    ファイル名は仮のもの。

    実際に使用するROOTファイル名へ置き換えること。
    */
    const std::vector<DatasetConfig> datasets = {
        {
            "/home/hashizume/Geant4-project/triumf2023_test/"
            "root/run_20260809_172459_run000.root",

            "gamma, Edep = 2.0 MeV",

            1.950,
            2.050,

            kRed + 1,
            1
        },
        {
            "/home/hashizume/Geant4-project/triumf2023_test/"
            "root/run_20260809_221026_run000.root",

            "gamma, Edep = 0.5 MeV",

            0.450,
            0.550,

            kOrange + 7,
            2
        },
        {
            "/home/hashizume/Geant4-project/triumf2023_test/"
            "root/run_20260809_164042_run000.root",

            "neutron, Edep = 2.0 MeV",

            1.950,
            2.050,

            kBlue + 1,
            7
        },
        {
            "/home/hashizume/Geant4-project/triumf2023_test/"
            "root/run_20260809_164615_run000.root",

            "neutron, Edep = 0.5 MeV",

            0.450,
            0.550,

            kGreen + 2,
            9
        }
    };

    std::vector<AverageHistograms> results;
    results.reserve(datasets.size());

    for (std::size_t i = 0;
         i < datasets.size();
         ++i) {

        const TString histogramBaseName =
            TString::Format(
                "averagePhotonDistribution_%zu",
                i
            );

        results.push_back(
            makeAverageHistograms(
                datasets[i],
                histogramBaseName
            )
        );

        std::cout
            << datasets[i].label.Data()
            << ": selected events = "
            << results.back().numberOfEvents
            << " / maximum "
            << kMaximumEvents
            << ", mean fraction inside "
            << kTimeMinimum
            << " - "
            << kTimeMaximum
            << " ns = "
            << results.back().visibleFraction
            << std::endl;
    }

    gStyle->SetOptStat(0);

    std::vector<TH1D*> meanPhotonCounts;
    std::vector<TH1D*> meanNormalizedShapes;
    std::vector<TH1D*> cumulativeShapes;

    meanPhotonCounts.reserve(results.size());
    meanNormalizedShapes.reserve(results.size());
    cumulativeShapes.reserve(results.size());

    for (auto& result : results) {
        meanPhotonCounts.push_back(
            result.meanPhotonCounts.get()
        );

        meanNormalizedShapes.push_back(
            result.meanNormalizedShape.get()
        );

        cumulativeShapes.push_back(
            result.cumulativeShape.get()
        );
    }

    /*
    1. 1イベント当たりの平均到達光子数。

    Y軸上限はデータから自動的に決める。
    */
    drawComparison(
        "canvasAverageMeanCounts",
        meanPhotonCounts,
        datasets,
        false,
        0.0
    );

    /*
    2. イベントごとに正規化してから平均した分布。

    Y軸上限はデータから自動的に決める。
    */
    drawComparison(
        "canvasAverageNormalizedShape",
        meanNormalizedShapes,
        datasets,
        false,
        0.0
    );

    /*
    3. 正規化済み累積分布。

    最終値は1で、表示上限を1.05にする。
    */
    drawComparison(
        "canvasAverageCumulative",
        cumulativeShapes,
        datasets,
        true,
        1.05
    );
}