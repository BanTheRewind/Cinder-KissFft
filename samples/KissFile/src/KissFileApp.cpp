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
#include "cinder/audio/Io.h"
#include "cinder/audio/Output.h"
#include "cinder/CinderMath.h"
#include "KissFFT.h"
#include "Resources.h"

/*
 * This application demonstrates how to read spectrum data
 * from an audio file with KissFFT.
 * 
 * The clip is from "Let Us Pray" by Let's Go Outside on 
 * the album "Conversations With My Invisible Friends"
 * (c) 2009 Soma Quality Recordsings
 * 
 * http://www.letsgooutside.com/
 * http://www.somarecords.com/
 * 
 */
class KissFileApp : public ci::app::AppBasic 
{

public:

	// Cinder callbacks
	void						draw();
	void						setup();
	void						shutdown();
	void						update();

private:

	// Audio file
	ci::audio::SourceRef		mAudioSource;
	ci::audio::PcmBuffer32fRef	mBuffer;
	ci::audio::TrackRef			mTrack;

	// Analyzer
	KissRef						mFft;
	bool						mFftInit;

};

// Imports
using namespace ci;
using namespace ci::app;

// Draw
void KissFileApp::draw()
{

	// Clear screen
	gl::clear( ColorAf::black() );

	// Check init flag
	if ( mFftInit ) {

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
			float logSize = math<float>::log( dataSizef );
			float x = (float)( ( math<float>::log( (float)i ) / logSize ) * dataSizef );
			float y = math<float>::clamp( freqData[ i ] * ( x / dataSizef ) * ( math<float>::log( ( dataSizef - (float)i ) ) ), 0.0f, 2.0f );

			// Plot points on lines for tme domain
			freqLine.push_back( Vec2f(        x * scale + 10.0f,            -y * ( windowHeight - 20.0f ) * 0.25f + ( windowHeight - 10.0f ) ) );
			timeLine.push_back( Vec2f( (float)i * scale + 10.0f, timeData[ i ] * ( windowHeight - 20.0f ) * 0.25f + ( windowHeight * 0.25f + 10.0f ) ) );

		}

		// Draw signals
		gl::draw( freqLine );
		gl::draw( timeLine );

	}

}

// Set up
void KissFileApp::setup()
{

	// Set up window
	setFrameRate( 60.0f );
	setWindowSize( 600, 600 );

	// Set up line rendering
	gl::enable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	gl::color( ColorAf::white() );
	
	// Load and play audio
	mAudioSource = audio::load( loadResource( RES_SAMPLE ) );
	mTrack = audio::Output::addTrack( mAudioSource, false );
	mTrack->enablePcmBuffering( true );
	mTrack->play();

	// Set initialization flag
	mFftInit = false;

}

// Called on exit
void KissFileApp::shutdown() 
{

	// Stop track
	mTrack->enablePcmBuffering( false );
	mTrack->stop();

}

// Runs update logic
void KissFileApp::update() 
{

	// Check if track is playing and has a PCM buffer available
	if ( mTrack->isPlaying() && mTrack->isPcmBuffering() ) {

		// Get buffer
		mBuffer = mTrack->getPcmBuffer();
		if ( mBuffer && mBuffer->getInterleavedData() ) {

			// Get sample count
			uint32_t sampleCount = mBuffer->getInterleavedData()->mSampleCount;
			if ( sampleCount > 0 ) {

				// Initialize analyzer
				if ( !mFftInit ) {
					mFftInit = true;
					mFft = Kiss::create( sampleCount );
				}

				// Analyze data
				if ( mBuffer->getInterleavedData()->mData != 0 ) {
					mFft->setData( mBuffer->getInterleavedData()->mData );
				}

			}

		}

	}

}

// Start application
CINDER_APP_BASIC( KissFileApp, RendererGl )
