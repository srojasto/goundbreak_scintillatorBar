#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class G4Event;
class RunAction;

class EventAction : public G4UserEventAction
{
  public:
    explicit EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void AddEdep(G4double edep);
    void AddScintillationPhoton();
    void AddCerenkovPhoton();
    void AddDetectedPhoton(G4int sensorId);

  private:
    RunAction* fRunAction = nullptr;
    G4double fEdep = 0.;
    G4int fNScint = 0;
    G4int fNCerenkov = 0;
    G4int fNDetectedLeft = 0;
    G4int fNDetectedRight = 0;
};

#endif
