/*
* 
* Copyright (c) 2012, Ban the Rewind
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#pragma once

// Includes
#include "cinder/CinderMath.h"
#include "kiss/kiss_fftr.h"

// Alias for pointer to Kiss instance
typedef std::shared_ptr<class Kiss> KissRef;

// KissFFT wrapper
class Kiss
{

public:

	// Filter types
	struct Filter
	{
		enum
		{
			NONE, 
			LOW_PASS, 
			HIGH_PASS, 
			NOTCH
		};
	};

	// Creates pointer to Kiss instance
	static KissRef	create( int32_t dataSize = 512 );

	// De-structor
	~Kiss();

	// Stop processing
	void			stop();

	// Convenience method for shutting off filter
	void			removeFilter();

	// Setters
	void			setData( float *data );
	void			setDataSize( int32_t dataSize );
	void			setFilter( float lowFrequency, float highFrequency );
	void			setFilter( float frequency, int32_t filter = Filter::LOW_PASS );

	// Getters
	float*			getAmplitude();
	int32_t			getBinSize() { return mBinSize; }
	float*			getData();
	int32_t			getDataSize() { return mDataSize; }
	float*			getImaginary();
	float*			getPhase();
	float*			getReal();

private:

	// Constructor
	// NOTE: Make this public to build
	// directory on the stack (advanced)
	Kiss( int32_t dataSize = 512 );

	// Clean up
	void			dispose();

	// Arrays
	float			*mAmplitude;
	float			*mData;
	float			*mImag;
	float			*mInverseWindow;
	float			*mPhase;
	float			*mReal;
	float			*mWindow;
	float			*mWindowedData;

	// Dimensions
	int32_t			mBinSize;
	int32_t			mDataSize;
	float			mWindowSum;

	// Flags
	bool			mCartesianNormalized;
	bool			mCartesianUpdated;
	bool			mDataNormalized;
	bool			mDataUpdated;
	bool			mPolarNormalized;
	bool			mPolarUpdated;

	// Performs FFT
	void			transform();

	// Sets amplitude and phase arrays
	void			cartesianToPolar();

	// KissFFT
	kiss_fft_cpx	*mCxIn;
	kiss_fft_cpx	*mCxOut;
	kiss_fftr_cfg	mFftCfg;
	kiss_fftr_cfg	mIfftCfg;

	// Filter frequencies
	float			mFrequencyHigh;
	float			mFrequencyLow;

	// Running flag
	bool			mRunning;

};
