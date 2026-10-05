#include "EventAction.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

EventAction::EventAction(RunAction* runAction)
  : fRunAction(runAction)
{
}

void EventAction::BeginOfEventAction(const G4Event*)
{
  fEdep = 0.;
  fNScint = 0;
  fNCerenkov = 0;
  fNDetectedLeft = 0;
  fNDetectedRight = 0;
}

void EventAction::AddEdep(G4double edep)
{
  fEdep += edep;
}

void EventAction::AddScintillationPhoton()
{
  fNScint++;
}

void EventAction::AddCerenkovPhoton()
{
  fNCerenkov++;
}

void EventAction::AddDetectedPhoton(G4int sensorId)
{
  if (sensorId == 0) {
    fNDetectedLeft++;
  }
  else {
    fNDetectedRight++;
  }
}

void EventAction::EndOfEventAction(const G4Event* event)
{
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillNtupleDColumn(0, fEdep / MeV);
  analysisManager->FillNtupleIColumn(1, fNScint);
  analysisManager->FillNtupleIColumn(2, fNCerenkov);
  analysisManager->FillNtupleIColumn(3, fNDetectedLeft);
  analysisManager->FillNtupleIColumn(4, fNDetectedRight);

  // x position of the muon gun (along the bar), to identify the scan point
  G4double xGun = 0.;
  const G4PrimaryVertex* vertex = event->GetPrimaryVertex();
  if (vertex != nullptr) {
    xGun = vertex->GetX0();
  }
  analysisManager->FillNtupleDColumn(5, xGun / mm);
  analysisManager->AddNtupleRow();

  const G4int nProduced = fNScint + fNCerenkov;
  const G4int nDetected = fNDetectedLeft + fNDetectedRight;
  fRunAction->AddEvent(fEdep, nProduced, nDetected);

  const G4int eventId = event->GetEventID();
  if (eventId < 10 || eventId % 100 == 0) {
    G4cout << "Event " << eventId
           << " | Edep = " << fEdep / MeV << " MeV"
           << " | produced: scint = " << fNScint << ", Cherenkov = " << fNCerenkov
           << " | detected: left = " << fNDetectedLeft << ", right = " << fNDetectedRight
           << G4endl;
  }
}
