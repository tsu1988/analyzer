//_____________________________________________________________________________
//
// THaBenchmark utility class
//
// Provides start/stop mode for ROOT's TBenchmark class
//_____________________________________________________________________________

#include "TBenchmark.h"

//_____________________________________________________________________________
class THaBenchmark : public TBenchmark {
public:
  THaBenchmark() { fNmax = 50;  }
  virtual ~THaBenchmark() {}

  virtual void Begin(const char *name) {
    if (!fNbench)
      TBenchmark::Start(name);
    else {
      Int_t bench = GetBench(name);
      if (bench < 0 && fNbench < fNmax ) 
	TBenchmark::Start(name);
      else if (bench>=0) 
	fTimer[bench].Start(kFALSE);
      else
	Warning("Start","too many benches");
    }
  }

  virtual void Print(Option_t *name="") const {
    if (name && name[0]!='\0') TBenchmark::Print(name);
    else {
      for (int i=0; i<fNbench; i++) {
	TBenchmark::Print( (fNames[i]).Data() );
      }
    }
  }
  
};
