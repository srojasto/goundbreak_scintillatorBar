// Mean detected photons (left SiPM) vs distance from the SiPM-side end.
// Usage:  root -l 'plotScan.C("scan_all.root", 48.)'
//   second argument = bar half-length in mm (kBarLength / 2)

void plotScan(const char* fileName = "scan_all.root", double barHalfMm = 48.)
{
  TFile* file = TFile::Open(fileName);
  if (file == nullptr || file->IsZombie()) {
    printf("Cannot open %s\n", fileName);
    return;
  }

  TTree* tree = nullptr;
  file->GetObject("photons", tree);
  if (tree == nullptr) {
    printf("Tree 'photons' not found in %s\n", fileName);
    return;
  }

  // Distance from the left (SiPM-side) end in cm, one bin per scan point
  TString expression = TString::Format("nDetLeft:(xGun_mm+%f)/10.>>prof(10,0.5,10.5)", barHalfMm);
  tree->Draw(expression, "", "prof goff");

  TProfile* prof = nullptr;
  gDirectory->GetObject("prof", prof);
  if (prof == nullptr) {
    printf("Could not build the profile\n");
    return;
  }

  // Error bars = error on the mean (default TProfile option)
  TGraphErrors* graph = new TGraphErrors();
  int point = 0;
  for (int bin = 1; bin <= prof->GetNbinsX(); ++bin) {
    if (prof->GetBinEntries(bin) > 0) {
      graph->SetPoint(point, prof->GetBinCenter(bin), prof->GetBinContent(bin));
      graph->SetPointError(point, 0., prof->GetBinError(bin));
      printf("d = %4.1f cm : <nDetLeft> = %8.2f +- %6.2f  (%d events)\n",
             prof->GetBinCenter(bin), prof->GetBinContent(bin), prof->GetBinError(bin),
             static_cast<int>(prof->GetBinEntries(bin)));
      point++;
    }
  }

  TCanvas* canvas = new TCanvas("cScan", "Position scan", 800, 600);
  canvas->cd();
  graph->SetTitle("Position scan;distance from SiPM-side end [cm];mean detected photons (left SiPM)");
  graph->SetMarkerStyle(20);
  graph->Draw("AP");
  canvas->SaveAs("scan_detected_vs_distance.pdf");
}
