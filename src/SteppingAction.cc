#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4OpticalPhoton.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "Randomize.hh"

namespace
{
  // Photon detection efficiency of the SiPM (wavelength-independent here).
  // Set to 1.0 to count every photon reaching the sensor.
  const G4double kPDE = 0.40;
}

SteppingAction::SteppingAction(EventAction* eventAction)
  : fEventAction(eventAction)
{
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  G4Track* track = step->GetTrack();
  const G4StepPoint* preStepPoint = step->GetPreStepPoint();
  const G4String& volumeName = preStepPoint->GetPhysicalVolume()->GetName();

  const bool isOpticalPhoton = (track->GetDefinition() == G4OpticalPhoton::Definition());

  // --- Optical photon that has entered a SiPM: count (with PDE) and kill ---
  if (isOpticalPhoton) {
    if (volumeName == "Sensor") {
      const G4int sensorId = preStepPoint->GetTouchableHandle()->GetCopyNumber();
      if (G4UniformRand() < kPDE) {
        fEventAction->AddDetectedPhoton(sensorId);
      }
      track->SetTrackStatus(fStopAndKill);
    }
    return;
  }

  // --- Charged particles in the bar: energy deposit and photon production ---
  if (volumeName == "Bar") {
    fEventAction->AddEdep(step->GetTotalEnergyDeposit());

    const std::vector<const G4Track*>* secondaries = step->GetSecondaryInCurrentStep();
    for (const G4Track* secondary : *secondaries) {
      if (secondary->GetDefinition() != G4OpticalPhoton::Definition()) {
        continue;
      }
      const G4String& creator = secondary->GetCreatorProcess()->GetProcessName();
      if (creator == "Scintillation") {
        fEventAction->AddScintillationPhoton();
      }
      else if (creator == "Cerenkov") {
        fEventAction->AddCerenkovPhoton();
      }
    }
  }
}
