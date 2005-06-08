#include "THaVDCSimRun.h"
#include "THaVDCSim.h"

#include "TFile.h"
#include "TTree.h"
#include <cstdio>
#include <evio.h>

using namespace std;

THaVDCSimRun::THaVDCSimRun(const char* description) :
  THaRunBase(description), event(0), nentries(0), entry(0) 
{
  // Constructor
}

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
  branch = tree->GetBranch("track");
  branch->SetAddress(&event);

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
  Int_t ret;
  if (!IsOpen()) {
    ret = Open();
    if (ret) return ret;
  }

  ret = branch->GetEntry(entry++);
  if( ret > 0 )
    return S_SUCCESS;
  else if ( ret == 0 )
    return EOF;
  return -128;  // CODA_ERR
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
