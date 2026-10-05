#include "RunAction.hh"

#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

RunAction::RunAction()
{
  auto* accManager = G4AccumulableManager::Instance();
  accManager->Register(fSumEdep);
  accManager->Register(fSumProduced);
  accManager->Register(fSumDetected);
  accManager->Register(fSumDetected2);

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetFileName("scintbar");
  analysisManager->SetNtupleMerging(true);
  analysisManager->SetVerboseLevel(1);

  // One row per event
  analysisManager->CreateNtuple("photons", "Muon in scintillator bar");
  analysisManager->CreateNtupleDColumn("edep_MeV");
  analysisManager->CreateNtupleIColumn("nScint");
  analysisManager->CreateNtupleIColumn("nCerenkov");
  analysisManager->CreateNtupleIColumn("nDetLeft");
  analysisManager->CreateNtupleIColumn("nDetRight");
  analysisManager->CreateNtupleDColumn("xGun_mm");
  analysisManager->FinishNtuple();
}

void RunAction::BeginOfRunAction(const G4Run*)
{
  G4AccumulableManager::Instance()->Reset();
  G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::AddEvent(G4double edep, G4int nProduced, G4int nDetected)
{
  fSumEdep += edep;
  fSumProduced += nProduced;
  fSumDetected += nDetected;
  fSumDetected2 += static_cast<G4double>(nDetected) * nDetected;
}

void RunAction::EndOfRunAction(const G4Run* run)
{
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();

  G4AccumulableManager::Instance()->Merge();

  const G4int nEvents = run->GetNumberOfEvent();
  if (nEvents == 0) {
    return;
  }

  if (IsMaster()) {
    const G4double meanEdep = fSumEdep.GetValue() / nEvents;
    const G4double meanProduced = fSumProduced.GetValue() / nEvents;
    const G4double meanDetected = fSumDetected.GetValue() / nEvents;
    G4double variance = fSumDetected2.GetValue() / nEvents - meanDetected * meanDetected;
    if (variance < 0.) {
      variance = 0.;
    }

    G4cout << G4endl
           << "================ Run summary ================" << G4endl
           << " Events                     : " << nEvents << G4endl
           << " Mean Edep in bar           : " << meanEdep / MeV << " MeV" << G4endl
           << " Mean photons produced      : " << meanProduced << G4endl
           << " Mean photons detected (L+R): " << meanDetected
           << " +- " << std::sqrt(variance) << " (RMS)" << G4endl
           << "=============================================" << G4endl;
  }
}
