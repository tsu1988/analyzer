/*
 * THaBendedBeam.h
 *
 *  Created on: Aug 3, 2010
 *      Author: jinhuang
 */

#ifndef THABENDEDBEAM_H_
#define THABENDEDBEAM_H_

#include "THaBeam.h"

#define NDEBUG
#include <cassert>

class THaBendedBeam: public THaBeam {
public:
	THaBendedBeam(const char* name, const char* description,
			THaBeamModule *throughbeam);
	THaBeamModule * SetThroughBeam(THaBeamModule *throughbeam) {
		if (!throughbeam) {Error("SetThroughBeam","Unknown through beam module"); return NULL;}
		else return fThroughBeam = throughbeam;
	}
	virtual ~THaBendedBeam();

	virtual EStatus Init(const TDatime& run_time);
	virtual Int_t Reconstruct();

protected:

	virtual Int_t ReadRunDatabase(const TDatime& date);

protected:
	THaBeamModule *fThroughBeam;

	Double_t fx_bend;
	Double_t fy_bend;
	Double_t fth_bend;
	Double_t fph_bend;

	ClassDef(THaBendedBeam,1) //  Beam Bended after BPM
};

#endif /* THABENDEDBEAM_H_ */
