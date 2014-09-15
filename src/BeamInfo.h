#ifndef Podd_BeamInfo
#define Podd_BeamInfo

//////////////////////////////////////////////////////////////////////////
//
// BeamInfo
//
//////////////////////////////////////////////////////////////////////////

#include "TVector3.h"

namespace Podd {

class Beam;

class BeamInfo {
public:
  BeamInfo() : fPosition(kBig,kBig,kBig), fPvect(kBig,kBig,kBig), fPol(0.0),
	       fOK(0), fBeam(0) {}
  BeamInfo( const TVector3& pvect, const TVector3& position,
	    Double_t pol = 0.0 )
    : fPosition(position), fPvect(pvect), fPol(pol), fOK(1), fBeam(0) {}
  BeamInfo( Double_t p, const TVector3& vect, const TVector3& position,
	    Double_t pol = 0.0 )
    : fPosition(position), fPvect(vect), fPol(pol), fOK(1), fBeam(0)
  { SetP(p); }

  BeamInfo( const BeamInfo& t ) :
    fPosition(t.fPosition), fPvect(t.fPvect), fPol(t.fPol), fOK(t.fOK),
    fBeam(t.fBeam) {}
  BeamInfo& operator=( const BeamInfo& rhs ) {
    if( this != &rhs ) {
      fPosition = rhs.fPosition;
      fPvect    = rhs.fPvect;
      fPol      = rhs.fPol;
      fOK       = rhs.fOK;
      fBeam     = rhs.fBeam;
    }
    return *this;
  }
  virtual ~BeamInfo() {}

  void      Clear( Option_t* opt="" );
  Bool_t    IsOK()     const { return fOK; }
  Double_t  GetPx()    const { return fPvect.X(); }
  Double_t  GetPy()    const { return fPvect.Y(); }
  Double_t  GetPz()    const { return fPvect.Z(); }
  Double_t  GetP()     const { return fPvect.Mag(); }
  Double_t  GetX()     const { return fPosition.X(); }
  Double_t  GetY()     const { return fPosition.Y(); }
  Double_t  GetZ()     const { return fPosition.Z(); }
  Double_t  GetTheta() const { return (GetPz() != 0.0) ? GetPx()/GetPz():kBig;}
  Double_t  GetPhi()   const { return (GetPz() != 0.0) ? GetPy()/GetPz():kBig;}
  Double_t  GetPol()   const { return fPol; }
  Double_t  GetE()     const;
  Double_t  GetM()     const;
  Int_t     GetQ()     const;
  Double_t  GetdE()    const;
  const TVector3& GetPvect()    const { return fPvect; }
  const TVector3& GetPosition() const { return fPosition; }

  Beam*  GetBeam() const  { return fBeam; }

  void SetP( Double_t p )     { fPvect.SetMag(p); }
  void SetPol( Double_t pol ) { fPol = pol; }
  void Set( const TVector3& pvect, const TVector3& position,
	    Double_t pol = 0.0 ) {
    fPvect = pvect; fPosition = position; SetPol(pol); fOK = 1;
  }
  void Set( Double_t p, const TVector3& vect, const TVector3& position,
	    Double_t pol = 0.0 ) {
    fPvect = vect; SetP(p); fPosition = position; SetPol(pol); fOK = 1;
  }
  void SetBeam( Beam* obj )  { fBeam = obj; }

protected:
  TVector3  fPosition;  // Reference position in lab frame (m)
  TVector3  fPvect;     // Momentum vector in lab (GeV/c)
  Double_t  fPol;       // Beam polarization
  Int_t     fOK;        // Data ok (0:no 1:yes)

  Beam*     fBeam;      //! Beam apparatus for this beam information

private:
  static const Double_t kBig;

  ClassDef(BeamInfo,1)  // Beam information for physics modules
};

//_____________________________________________________________________________
inline
void BeamInfo::Clear( Option_t* )
{
  fPvect.SetXYZ(kBig,kBig,kBig);
  fPosition.SetXYZ(kBig,kBig,kBig);
  fPol = 0.0;
  fOK = 0;
}

} // end namespace Podd

// Backwards-compatibility
#define THaBeamInfo Podd::BeamInfo

#endif

