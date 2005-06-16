#include "THaVDCSim.h"


ClassImp(THaVDCSimWireHit);
ClassImp(THaVDCSimEvent);
ClassImp(THaVDCSimConditions);
ClassImp(THaVDCSimTrack);

#include <iostream>
using namespace std;

void THaVDCSimTrack::Clear( const Option_t* opt ) {
  for (int i = 0; i < 4; i++)
    hits[i].Clear(opt);
}

void THaVDCSimTrack::Print( const Option_t* opt ) {
  //TObject::Print(opt);
  cout << "Track number = " << track_num << ", type = " << type 
       << ", layer = " << layer << ", obj = " << hex << this << dec << endl;
  cout << "  Origin   = (" << origin.X() << "," << origin.Y() << "," << origin.Z() << ")" << endl;
  cout << "  Momentum = (" << momentum.X() << "," << momentum.Y() << "," << momentum.Z() << ")" << endl;
  cout << "  #hits = " << hits[0].GetSize() << ", " << hits[1].GetSize() << ", "
       << hits[2].GetSize() << ", " << hits[3].GetSize() << ", " << endl;
}

THaVDCSimEvent::THaVDCSimEvent() {
  for (int i = 0; i < 4; i++){
    wirehits[i] = new TList;
  }
}

THaVDCSimEvent::~THaVDCSimEvent() {
  Clear();
  for (int i = 0; i < 4; i++)
    delete wirehits[i];
}

void THaVDCSimEvent::Clear( const Option_t* opt ) {
  for (Int_t i = 0; i < 4; i++)
    wirehits[i]->Delete(opt);

  // Debug
#ifdef DEBUG
  cout << tracks.GetSize() << endl;
  for( int i=0; i<tracks.GetSize(); i++ ) {
    THaVDCSimTrack* track = (THaVDCSimTrack*)tracks.At(i);
    track->Print();
  }
#endif

  tracks.Delete(opt);
}

void THaVDCSimConditions::set(Float_t *something,
			      Float_t a, Float_t b, Float_t c, Float_t d) {
  something[0] = a;
  something[1] = b;
  something[2] = c;
  something[3] = d;
}

Int_t THaVDCSimConditions::ReadDatabase(int j, float* timeOffsets, const int numWires)
  {
    //open the file as read only
    FILE* file = fopen("/u/home/orsborn/vdcsim/DB/20030415/db_L.vdc.dat", "r");
    if( !file ) return 1;

    // Use default values until ready to read from database

    const int LEN = 100;
    char buff[LEN];
  
    // Build the search tag and find it in the file. Search tags
    // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
    TString tag(Prefixes[j]); tag.Prepend("["); tag.Append("]"); 
    TString line, tag2(tag);
    tag.ToLower();
    // cout << "ReadDatabase:  tag = " << tag.Data() << endl;
    bool found = false;
    while (!found && fgets (buff, LEN, file) != NULL) {
      char* buf = ::Compress(buff);  //strip blanks
      line = buf;
      delete [] buf;
      if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
      line.ToLower();
      if ( tag == line ) 
      found = true;
    }
    if( !found ) {
      fclose(file);
      return 1;
    }
    
    for(int i = 0; i<7; i++)
      fgets(buff, LEN, file);  //skip 7 lines to get to actual offset data
  
    //Found the entry for this plane, so read time offsets and wire numbers
    int*   wire_nums    = new int[4*numWires];

    for (int i = j*numWires; i < (j+1)*numWires; i++) {
      int wnum = 0;
      float offset = 0.0;
      fscanf(file, " %d %f", &wnum, &offset);
      wire_nums[i] = wnum-1; // Wire numbers in file start at 1
      timeOffsets[i] = offset;
    }

    fclose(file);
    return 0;
    }
