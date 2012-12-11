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
#include "cinder/audio/Io.h"
#include "cinder/CinderMath.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Utilities.h"
#include "KissFFT.h"
#include "Resources.h"

/*
 * This example demonstrates how to detect tempo in a song
 * using the KissFFT block as a low pass filter. The track is
 * transformed to the frequency domain, filtered, then transformed
 * back to the time domain. We then go through each sample and 
 * measure the distance between peaks in volume. That distance is
 * then used to calculate the duration of a beat.
 * 
 * The tempo of the sample track is 125 bpm, so you should see
 * it hovering around there for a bit before more or less locking 
 * in. This application does very simple tempo detection. Consider 
 * a tempo range, averaging, and or scoring and sorting tempos 
 * to get more accurate and consistent results.
 * 
 * The clip is from "Machismo" by Let's Go Outside on the album
 * "Conversations With My Invisible Friends"
 * (c) 2009 Soma Quality Recordsings
 * 
 * http://www.somarecords.com/
 * 
 */
class KissTempoApp : public ci::app::AppBasic 
{

public:

	// Cinder callbacks
	void draw();
	void keyDown( ci::app::KeyEvent event );
	void setup();
	void shutdown();
	void update();

private:

	// Set default neighbor count. This is the number of neighbors 
	// needed to evaluate a peak. Higher numbers are better for 
	// more complex music.
#ifdef CINDER_MSW
    static const int32_t DEFAULT_NEIGHBOR_COUNT = 4;
#else
	static const int32_t DEFAULT_NEIGHBOR_COUNT = 2;
#endif

	// Audio file
	void playTrack();
	ci::audio::PcmBuffer32fRef	mBuffer;
	ci::audio::TrackRef			mTrack;

	// Analyzer
	KissRef						mFft;

	// Data
	int32_t						mDataSize;
	float						*mInputData;
	int32_t						mInputSize;
	float						*mTimeData;

	// Tempo information
	int32_t						mFirstPeak;
	int32_t						mNeighbors;
	std::vector<int32_t>		mPeakDistances;
	int32_t						mSampleDistance;
	float						mTempo;
	float						mThreshold;

	// Waveform drawing data
	std::vector<float>			mWaveform;

	// Text
	ci::gl::TextureFontRef		mFont;

};

// Imports
using namespace ci;
using namespace ci::app;
using namespace std;

// Draw
void KissTempoApp::draw()
{
	// Clear screen
	gl::clear( ColorAf::black() );

	// Check sizes
	if ( mDataSize > 0 && mWaveform.size() > 0 ) {

		// Get dimensions
		float windowWidth	= (float)getWindowWidth();
		float center		= windowWidth * 0.5f;

		// Draw waveform
		float y = 0.0f;
		PolyLine<Vec2f> mLine;
		uint32_t ampCount = mWaveform.size();
		for ( uint32_t i = 0; i < ampCount; i++, y += 6.5f ) {
			float x = mWaveform[ i ] * windowWidth;
			mLine.push_back( Vec2f( center + x, y ) );
			mLine.push_back( Vec2f( center - x, y + 3.25f ) );
		}
		gl::draw( mLine );

	}

	// Draw tempo (scale text to improve quality)
	gl::pushMatrices();
	gl::scale( 0.25f, 0.25f );
	mFont->drawString( toString( (int32_t)math<float>::ceil( mTempo ) ) + " BPM", Vec2f( 20.0f * 4.0f, 540.0f * 4.0f ) );
	gl::scale( 0.5f, 0.5f );
	mFont->drawString( "Press SPACE to reset track", Vec2f( 20.0f * 8.0f, 575.0f * 8.0f ) );
	gl::popMatrices();
}

// Handles key press
void KissTempoApp::keyDown( KeyEvent event )
{
	// Press ESC to quit or "SPACE" to switch tracks
	switch ( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		playTrack();
		break;
	}
}

// Play the track
void KissTempoApp::playTrack()
{
	// Stop current track
	if ( mTrack ) {
		mTrack->enablePcmBuffering( false );
		mTrack->stop();
		mTrack.reset();
	}

	// Reset values
	mFirstPeak		= -1;
	mNeighbors		= DEFAULT_NEIGHBOR_COUNT;
	mSampleDistance = 0;
	mTempo			= 0.0f;
	mPeakDistances.clear();

	// Play track
	mTrack = audio::Output::addTrack( audio::load( loadResource( RES_SAMPLE ) ), false );
	mTrack->enablePcmBuffering( true );
	mTrack->play();
}

