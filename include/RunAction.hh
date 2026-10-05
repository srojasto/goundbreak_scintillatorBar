#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4AccValue.hh"
#include "globals.hh"

class G4Run;

class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

    void AddEvent(G4double edep, G4int nProduced, G4int nDetected);

  private:
    G4AccValue<G4double> fSumEdep = 0.;
    G4AccValue<G4double> fSumProduced = 0.;
    G4AccValue<G4double> fSumDetected = 0.;
    G4AccValue<G4double> fSumDetected2 = 0.;
};

#endif
