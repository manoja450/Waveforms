//This code gives the plots of waveforms and creates a combined canvas according to the physical location of the PMTS/SiPMs.
//( EventID is  specified, so it gives a plot of the specific event).It also creates a legend on Combined canvas .
// It also prints baselineMean and area on each canvas and combined canvas. 

#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TAxis.h>
#include <TH1F.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include "TLatex.h"

using namespace std;

// Function to round up a value to the nearest bin size
double roundUpToBin(double value, double binSize) {
    return ceil((value + 0.5) / binSize) * binSize;
}

// Main function to process the ROOT file and generate plots
void lowlight(const char *fileName, int EventID) {
    // Open the ROOT file
    TFile *file = TFile::Open(fileName);
    if (!file || file->IsZombie()) {
        cerr << "Error opening file: " << fileName << endl;
        return;
    }

    // Access the TTree named "tree" from the ROOT file
    TTree *tree = (TTree*)file->Get("tree");
    if (!tree) {
        cerr << "Error accessing TTree 'tree'!" << endl;
        file->Close();
        return;
    }

    // Declare variables to store data from the TTree
    Short_t adcVal[23][45]; // ADC values for 23 channels and 45 time bins
    Double_t area[23];      // Area for each channel
    Double_t baselineMean[23]; // Baseline mean for each channel

    // Set branch addresses to read data from the TTree
    tree->SetBranchAddress("adcVal", adcVal);
    tree->SetBranchAddress("area", area);
    tree->SetBranchAddress("baselineMean", baselineMean);

    // Get the total number of entries in the TTree
    Long64_t nEntries = tree->GetEntries();
    if (EventID < 0 || EventID >= nEntries) {
        cerr << "Error: EventID " << EventID << " is out of range (0-" << nEntries-1 << ")" << endl;
        file->Close();
        return;
    }

    // Load the specified event into memory
    tree->GetEntry(EventID);

    // Find the maximum ADC value across all channels and time bins for this event
    double maxADC = 0;
    for (int i = 0; i < 23; i++) {
        for (int k = 0; k < 45; k++) {
            if (adcVal[i][k] > maxADC) {
                maxADC = adcVal[i][k];
            }
        }
    }

    // Round up the maximum ADC value to the nearest 100 for better y-axis scaling
    maxADC = roundUpToBin(maxADC, 10);

    // Mapping of PMT and SiPM channels
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1};
    int sipmChannelMap[10] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

    // Create a master canvas for the combined plot
    TCanvas *masterCanvas = new TCanvas("MasterCanvas", "Combined PMT and SiPM Waveforms", 3600, 3000);
    masterCanvas->Divide(5, 6, 0.002, 0.002); // Divide canvas into 5x6 pads with minimal spacing

    // Reduce margins for the master canvas
    masterCanvas->SetLeftMargin(0.02);
    masterCanvas->SetRightMargin(0.02);
    masterCanvas->SetTopMargin(0.02);
    masterCanvas->SetBottomMargin(0.02);

    // Define the layout of PMT and SiPM channels on the canvas
    int layout[6][5] = {
        {-1,  -1,  20,  21, -1},
        {16,  9,   3,   7,  12},
        {15,  5,   4,   8,   -1},
        {19, 0,   6,  1,  17},
        {-1,  10,  11,   2,  13},
        {-1, 14,   18,  -1, -1}
    };

    // Add global labels to the master canvas
    masterCanvas->cd(0);
    TLatex *textbox = new TLatex();
    textbox->SetTextSize(0.02);
    textbox->SetTextAlign(13);
    textbox->SetNDC(true);
    textbox->DrawLatex(0.01, 0.10, "X axis: Time (0-720) ns");
    textbox->DrawLatex(0.01, 0.08, "Y axis: ADC values(mV)");

    // Loop through the layout to create individual plots
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 5; col++) {
            int padPosition = layout[row][col];
            if (padPosition >= 0) {
                masterCanvas->cd(row * 5 + col + 1);

                // Reduce margins for individual pads
                gPad->SetLeftMargin(0.05);
                gPad->SetRightMargin(0.05);
                gPad->SetTopMargin(0.05);
                gPad->SetBottomMargin(0.05);

                // Create a TGraph to plot the ADC values
                TGraph *graph = new TGraph();

                int adcIndex = -1;
                if (padPosition < 12) {
                    adcIndex = pmtChannelMap[padPosition]; // Map PMT channels
                } else {
                    adcIndex = sipmChannelMap[padPosition - 12]; // Map SiPM channels
                }

                // Fill the TGraph with ADC values
                for (int k = 0; k < 45; k++) {
                    double time = (k + 1) * 16.0;
                    if (time > 720) break;
                    double adcValue = adcVal[adcIndex][k];
                    graph->SetPoint(k, time, adcValue);
                }

                // Set the title for the plot
                TString title;
                if (padPosition < 12) {
                    title = Form("PMT %d", padPosition + 1);
                } else {
                    title = Form("SiPM %d", padPosition - 11);
                }
                graph->SetLineWidth(3);     // Set line width to 3 for thicker lines
                graph->SetLineColor(kBlack); // Set line color to black
                graph->SetTitle("");
                graph->GetXaxis()->SetTitle("Time (ns)");
                graph->GetYaxis()->SetTitle("ADC Value (mV)");
                graph->SetMinimum(170);   // Set y-axis starting value
                graph->SetMaximum(maxADC); // Set y-axis maximum based on max ADC value
                graph->GetXaxis()->SetRangeUser(0, 720);
                graph->Draw("AL"); // Draw the graph

                // Add the title to the plot
                TLatex *latexTitle = new TLatex();
                latexTitle->SetTextSize(0.14);
                latexTitle->SetTextAlign(22);
                latexTitle->SetNDC(true);
                latexTitle->DrawLatex(0.5, 0.94, title);

                // Add area and baselineMean information with colored text
                TLatex *infoArea = new TLatex();
                infoArea->SetTextSize(0.08);
                infoArea->SetTextAlign(13);
                infoArea->SetNDC(true);
                infoArea->SetTextColor(kBlue); // Blue color for Area
                infoArea->DrawLatex(0.08, 0.90, Form("Area: %.2f", area[adcIndex]));

                TLatex *infoBaseline = new TLatex();
                infoBaseline->SetTextSize(0.08);
                infoBaseline->SetTextAlign(13);
                infoBaseline->SetNDC(true);
                infoBaseline->SetTextColor(kRed); // Red color for Baseline Mean
                infoBaseline->DrawLatex(0.08, 0.85, Form("BM: %.2f", baselineMean[adcIndex]));
            }
        }
    }

    // Save the combined canvas as a PNG file
    TString combinedChartFileName = Form("/root/gears/new/CombinedChart_SpecificLayout_%s_Event%d.png", fileName, EventID);
    masterCanvas->SaveAs(combinedChartFileName);
    cout << "Combined chart saved as " << combinedChartFileName << endl;

    // Save individual PMT plots
    for (int i = 0; i < 12; i++) {
        TString individualPMTFileName = Form("/root/gears/new/PMT%d_%s_Event%d.png", i + 1, fileName, EventID);
        TCanvas *individualCanvas = new TCanvas(Form("PMT%d_Canvas", i + 1), Form("PMT %d", i + 1), 800, 600);
        TGraph *graph = new TGraph();

        int adcIndex = pmtChannelMap[i];
        for (int k = 0; k < 45; k++) {
            double time = (k + 1) * 16.0;
            double adcValue = adcVal[adcIndex][k];
            graph->SetPoint(k, time, adcValue);
        }
        graph->SetLineWidth(3);     // Set line width to 3 for thicker lines
        graph->SetLineColor(kBlack); // Set line color to black
        graph->SetTitle(Form("PMT %d", i + 1));
        graph->GetXaxis()->SetTitle("Time (ns)");
        graph->GetYaxis()->SetTitle("ADC Value(mV)");
        graph->SetMinimum(170); // y axis starting for PMTS
        graph->SetMaximum(maxADC); // Set y-axis maximum based on max ADC value
        graph->GetXaxis()->SetRangeUser(0, 720);
        graph->Draw("AL");

        // Add area and baselineMean information with colored text
        TLatex *infoArea = new TLatex();
        infoArea->SetTextSize(0.04);
        infoArea->SetTextAlign(13);
        infoArea->SetNDC(true);
        infoArea->SetTextColor(kBlue); // Blue color for Area
        infoArea->DrawLatex(0.14, 0.90, Form("Area: %.2f", area[adcIndex]));

        TLatex *infoBaseline = new TLatex();
        infoBaseline->SetTextSize(0.04);
        infoBaseline->SetTextAlign(13);
        infoBaseline->SetNDC(true);
        infoBaseline->SetTextColor(kRed); // Red color for Baseline Mean
        infoBaseline->DrawLatex(0.14, 0.85, Form("BM: %.2f", baselineMean[adcIndex]));

        individualCanvas->SaveAs(individualPMTFileName);
        delete individualCanvas;
    }

    // Save individual SiPM plots
    for (int i = 0; i < 10; i++) {
        TString individualSiPMFileName = Form("/root/gears/new/SiPM%d_%s_Event%d.png", i + 1, fileName, EventID);
        TCanvas *individualCanvas = new TCanvas(Form("SiPM%d_Canvas", i + 1), Form("SiPM %d", i + 1), 800, 600);
        TGraph *graph = new TGraph();

        int adcIndex = sipmChannelMap[i];
        for (int k = 0; k < 45; k++) {
            double time = (k + 1) * 16.0;
            double adcValue = adcVal[adcIndex][k];
            graph->SetPoint(k, time, adcValue);
        }
        graph->SetLineWidth(3);     // Set line width to 3 for thicker lines
        graph->SetLineColor(kBlack); // Set line color to black
        graph->SetTitle(Form("SiPM %d", i + 1));
        graph->GetXaxis()->SetTitle("Time (ns)");
        graph->GetYaxis()->SetTitle("ADC Value");
        graph->SetMinimum(170); // y axis starting SiPMs
        graph->SetMaximum(maxADC); // Set y-axis maximum based on max ADC value
        graph->GetXaxis()->SetRangeUser(0, 720);
        graph->Draw("AL");

        // Add area and baselineMean information with colored text
        TLatex *infoArea = new TLatex();
        infoArea->SetTextSize(0.04);
        infoArea->SetTextAlign(13);
        infoArea->SetNDC(true);
        infoArea->SetTextColor(kBlue); // Blue color for Area
        infoArea->DrawLatex(0.14, 0.90, Form("Area: %.2f", area[adcIndex]));

        TLatex *infoBaseline = new TLatex();
        infoBaseline->SetTextSize(0.04);
        infoBaseline->SetTextAlign(13);
        infoBaseline->SetNDC(true);
        infoBaseline->SetTextColor(kRed); // Red color for Baseline Mean
        infoBaseline->DrawLatex(0.14, 0.85, Form("BM: %.2f", baselineMean[adcIndex]));

        individualCanvas->SaveAs(individualSiPMFileName);
        delete individualCanvas;
    }

    // Close the ROOT file
    file->Close();
}

// Main function to handle command-line arguments
int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <root_file> <EventID>" << endl;
        return 1;
    }

    const char* fileName = argv[1];
    int EventID = atoi(argv[2]);
    lowlight(fileName, EventID);

    return 0;
}
