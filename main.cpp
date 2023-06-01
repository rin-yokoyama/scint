#include <iostream>
#include <numeric>
#include <vector>
#include "TROOT.h"
#include "TGraph.h"
#include "TFile.h"
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
    // Create RDataFrame from a tree, "exampleTree" in the "example.root" file.
    ROOT::RDataFrame d("tree", "co60_230530.root");

    // Function defining cuts
    const auto threshold = [](const std::vector<double> &input)
    {
        const std::vector<double> sub = {input.begin() + 50, input.begin() + 70};
        bool flag = true;
        for (const auto &sample : sub)
        {
            flag = flag && sample > 100;
        }
        return flag;
    };

    // Function to define wave form (baseline subtracted)
    const auto waveForm = [](const std::vector<float> &input)
    {
        static const int prePulseRange = 20;
        // Calculate baseline
        const std::vector<float> pre_pulse = {input.begin(), input.begin() + prePulseRange};
        const auto baseline = getAverage(pre_pulse);

        std::vector<double> wave;
        for (const auto &in : input)
        {
            wave.emplace_back(baseline - in);
        }
        return wave;
    };

    // Function to define sample id
    const auto sampleId = [](const std::vector<double> &input)
    {
        std::vector<int> id(input.size());
        std::iota(id.begin(), id.end(), 1);
        return id;
    };

    const auto traceMax = [](const std::vector<double> &input) -> double
    {
        return *std::max_element(input.begin(), input.end());
    };

    auto filtered = d.Define("wave", waveForm, {"pulse"}).Filter(threshold, {"wave"});
    auto output = filtered.Define("pmax", traceMax, {"wave"});
    // auto output = filtered.Define("pmax", traceMax, {"wave"}).Define("sampleId", sampleId, {"wave"});
    output.Snapshot("output", "output.root", {
                                                 "wave",
                                                 "pmax",
                                             });
    auto h = output.Histo1D({"hist", "hist", 1000, 0, 10000}, "pmax");
    // auto h2 = output.Histo2D({"hist", "hist", 500, 0, 500, 1000, 0, 5000}, "sampleId", "wave");

    TFile f("output.root", "update");
    h->Write();
    // h2->Write();
    f.Close();

    return 0;
}