// Set up
void KissTempoApp::setup()
{
	// Set up window
	setFrameRate( 60.0f );
	setWindowSize( 600, 600 );

	// Set up line and text rendering
	gl::enable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	gl::enable( GL_POLYGON_SMOOTH );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	gl::color( ColorAf::white() );

	// Define properties
	mDataSize		= 0;
	mFirstPeak		= -1;
	mInputSize		= 0;
	mNeighbors		= DEFAULT_NEIGHBOR_COUNT;
	mSampleDistance = 0;
	mTempo			= 0.0f;
	mThreshold		= 0.1f;

	// Load font
	mFont = gl::TextureFont::create( Font( loadResource( RES_FONT ), 96.0f ) );

	// Load sample
	playTrack();
}

// Called on exit
void KissTempoApp::shutdown() 
{
	// Stop track
	mTrack->enablePcmBuffering( false );
	mTrack->stop();
	if ( mFft ) {
		mFft->stop();
	}
}

// Runs update logic
void KissTempoApp::update() 
{
	// Don't evaluate right away or unrealistically 
	// high numbers will pop up
	if ( getElapsedSeconds() < 0.5 ) {
		return;
	}

	// Check if track is playing and has a PCM buffer available
	if ( mTrack && mTrack->isPlaying() && mTrack->isPcmBuffering() ) {

		// Get buffer
		mBuffer = mTrack->getPcmBuffer();
		if ( mBuffer && mBuffer->getInterleavedData() ) {

			// Get sample count
			uint32_t sampleCount = mBuffer->getInterleavedData()->mSampleCount;
			if ( sampleCount > 0 ) {

				// Kiss is not initialized
				if ( !mFft ) {

					// Initialize analyzer
					mFft = Kiss::create( sampleCount );

					// Set filter on FFT to calculate tempo based on beats
					mFft->setFilter( 0.2f, Kiss::Filter::LOW_PASS );

				}

				// Analyze data
				if ( mBuffer->getInterleavedData()->mData != 0 ) {

					// Set FFT data
					mInputData = mBuffer->getInterleavedData()->mData;
					mInputSize = mBuffer->getInterleavedData()->mSampleCount;
					mFft->setData( mInputData );

					// Get data
					mTimeData = mFft->getData();
					mDataSize = mFft->getBinSize();

					// Iterate through amplitude data
					for ( int32_t i = 0; i < mDataSize; i++, mSampleDistance++ ) {

						// Check value against threshold
						if ( mTimeData[ i ] >= mThreshold ) {

							// Determine neighbor range
							int32_t start	= math<int32_t>::max( i - mNeighbors, 0 );
							int32_t end		= math<int32_t>::min( i + mNeighbors, mDataSize - 1 );

							// Compare this value with neighbors to find peak
							bool peak = true;
							for ( int32_t j = start; j < end; j++ ) {
								if ( j != i && mTimeData[ i ] <= mTimeData[ j ] ) {
									peak = false;
									break;
								}
							}

							// This is a peak
							if ( peak ) {

								// Add distance between this peak and last into the 
								// list. Just note position if this is the first peak.
								if ( mFirstPeak >= 0 ) {
									mPeakDistances.push_back( mSampleDistance );
								} else {
									mFirstPeak = mSampleDistance;
								}
							
								// Reset distance counter
								mSampleDistance = 0;

							}

						}

					}

				}

			}

			// We have multiple peaks to compare
			if ( mPeakDistances.size() > 1 ) {

				// Add distances
				int32_t total = 0;
				uint32_t peakCount = mPeakDistances.size();
				for ( uint32_t i = 0; i < peakCount; i++ ) {
					total += mPeakDistances[ i ];
				}

				// Determine tempo based on average peak distance
				mTempo = total > 0 ? ( 44100.0f / ( (float)total / (float)mPeakDistances.size() ) ) * 60.0f / 1000.0f : 0.0f;

			}

			// Get current window height
			int32_t windowHeight = getWindowHeight() / 8;

			// Add up values, combine input and filtered values
			// to emphasize bass
			float total = 0.0f;
			for ( int32_t i = 0; i < mDataSize; i++ ) {
				if ( i * 8 < mInputSize ) {
					total += mTimeData[ i ] * 2.0f * mInputData[ i * 8 ];
				}
			}
		
			// Add average to drawing line
			mWaveform.push_back( total / (float)mDataSize );

			// Remove points when vector is wider than screen
			while ( mWaveform.size() >= (uint32_t)windowHeight ) {
				mWaveform.erase( mWaveform.begin() );
			}

		}

	}
}

// Start application
CINDER_APP_BASIC( KissTempoApp, RendererGl )
