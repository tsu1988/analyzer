#include "THaVDCSimRun.h"
#include "THaVDCSim.h"

#include "TFile.h"
#include "TTree.h"

THaVDCSimRun::THaVDCSimRun(const THaVDCSimRun &run)
  : THaRunBase(run), nentries(0), entry(0)
{
  rootFile = NULL;
  tree = NULL;
  event = NULL;
}

THaVDCSimRun &THaVDCSimRun::operator=(const THaVDCSimRun &rhs)
{
  THaRunBase::operator=(rhs);
  rootFile = NULL;
  tree = NULL;
  event = NULL;

  return *this;
}

Int_t THaVDCSimRun::Init()
{
  // Use the date we're familiar with, so we can use those channel mappings
  fDate.Set(2003,4,15,12,0,0);
  fAssumeDate = kTRUE;
  fDataSet |= kDate;

  return THaRunBase::Init();
}

Int_t THaVDCSimRun::Open()
{
  rootFile = new TFile("vdctracks.root", "READ", "VDC Tracks");
  if (!rootFile || rootFile->IsZombie()) {
    if (rootFile->IsOpen()) Close();
    return -1;
  }

  event = 0;

  tree = static_cast<TTree*>(rootFile->Get("tree"));
  tree->SetBranchAddress("track", &event);

  nentries = static_cast<Int_t>(tree->GetEntries());
  entry = 0;

  fOpened = kTRUE;
  return 0;
}

Int_t THaVDCSimRun::Close() {
  if (rootFile) {
    rootFile->Close();
    delete rootFile;
    rootFile = 0;
  }
  fOpened = kFALSE;
  return 0;
}

Int_t THaVDCSimRun::ReadEvent() {
  if (!IsOpen()) {
    Int_t ret = Open();
    if (ret) return ret;
  }

  tree->GetEntry(entry++);

  return 0;
}

const Int_t *THaVDCSimRun::GetEvBuffer() const {
  if (!IsOpen()) return NULL;

  return reinterpret_cast<Int_t*>(event);
}

THaVDCSimRun::~THaVDCSimRun() {
  if (IsOpen())
    Close();
}

ClassImp(THaVDCSimRun)
