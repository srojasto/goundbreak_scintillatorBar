#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"
#include "FTFP_BERT.hh"

int main(int argc, char** argv)
{
  G4UIExecutive* ui = nullptr;
  if (argc == 1) {
    ui = new G4UIExecutive(argc, argv);
  }

  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  runManager->SetUserInitialization(new DetectorConstruction());

  // Standard hadronic/EM physics plus optical processes
  // (scintillation, Cherenkov, absorption, boundary, Rayleigh, WLS)
  auto* physicsList = new FTFP_BERT;
  physicsList->RegisterPhysics(new G4OpticalPhysics());
  runManager->SetUserInitialization(physicsList);

  auto* opticalParams = G4OpticalParameters::Instance();
  opticalParams->SetScintTrackSecondariesFirst(true);
  opticalParams->SetCerenkovTrackSecondariesFirst(true);
  opticalParams->SetCerenkovMaxPhotonsPerStep(100);

  runManager->SetUserInitialization(new ActionInitialization());

  auto* visManager = new G4VisExecutive;
  visManager->Initialize();

  auto* uiManager = G4UImanager::GetUIpointer();
  if (ui == nullptr) {
    G4String fileName = argv[1];
    uiManager->ApplyCommand("/control/execute " + fileName);
  }
  else {
    uiManager->ApplyCommand("/control/execute vis.mac");
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}
