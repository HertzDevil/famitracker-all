// Blip_Buffer provides a foundation for efficient real-time emulation of
// electronic sound generator chips like those in early video game consoles.

// Copyright (c) 2003-2004 Shay Green; licensed under GNU GPL (see source)

#ifndef BLIP_BUFFER_H
#define BLIP_BUFFER_H

#include "blargg_common.h"

class Blip_Reader;

// Blip_Buffer is a buffer of audio samples into which band-limited waveforms
// can be synthesized by using Blip_Wave or Blip_Synth to specify when the
// amplitude changes occur.
//
// Synthesis is usually of much greater duration than the size of the sample
// buffer and limited range of the time type. Because of this, synthesis must
// be broken into individual frames and time specified relative to the
// current frame. A frame of audio is made with Blip_Buffer by adding
// transitions and ended by specifying its duration to end_frame(); this
// begins a new frame after the old one. The resulting samples of previous
// frames can be read out of the buffer with read_samples(). Unread samples
// reduce buffer space available to the current frame.

// Source time unit.
typedef long blip_time_t;

// Number of samples of delay of the appearance of a transition in the samples
// read out.
int const blip_latency = 12 + 1;

// Type of sample produced.
typedef BOOST::int16_t blip_sample_t;

class Blip_Buffer {
	// noncopyable
	Blip_Buffer( Blip_Buffer const& );
	Blip_Buffer& operator = ( Blip_Buffer const& );
public:
	// Construct an empty buffer.
	Blip_Buffer();
	~Blip_Buffer();
	
	// Resize buffer to the number of samples specified, then clear buffer.
	// If memory is unavailable, set buffer size to 0 and report failure
	// by allowing exception to propagate (or return false if exceptions
	// aren't supported or aren't being used by the compilation environment).
	bool buffer_size( unsigned );
	
	// Set the number of blip_time_t units per sample, which must be greater
	// than 1.0. Current contents of buffer are unaffected.
	void units_per_sample( double );
	
	// Set the bass level, where 1.0 allows all bass and 0.0 removes it completely.
	void bass_level( double );
	
	// Remove any available samples and clear buffer to silence.
	void clear();
	
	// End current frame at specified time and make its samples (along with any
	// still-unread samples) available for reading with read_samples(). Begin
	// a new time frame at end of old frame, where time 0 is the beginning. All
	// transitions must have been added before specified time.
	void end_frame( blip_time_t );
	
	// Number of samples available for reading with read_samples().
	unsigned samples_avail() const;
	
	// Read at most 'max_samples' out of buffer into 'dest'. Return number of
	// samples actually read and removed. If stereo is true, increment 'dest' one
	// extra time after writing each sample, to allow easy interleving of two
	// channels into a stereo output buffer.
	unsigned read_samples( blip_sample_t* dest, unsigned max_samples, bool stereo = false );
	
	// Remove specified number of samples from those waiting to be read.
	void remove_samples( unsigned count );
	
	// not documented yet
	
	typedef unsigned long resampled_time_t;
	
	resampled_time_t resampled_time( blip_time_t t ) const {
		return t * resampled_time_t (factor_) + offset_;
	}
	
	resampled_time_t resampled_duration( int t ) const {
		return t * resampled_time_t (factor_);
	}
	
	// Don't use the following members. They are public only for technical reasons.
	
//private: // needed by Blip_Synth which can't be made friend in ARM C++
	 
	enum { widest_impulse = (blip_latency - 1) * 2 };
	
	typedef BOOST::uint16_t buf_t; // buffer sample
	
	resampled_time_t offset_;
	unsigned short factor_;
	unsigned short buffer_size_;
	buf_t* buffer_;
	
private:
	long reader_accum;
	int bass_shift;
	
	enum { accum_fract = 15 }; // less than 16 to give extra sample range
	enum { sample_offset = 0x7F7F }; // repeated byte allows memset to clear buffer
	
