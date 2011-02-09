/*
 * THaBendedBeam.C
 *
 *  Created on: Aug 3, 2010
 *      Author: jinhuang
 *
 *  WARNING: Through beam module should be added into gHaApps before this class to ensure it's processed first.
 */

#include "THaBendedBeam.h"
#include "TMath.h"
#include "TDatime.h"
#include "VarDef.h"


ClassImp(THaBendedBeam)

//_____________________________________________________________________________
THaBendedBeam::THaBendedBeam(const char* name, const char* description,
		THaBeamModule *throughbeam) :
		THaBeam(name, description) {
	// TODO Auto-generated constructor stub
	SetThroughBeam(throughbeam);
	fDirection.SetXYZ(0.0, 0.0, 1.0);
}

//_____________________________________________________________________________
THaBendedBeam::~THaBendedBeam() {
	// TODO Auto-generated destructor stub
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaBendedBeam::Init(const TDatime& run_time) {
	// Module initialization: get run parameters, then read databases. Calls
	// our ReadRunDatabase below and sets our fixed position & direction.

	if (!fThroughBeam)
	{Error("Init","Unknown through beam module"); return kInitError;}

	if (THaBeam::Init(run_time))
		return fStatus;

	// Copy our beam data to our beam info variables. This can be done here
	// instead of in Reconstruct() because the info is constant.
	Update();
	return kOK;
}

//_____________________________________________________________________________
Int_t THaBendedBeam::Reconstruct() {
	// Through beam module should be added into gHaApps before this class to ensure it's processed first

	assert(fThroughBeam);

	fBeamIfo = *(fThroughBeam -> GetBeamInfo());
	fBeamIfo.SetBeam(this);

	fPosition = fBeamIfo.GetPosition();  // Reference position in lab frame (m)
	TVector3  fPvect = fBeamIfo.GetPvect();     // Momentum vector in lab (GeV/c)

	fPosition.SetX(fPosition.x()+fx_bend);
	fPosition.SetY(fPosition.y()+fy_bend);

	fPvect.SetX(fPvect.x()+fth_bend*fPvect.z());
	fPvect.SetY(fPvect.y()+fph_bend*fPvect.z());

	fBeamIfo.Set(fPvect, fPosition, fBeamIfo.GetPol());

	if (fDebug>=3)
	{
		fPosition.Print();
		fPvect.Print();
	}

	fDirection = fPvect;
	Update();

	return 0;
}

//_____________________________________________________________________________
Int_t THaBendedBeam::ReadRunDatabase( const TDatime& date )
{
	// Query the run database for the position and direction information.
	// Tags:  <prefix>.x     (m)
	//        <prefix>.y     (m)
	//        <prefix>.theta (deg)
	//        <prefix>.phi   (deg)
	//
	// If no tags exist, position and direction will be set to (0,0,0)
	// and (0,0,1), respectively.

	Int_t err = THaBeam::ReadRunDatabase( date );
	if( err ) return err;

	FILE* file = OpenRunDBFile( date );
	if( !file ) return kFileError;

	//	static const Double_t degrad = TMath::Pi()/180.0;

	Double_t x = 0.0, y = 0.0, th = 0.0, ph = 0.0;

	const TagDef tags[] = {
// 		{ "x_bend",     &x },
//   { "y_bend",     &y },
//   { "dx_bend", 	&th },
//   { "dy_bend",   	&ph },
  { "beambend_x",     &x },
  { "beambend_y",     &y },
  { "beambend_dx", 	&th },
  { "beambend_dy",   	&ph },
			{ 0 }
	};
	LoadDB( file, date, tags/*, fPrefix*/ );

	fPosition.SetXYZ( x, y, 0.0 );

	Double_t tt = th;
	Double_t tp = ph;
	fDirection.SetXYZ( tt, tp, 1.0 );
	fDirection *= 1.0/TMath::Sqrt( 1.0+tt*tt+tp*tp );

	fx_bend = x;
	fy_bend = y;
	fth_bend = th;
	fph_bend = ph;

	Info(Here("ReadRunDatabase"),"Bend beam by x=%f, y=%f, dx=%f, dy=%f",
			x, y, th, ph);

	fclose(file);
	return kOK;
}
