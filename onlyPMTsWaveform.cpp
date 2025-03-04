//This code gives all PMTs' waveform and creates a combined canvas based on channel mapping. It also accepts eventID from the terminal.
// It displays BM  and Area on the plot.
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

    // Mapping of PMT channels
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1};

    // Define the layout of PMT channels on the canvas
    int layout[4][3] = {
        {9, 3, 7},  // Row 1: PMT 10, PMT 4, PMT 8
        {5, 4, 8},  // Row 2: PMT 6, PMT 5, PMT 9
        {0, 6, 1},  // Row 3: PMT 1, PMT 7, PMT 2
        {10, 11, 2} // Row 4: PMT 11, PMT 12, PMT 3
    };

    // Create a master canvas for the combined plot
    TCanvas *masterCanvas = new TCanvas("MasterCanvas", "Combined PMT Waveforms", 3600, 3000);
    masterCanvas->Divide(3, 4, 0.007, 0.009); // Divide canvas into 3x4 pads with minimal spacing

    // Adjust margins to create space in the bottom-left corner
    masterCanvas->SetLeftMargin(0.10);  // Increase left margin
    masterCanvas->SetRightMargin(0.05);
    masterCanvas->SetTopMargin(0.05);
    masterCanvas->SetBottomMargin(0.10); // Increase bottom margin

    // Loop through the layout to create individual PMT plots
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int padPosition = row * 3 + col + 1; // Calculate pad position (1-12)
            masterCanvas->cd(padPosition); // Switch to the specific pad

            // Reduce margins for individual pads
            gPad->SetLeftMargin(0.15);  // Increase left margin for individual pads
            gPad->SetRightMargin(0.05);
            gPad->SetTopMargin(0.05);
            gPad->SetBottomMargin(0.15); // Increase bottom margin for individual pads

            // Get the PMT channel index from the layout
            int pmtIndex = layout[row][col];
            int adcIndex = pmtChannelMap[pmtIndex]; // Map PMT channels

            // Create a TGraph to plot the ADC values
            TGraph *graph = new TGraph();

            // Fill the TGraph with ADC values
            for (int k = 0; k < 45; k++) {
                double time = (k + 1) * 16.0;
                if (time > 720) break;
                double adcValue = adcVal[adcIndex][k];
                graph->SetPoint(k, time, adcValue);
            }
            graph->SetLineWidth(3);     // Set line width to 3 for thicker lines
            graph->SetLineColor(kBlack); // Set line color to black

            // Set the title for the plot
            TString title = Form("PMT %d", pmtIndex + 1);
            graph->SetTitle("");
            graph->GetXaxis()->SetTitle("Time (ns)");
            graph->GetYaxis()->SetTitle("ADC Value (mV)");

            // Set the axis title sizes
            graph->GetXaxis()->SetTitleSize(0.07); // Set x-axis title size
            graph->GetYaxis()->SetTitleSize(0.07); // Set y-axis title size

            // Set the axis title offsets
            graph->GetXaxis()->SetTitleOffset(1.2); // Increase x-axis title offset
            graph->GetYaxis()->SetTitleOffset(1.0); // Set y-axis title offset

            graph->SetMinimum(170);   // Set y-axis starting value
            graph->SetMaximum(maxADC); // Set y-axis maximum based on max ADC value
            graph->GetXaxis()->SetRangeUser(0, 720);
            graph->Draw("AL"); // Draw the graph

            // Add the title to the plot
            TLatex *latexTitle = new TLatex();
            latexTitle->SetTextSize(0.12);
            latexTitle->SetTextAlign(22);
            latexTitle->SetNDC(true);
            latexTitle->DrawLatex(0.5, 0.94, title);

            // Add area and baselineMean information with colored text
            TLatex *infoArea = new TLatex();
            infoArea->SetTextSize(0.04);
            infoArea->SetTextAlign(13);
            infoArea->SetNDC(true);
            infoArea->SetTextColor(kBlue); // Blue color for Area
            infoArea->DrawLatex(0.2, 0.85, Form("Area: %.2f", area[adcIndex]));

            TLatex *infoBaseline = new TLatex();
            infoBaseline->SetTextSize(0.04);
            infoBaseline->SetTextAlign(13);
            infoBaseline->SetNDC(true);
            infoBaseline->SetTextColor(kRed); // Red color for Baseline Mean
            infoBaseline->DrawLatex(0.2, 0.80, Form("Baseline Mean: %.2f", baselineMean[adcIndex]));
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

        graph->SetTitle(Form("PMT %d", i + 1));
        graph->GetXaxis()->SetTitle("Time (ns)");
        graph->GetYaxis()->SetTitle("ADC Value(mV)");

        // Set the axis title sizes for individual plots
        graph->GetXaxis()->SetTitleSize(0.04); // Set x-axis title size
        graph->GetYaxis()->SetTitleSize(0.04); // Set y-axis title size

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
        infoArea->DrawLatex(0.2, 0.85, Form("Area: %.2f", area[adcIndex]));

        TLatex *infoBaseline = new TLatex();
        infoBaseline->SetTextSize(0.04);
        infoBaseline->SetTextAlign(13);
        infoBaseline->SetNDC(true);
        infoBaseline->SetTextColor(kRed); // Red color for Baseline Mean
        infoBaseline->DrawLatex(0.2, 0.80, Form("Baseline Mean: %.2f", baselineMean[adcIndex]));

        individualCanvas->SaveAs(individualPMTFileName);
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
