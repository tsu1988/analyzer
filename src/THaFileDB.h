#ifndef Podd_THaFilter
#define Podd_THaFilter

//////////////////////////////////////////////////////////////////////////
//
// THaFileDB
// 
//////////////////////////////////////////////////////////////////////////


class THaFileDB : public THaDB {
 public:
  THaFileDB();
  virtual ~THaFileDB();
  
  virtual Int_t Init(const TDatime&);
  virtual Int_t Process( const THaEvData*, const THaRunBase*, Int_t code );
  virtual Int_t Close();

  THaCut* GetCut() const { return fCut; }

 protected:
  TString   fCutExpr;    // Definition of cut to use for filtering events
  TString   fFileName;   // Name of CODA output file
  THaCodaFile* fCodaOut; // The CODA output file
  THaCut*   fCut;        // Pointer to cut used for filtering
  
 public:
  ClassDef(THaFileDB,0)
};

#endif

