#ifndef THaVDCSimRun_
#define THaVDCSimRun_

#include "THaRunBase.h"
#include "THaVDCSim.h"
#include "TString.h"

class TFile;
class TTree;
class TBranch;

class THaVDCSimRun : public THaRunBase {
 public:
  THaVDCSimRun(const char* filename = "", const char* description = "");
  THaVDCSimRun(const THaVDCSimRun &run);
  virtual ~THaVDCSimRun();
  virtual THaVDCSimRun &operator=(const THaRunBase &rhs);

  Int_t Close();
  Int_t Open();
  const Int_t* GetEvBuffer() const;
  Int_t ReadEvent();
  Int_t Init();
  const char* GetFileName() const { return rootFileName.Data(); }
  void SetFileName( const char* name ) { rootFileName = name; }

 protected:
  virtual Int_t ReadDatabase() {return 0;}

  TString rootFileName;
  TFile *rootFile;
  TTree *tree;
  TBranch *branch;
  THaVDCSimEvent *event;

  Int_t nentries;
  Int_t entry;

  ClassDef(THaVDCSimRun, 0) // Run class for simulated VDC data
};

#endif
