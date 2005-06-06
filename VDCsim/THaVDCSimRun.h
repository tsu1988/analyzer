#ifndef THaVDCSimRun_
#define THaVDCSimRun_

#include "THaRunBase.h"
#include "THaVDCSim.h"

class TFile;
class TTree;
class TBranch;

class THaVDCSimRun : public THaRunBase {
 public:
  THaVDCSimRun(const char* description = "")
    : THaRunBase(description), nentries(0), entry(0), event(0) {}
  THaVDCSimRun(const THaVDCSimRun &run);
  virtual ~THaVDCSimRun();
  virtual THaVDCSimRun &operator=(const THaVDCSimRun &rhs);

  Int_t Close();
  Int_t Open();
  const Int_t* GetEvBuffer() const;
  Int_t ReadEvent();
  Int_t Init();

 protected:
  Int_t ReadDatabase() {return 0;}

  TFile *rootFile;
  TTree *tree;
  TBranch *branch;
  THaVDCSimEvent *event;

  Int_t nentries;
  Int_t entry;

  ClassDef(THaVDCSimRun, 0) // Run class for simulated VDC data
};

#endif