	friend class Blip_Reader;
};

	#ifndef BLIP_BUFFER_ACCURACY
	#	define BLIP_BUFFER_ACCURACY 16
	#endif

	int const blip_res_bits_ = 5;
	
	typedef BOOST::uint16_t blip_imp_t_;
	
	struct Blip_Eq;
	
	void blip_treble_eq( blip_imp_t_*, Blip_Eq const&, int width, float* fimp );
	void blip_fine_treble_eq( blip_imp_t_*, Blip_Eq const&, int width, float* fimp );
	
	BOOST::uint32_t blip_volume_unit( blip_imp_t_*, double unit, int width, float* fimp );
	BOOST::uint32_t blip_fine_volume_unit( blip_imp_t_*, double unit, int width, float* fimp );
	
	template<int quality,int mode>
	class Blip_Synth_Base {
	public:
		typedef BOOST::uint32_t pair_t;
		enum {
			width = quality < 5 ? quality * 4 : (blip_latency - 1) * 2,
			res = 1 << blip_res_bits_,
			fine_mode = (mode == 2)
		}; // needs to be public so union can use it
	protected:
		union {
			blip_imp_t_ impulse [res * 2 * width * mode + 1];
			float temp_buf [res * width / 2];
			double help_alignment; // encourage compiler to give impulse better alignment
		};
		Blip_Buffer* buf;
		pair_t impulse_offset;
		float fimpulse [(res / 2 + 1) * width + 8];
	public:
		Blip_Synth_Base() {
			fimpulse [0] = -1.0f;
			fimpulse [1] = -1.0f;
			buf = NULL;
		}
		
		// not documented yet
		void offset_resampled( Blip_Buffer::resampled_time_t time, int amp,
				Blip_Buffer* blip_buf ) const
		{
			pair_t offset = impulse_offset * amp;
			
			unsigned sample_index = (time >> BLIP_BUFFER_ACCURACY) & ~1;
			assert( sample_index < blip_buf->buffer_size_ );
			// ensure compiler optimizes this to a constant
			enum { const_offset = Blip_Buffer::widest_impulse / 2 - width / 2};
			pair_t* buf2 = (pair_t*) &blip_buf->buffer_ [const_offset + sample_index];
			
			int const shift = BLIP_BUFFER_ACCURACY - blip_res_bits_;
			pair_t const* imp2 = (pair_t*) &impulse
					[((time >> shift) & (res * 2 - 1)) * width * mode];
			
			if ( mode == 1 ) {
				// normal mode
				for ( int n = width / 4; n; --n ) {
					pair_t buf_0 = buf2 [0] - offset;
					pair_t buf_1 = buf2 [1] - offset;
					
					pair_t imp_0 = imp2 [0] * amp;
					pair_t imp_1 = imp2 [1] * amp;
					imp2 += 2;
					
					buf2 [0] = buf_0 + imp_0;
					buf2 [1] = buf_1 + imp_1;
					buf2 += 2;
				}
			}
			else {
				// fine mode
				int amp2 = BOOST::int8_t (amp); // keep low multiplier within -128 to 127
				amp = (amp + 128) >> 8;
				
				for ( int n = width / 4; n; --n )
				{
					pair_t buf_0 = buf2 [0] - offset;
					pair_t buf_1 = buf2 [1] - offset;
					
					pair_t imp_0 = imp2 [0] * amp2;
					pair_t imp_1 = imp2 [1] * amp;
					
					pair_t imp_2 = imp2 [2] * amp2;
					pair_t imp_3 = imp2 [3] * amp;
					
					imp2 += 4;
					
					buf2 [0] = buf_0 + imp_0 + imp_1;
					buf2 [1] = buf_1 + imp_2 + imp_3;
					buf2 += 2;
				}
			}
		}
		
		void offset( blip_time_t, int amp, Blip_Buffer* ) const;
		
		void offset_inline( blip_time_t time, int amp, Blip_Buffer* buf ) const {
			offset_resampled( time * buf->factor_ + buf->offset_, amp, buf );
		}
		
		void offset_resampled( Blip_Buffer::resampled_time_t t, int o ) const {
			offset_resampled( t, o, buf );
		}
		
		void offset( blip_time_t t, int o ) const {
			offset( t, o, buf );
		}
		
		void offset_inline( blip_time_t t, int o ) const {
			offset_inline( t, o, buf );
		}
	};

// High frequency equalization parameters used by Blip_Synth. Begin
// exponential rolloff at sampling frequency / 2 * cutoff and pass through
// point at sampling frequency / 2 * treble_point with an attenuation of
// treble. Treble can be greater than 1.0, which results in high-frequency
// emphasis. Cutoff must be less than treble_point.
//
// 0.0   <- cutoff ->     1.0 :
// -----------*             | |
//             \            | |
//              `-*_        | treble (can go beyond 1.0)
//                  ```-----| |
//      <- treble_point ->  | v
// - - - - - - - - - - - - -| 0.0
// 0      frequency       fs/2
struct Blip_Eq {
	Blip_Eq( double treble = 0.50f, double cutoff = 0.0f, double treble_point = 1.0f ) :
			treble_( treble ), cutoff_( cutoff ), treble_point_( treble_point ) {
	}
	double cutoff_, treble_, treble_point_;
};

