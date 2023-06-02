#include <iostream>
#include <numeric>
#include <vector>
#include "TROOT.h"
#include "TF1.h"
#include "TGraph.h"
#include "ROOT/RDataFrame.hxx"

// Number of workers
const UInt_t kNWorkers = 10U;

// Function to return average value of the vector
template <typename T>
double getAverage(std::vector<T> const &v)
{
    if (v.empty())
    {
        return 0;
    }
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// Main function
int main(int argc, char **argv)
{
    // Enable multithreading
    ROOT::EnableImplicitMT(kNWorkers);
    // Create RDataFrame from a tree, "tree" in the argv[1] file.
    ROOT::RDataFrame d("output", argv[1]);

    // Function to fit
    const auto fit = [](const std::vector<double> &input, const double &pmax, const std::vector<double> &id)
    {
        std::vector<double> param;
        if (input.empty())
            return param;
        TF1 fit("fit", "[2]*(1+TMath::Erf((x-[0])/[1]))*([3]*exp(-(x-[0])/57.72)+(1.0-[3])*exp(-(x-[0])/447.33))", 0, 500);
        fit.SetParameters(30, 1, pmax, 0.8);
        // fit.SetParLimits(0, 20, 40);
        // fit.SetParLimits(1, 0.5, 4);
        // fit.SetParLimits(2, 0, 20000);
        // fit.SetParLimits(3, 0, 1);
        TGraph gr(input.size(), id.data(), input.data());
        gr.Fit(&fit, "Q", "", 0, 500);
        param.emplace_back(fit.GetParameter(0));
        param.emplace_back(fit.GetParameter(1));
        param.emplace_back(fit.GetParameter(2));
        param.emplace_back(fit.GetParameter(3));
        param.emplace_back(fit.GetParError(0));
        param.emplace_back(fit.GetParError(1));
        param.emplace_back(fit.GetParError(2));
        param.emplace_back(fit.GetParError(3));
        param.emplace_back(fit.GetChisquare());
        return param;
    };

    auto output = d.Define("fit_params", fit, {"wave", "pmax", "sampleId"});
    output.Snapshot("fit_output", "fit_output.root", {
                                                         "fit_params",
                                                         "wave",
                                                     });

    return 0;
}