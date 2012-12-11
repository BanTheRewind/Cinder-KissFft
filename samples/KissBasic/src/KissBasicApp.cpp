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

// Includes
#include "cinder/app/AppBasic.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Callback.h"
#include "cinder/CinderMath.h"
#include "KissFFT.h"

/*
 * This application demonstrates how to read spectrum
 * data in real time with KissFFT using synthesis as
 * the audio input.
 */
class KissBasicApp : public ci::app::AppBasic
{

public:

	// Cinder callbacks
	void	draw();
	void	mouseMove( ci::app::MouseEvent event );
	void	setup();
	void	shutdown();
	void	sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, ci::audio::Buffer32f *ioBuffer );

private:

	// Audio generation settings
	float	mAmplitude;
	float	mFreqTarget;
	float	mPhase;
	float	mPhaseAdjust;
	float	mMaxFreq;
	float	mMinFreq;

	// Analyzer
	KissRef mFft;

};

// Imports
using namespace ci;
using namespace ci::app;
using namespace std;

// Draw
void KissBasicApp::draw()
{

	// Clear screen
	gl::clear( ColorAf::black() );

	// Check init flag
	if ( mFft ) {

		// Get data in the frequency (transformed) and time domains
		float * freqData = mFft->getAmplitude();
		float * timeData = mFft->getData();
		int32_t dataSize = mFft->getBinSize();

		// Cast data size to float
		float dataSizef = (float)dataSize;

		// Get dimensions
		float scale = ( (float)getWindowWidth() - 20.0f ) / dataSizef;
		float windowHeight = (float)getWindowHeight();

		// Use polylines to depict time and frequency domains
		PolyLine<Vec2f> freqLine;
		PolyLine<Vec2f> timeLine;

		// Iterate through data
		for ( int32_t i = 0; i < dataSize; i++ ) {

			// Do logarithmic plotting for frequency domain
			float logSize	= math<float>::log( dataSizef );
			float x			= (float)( (math<float>::log( (float)i) / logSize ) * dataSizef );
			float y			= math<float>::clamp( freqData[i] * ( x / dataSizef ) * ( math<float>::log( ( dataSizef - (float)i ) ) ), 0.0f, 2.0f );

			// Plot points on lines for tme domain
			freqLine.push_back( Vec2f(        x * scale + 10.0f,          -y   * ( windowHeight - 20.0f ) * 0.25f + ( windowHeight - 10.0f ) ) );
			timeLine.push_back( Vec2f( (float)i * scale + 10.0f, timeData[ i ] * ( windowHeight - 20.0f ) * 0.25f + ( windowHeight * 0.25f + 10.0f ) ) );

		}

		// Draw signals
		gl::draw( freqLine );
		gl::draw( timeLine );

	}

}

// Handles mouse movement
void KissBasicApp::mouseMove( MouseEvent event )
{
	// Change frequency and amplitude based on mouse position
	// Scale everything logarithmically to get a better feel and sound
	mAmplitude		= 1.0f - event.getY() / (float)getWindowHeight();
	double width	= (double)getWindowWidth();
	double x		= width - (double)event.getX();
	float mPosition	= (float)( ( log( width ) - log( x ) ) / log( width ) );
	mFreqTarget		= math<float>::clamp( mMaxFreq * mPosition, mMinFreq, mMaxFreq );
	mAmplitude		= math<float>::clamp( mAmplitude * ( 1.0f - mPosition ), 0.05f, 1.0f );
}

// Set up
void KissBasicApp::setup()
{
	// Set up window
	setFrameRate( 60.0f );
	setWindowSize( 600, 600 );

	// Set up line rendering
	gl::enable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	gl::color( ColorAf::white() );

	// Set synth properties
	mAmplitude		= 0.5f;
	mMaxFreq		= 20000.0f;
	mMinFreq		= 1.0f;
	mFreqTarget		= 0.0f;
	mPhase			= 0.0f;
	mPhaseAdjust	= 0.0f;
	
	// Play sine
	audio::Output::play( audio::createCallback( this, &KissBasicApp::sineWave ) );
}

// Called on exit
void KissBasicApp::shutdown()
{
	if ( mFft ) {
		mFft->stop();
	}
}

// Creates sine wave
void KissBasicApp::sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer ) 
{

	// Fill buffer with sine wave
	mPhaseAdjust = mPhaseAdjust * 0.95f + ( mFreqTarget / 44100.0f ) * 0.05f;
	for ( uint32_t i = 0; i < ioSampleCount; i++ ) {
		mPhase		+= mPhaseAdjust;
		mPhase		= mPhase - math<float>::floor( mPhase );
		float val	= math<float>::sin( mPhase * 2.0f * (float)M_PI ) * mAmplitude;
		ioBuffer->mData[ i * ioBuffer->mNumberChannels ] = val;
		ioBuffer->mData[ i * ioBuffer->mNumberChannels + 1 ] = val;
	}

	// Initialize analyzer
	if ( !mFft ) {
		mFft = Kiss::create( ioSampleCount );
	}

	// Analyze data
	mFft->setData( ioBuffer->mData );

}

// Start application
CINDER_APP_BASIC( KissBasicApp, RendererGl )