// A Blip_Synth is a transition waveform synthesizer which adds band-limited
// offsets (transitions) into a Blip_Buffer at specified source times. Its
// quality and high-end frequency equalization can be adjusted. Use Blip_Wave
// for a higher-level equivalent that's easier to use.
//
// Quality ranges from 1 to 5, where 3 average and 5 is best; performance is
// better for lower qualities. Amp_range specifies the highest expected
// amplitude of the output wave in terms of the volume unit; when amp_range
// is large (256 or higher) a slightly slower implementation is necessary to
// avoid quality loss due to amplification of slight errors.
template<int quality,int amp_range = 1>
class Blip_Synth : public Blip_Synth_Base<quality,(amp_range > 256 ? 2 : 1)> {
	BOOST_STATIC_ASSERT( 1 <= quality && quality <= 5 );
	BOOST_STATIC_ASSERT( 0 < amp_range && amp_range < 0x8000 );
public:
	
	// Set high frequency equalization (refer to Blip_Eq above).
	void treble_eq( Blip_Eq const& );
	
	// Set base volume unit of transitions, where 1.0 is a full swing between the
	// positive and negative extremes. Not meant for real-time modulation of sound
	// volume.
	void volume_unit( double unit );
	
	// Set volume of a transition at amplitude 'amp_range' by setting volume_unit
	// to amp_range / v.
	void volume( double v )			{ volume_unit( (1.0f / amp_range) * v ); }
	
	// Default Blip_Buffer used for output.
	Blip_Buffer* output() const						{ return buf; }
	void output( Blip_Buffer* b )					{ buf = b; }
	
	// Add an amplitude offset (transition) with an amplitude of amp * volume_unit
	// into the specified buffer (default buffer if none specified) at the
	// specified source time. Amplitude can be positive or negative. To increase
	// performance by inlining code at the call site, use add_inline().
	//void offset( blip_time_t, int amp ) const;
	//void offset( blip_time_t, int amp, Blip_Buffer* ) const;
	//void offset_inline( blip_time_t, int amp ) const;
	//void offset_inline( blip_time_t, int amp, Blip_Buffer* ) const;
};

// Blip_Wave is a synthesizer for adding a single waveform to a Blip_Buffer.
// A wave can be built as a series of delays and new amplitudes. This is simpler
// than the low-level interface of Blip_Synth which deals with offsets.
template<int quality,int range = 1>
class Blip_Wave : private Blip_Synth<quality,range> {
	blip_time_t time_;
	int amp;
public:
	Blip_Wave() : amp( 0 ), time_( 0 ) {
	}
	
	// See Blip_Synth for documentation of these functions.
	Blip_Synth<quality,range>::treble_eq;
	Blip_Synth<quality,range>::volume_unit;
	Blip_Synth<quality,range>::volume;
	Blip_Synth<quality,range>::output;
	
	// Time relative to beginning of current frame
	blip_time_t time() const		{ return time_; }
	void time( blip_time_t t )		{ time_ = t; }
	
	// Move current time forward by 't' time units.
	void delay( blip_time_t t )		{ time_ += t; }
	
	// Current amplitude
	int amplitude() const			{ return amp; }
	void amplitude( int new_amp ) {
		int diff = new_amp - amp;
		if ( diff ) {
			amp = new_amp;
			offset( time_, diff );
		}
	}
	
	// Update amplitude without outputting a transition.
	void update( int new_amp )		{ amp = new_amp; }
	
	// End current frame and adjust time to be relative to new frame.
	void end_frame( blip_time_t t ) {
		assert( time_ >= t );
		time_ -= t;
	}
};

	// internal

	template<int quality,int amp_range>
	inline void Blip_Synth<quality,amp_range>::treble_eq( Blip_Eq const& eq ) {
		(fine_mode ? blip_fine_treble_eq : blip_treble_eq)( impulse, eq, width, fimpulse );
	}
	
	template<int quality,int amp_range>
	inline void Blip_Synth<quality,amp_range>::volume_unit( double unit ) {
		impulse_offset = (fine_mode ? blip_fine_volume_unit : blip_volume_unit)(
				impulse, unit, width, fimpulse );
	}
	
	template<int quality,int mode>
	void Blip_Synth_Base<quality,mode>::offset( blip_time_t time, int amp, Blip_Buffer* buf ) const {
		offset_resampled( time * buf->factor_ + buf->offset_, amp, buf );
	}
	
	inline void Blip_Buffer::bass_level( double bl ) {
		bass_shift = int (bl * bl * 31 + 0.5f);
	}
	
	inline unsigned Blip_Buffer::samples_avail() const {
		return offset_ >> BLIP_BUFFER_ACCURACY;
	}

	inline void Blip_Buffer::end_frame( blip_time_t t ) {
		offset_ += t * factor_;
		assert( samples_avail() <= buffer_size_ );
	}

#endif
