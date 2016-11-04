// Blip_Buffer provides a foundation for efficient real-time emulation of
// electronic sound generator chips like those in early video game consoles.

// http://www.slack.net/~ant/nes-emu/

#include "Blip_Buffer.h"

#include <string.h>
#include <math.h>

/* Library Copyright (c) 2003-2004 Shay Green. Blip_Buffer is free
software; you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.
Blip_Buffer is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details. You should have received a copy of the GNU General Public
License along with Blip_Buffer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

Blip_Buffer::Blip_Buffer() {
	factor_ = 0;
	offset_ = 0;
	buffer_ = NULL;
	buffer_size_ = 0;
	bass_level( 0.55f );
}

void Blip_Buffer::clear() {
	offset_ = 0;
	reader_accum = 0;
	memset( buffer_, sample_offset & 0xFF, (buffer_size_ + widest_impulse) * sizeof (buf_t) );
}

bool Blip_Buffer::buffer_size( unsigned max_samples )
{
	// prevent excessive buffer size
	assert( max_samples + widest_impulse <= (ULONG_MAX >> BLIP_BUFFER_ACCURACY) );
	
	delete [] buffer_;
	buffer_ = NULL;
	buffer_size_ = 0;
	offset_ = 0;
	
	buffer_ = new buf_t [max_samples + widest_impulse + 1];
	if ( !buffer_ )
		return false;
	
	buffer_size_ = (unsigned short) max_samples;
	clear();
	
	return true;
}

void Blip_Buffer::units_per_sample( double r ) {
	factor_ = (unsigned short) floor( (1L << BLIP_BUFFER_ACCURACY) / r + 0.5 );
	assert( factor_ > 0 );
}

Blip_Buffer::~Blip_Buffer() {
	delete [] buffer_;
}

unsigned Blip_Buffer::read_samples( blip_sample_t* out, unsigned max_samples, bool stereo )
{
	unsigned count = samples_avail();
	if ( count > max_samples )
		count = max_samples;
	
	int sample_offset = this->sample_offset;
	int bass_shift = this->bass_shift;
	buf_t* buf = buffer_;
	long accum = reader_accum;
	
	if ( stereo ) {
		for ( unsigned n = count; n--; ) {
			*out = accum >> accum_fract;
			out += 2;
			accum -= accum >> bass_shift;
			accum += (long (*buf++) - sample_offset) << accum_fract;
		}
	}
	else {
		for ( unsigned n = count; n--; ) {
			*out++ = accum >> accum_fract;
			accum -= accum >> bass_shift;
			accum += (long (*buf++) - sample_offset) << accum_fract;
		}
	}
	
	reader_accum = accum;
	
	remove_samples( count );
	
	return count;
}

void Blip_Buffer::remove_samples( unsigned count )
{
	assert( count <= samples_avail() ); // attempted to remove more samples than available
	
	if ( count ) // optimization
	{
		offset_ -= resampled_time_t (count) << BLIP_BUFFER_ACCURACY;
		
		// copy remaining samples to beginning and clear old samples
		unsigned remain = samples_avail() + widest_impulse;
		if ( count >= remain )
			memmove( buffer_, buffer_ + count, remain * sizeof (buf_t) );
		else
			memcpy(  buffer_, buffer_ + count, remain * sizeof (buf_t) );
		memset( buffer_ + remain, sample_offset & 0xFF, count * sizeof (buf_t) );
	}
}

int const blip_res = 1 << blip_res_bits_;

double const pi = 3.1415926535897932384626433832795029L;

static void update_fimpulse( blip_imp_t_* imp, Blip_Eq const& eq, int width, float* fimp )
{
	double const cutoff = eq.cutoff_;
	double const pt = eq.treble_point_;
	double treble = eq.treble_;
	
	assert( 0.0 <= cutoff && cutoff <= 1.0 );
	assert( cutoff < pt && 0.0 <= pt && pt <= 1.0 );
	assert( treble >= 0.0 );
	
	if ( treble == fimp [1] && cutoff == fimp [2] && pt == fimp [3] )
		return;
	
	fimp [1] = (float) treble;
	fimp [2] = (float) cutoff;
	fimp [3] = (float) pt;
	fimp += 8;
	
	float* const buf = (float*) imp; // imp is in a union with a float array
	
	if ( treble < 0.000005f )
		treble = 0.000005f;
	
	// DSF Synthesis (See T. Stilson & J. Smith (1996),
	// Alias-free digital synthesis of classic analog waveforms)
	
	// reduce adjacent impulse interference by using small part of wide impulse
	double const n_harm = 4096;
	double const rolloff = pow( treble, 1.0 / (n_harm * pt - n_harm * cutoff) );
	double const rescale = 1.0 / pow( rolloff, n_harm * cutoff );
	
	double const pow_a_n = rescale * pow( rolloff, n_harm );
	double const pow_a_nc = rescale * pow( rolloff, n_harm * cutoff );
	
	double total = 0.0;
	double const to_angle = pi / 2 / n_harm / blip_res;
	int const size = blip_res * (width - 2) / 2;
	for ( int i = size; i--; )
	{
		double angle = (i * 2 + 1) * to_angle;
		
		// equivalent to below
		//double y =     dsf( angle, n_harm * cutoff, 1.0 );
		//y -= rescale * dsf( angle, n_harm * cutoff, rolloff );
		//y += rescale * dsf( angle, n_harm,          rolloff );
		
		double const cos_angle = cos( angle );
		double const cos_nc_angle = cos( n_harm * cutoff * angle );
		double const cos_nc1_angle = cos( (n_harm * cutoff - 1.0) * angle );
		
		double b = 2.0 - 2.0 * cos_angle;
		double a = 1.0 - cos_angle - cos_nc_angle + cos_nc1_angle;
		
		double d = 1.0 + rolloff * (rolloff - 2.0 * cos_angle);
		double c = pow_a_n * rolloff * cos( (n_harm - 1.0) * angle ) -
				pow_a_n * cos( n_harm * angle ) -
				pow_a_nc * rolloff * cos_nc1_angle +
				pow_a_nc * cos_nc_angle;
		
		// optimization of a / b + c / d
		double y = (a * d + c * b) / (b * d);
		
		// fixed window which affects wider impulses more
		double window = cos( n_harm / 1.25 / Blip_Buffer::widest_impulse * angle );
		y *= window * window;
		
		total += y;
		buf [i] = float (y);
	}
	//printf( "total: %f\n", total );
	
	// integrate runs of length 'blip_res'
	double factor = 0.5 / total; // account for other mirrored half
	for ( int offset = blip_res; offset >= blip_res / 2; offset-- )
	{
		for ( int w = -width / 2; w < width / 2; w++ )
		{
			double sum = 0;
			for ( int i = blip_res; i--; )
			{
				int index = w * blip_res + offset + i;
				if ( index < 0 )
					index = -index - 1;
				if ( index < size )
					sum += buf [index];
			}
			*fimp++ = float (sum * factor);
		}
	}
}

static void scale_impulse( blip_imp_t_ unit, int width, float const* fimp,
		blip_imp_t_* imp_in )
{
	double famp = unit;
	blip_imp_t_* imp = imp_in;
	for ( int n = blip_res / 2 + 1; n--; )
	{
		long error = unit;
		for ( int nn = width; nn--; )
		{
			long a = (long) floor( *fimp++ * famp + 0.5 );
			error -= a;
			*imp++ = blip_imp_t_ (a + unit);
			//printf( "%d, ", int (a) );
		}
		//if ( error < -2 || 2 < error )
		//	printf( " error = %d\n", int (error) );
		
		// distribute error over middle pair
		imp [-width / 2 - 2] += error / 2;
		imp [-width / 2 - 1] += error - error / 2;
	}
	
	// second half is mirror-image
	blip_imp_t_ const* rev = imp - width - 1;
	for ( int nn = (blip_res / 2 - 1) * width - 1; nn--; )
		*imp++ = *--rev;
	*imp++ = unit;

	// copy to odd offset
	*imp++ = unit;
	memcpy( imp, imp_in, (blip_res * width - 1) * sizeof *imp );
}

static Blip_Eq const default_eq( 0.36f, 0.40f );

void blip_treble_eq( blip_imp_t_* imp, Blip_Eq const& eq, int width, float* fimp ) {
	update_fimpulse( imp, eq, width, fimp );
	double volume = fimp [0];
	if ( volume >= 0 ) {
		fimp [0] = -1;
		blip_volume_unit( imp, volume, width, fimp );
	}
}
	
BOOST::uint32_t blip_volume_unit( blip_imp_t_* imp, double funit, int width, float* fimp ) {
	blip_imp_t_ unit = blip_imp_t_ (funit * 0xffff + 0.5);
	if( fimp [0] != funit ) {
		fimp [0] = (float) funit;
		if ( fimp [1] < 0 )
			update_fimpulse( imp, default_eq, width, fimp );
		scale_impulse( unit, width, fimp + 8, imp );
	}
	return unit * 0x10000u + unit;
}

void blip_fine_treble_eq( blip_imp_t_* imp, Blip_Eq const& eq, int width, float* fimp ) {
	update_fimpulse( imp, eq, width, fimp );
	double volume = fimp [0];
	if ( volume >= 0 ) {
		fimp [0] = -1;
		blip_fine_volume_unit( imp, volume, width, fimp );
	}
}
	
BOOST::uint32_t blip_fine_volume_unit( blip_imp_t_* imp, double funit, int width, float* fimp ) {
	blip_imp_t_ unit = blip_imp_t_ (funit * 0xffff + 0.5);
	if( fimp [0] != funit ) {
		fimp [0] = (float) funit;
		if ( fimp [1] < 0 )
			update_fimpulse( imp, default_eq, width, fimp );
		
		int const fine_step = 256;
		
		// TODO: find more elegant way to handle this
		blip_imp_t_ temp [blip_res * 2 * Blip_Buffer::widest_impulse];
		scale_impulse( unit * fine_step, width, fimp + 8, temp );
		blip_imp_t_* imp2 = imp + blip_res * 2 * width;
		scale_impulse( unit, width, fimp + 8, imp2 );
		
		// merge impulses
		blip_imp_t_* src2 = temp;
		for ( int n = blip_res / 2 * width * 2; n--; ) {
			*imp++ = *imp2++;
			*imp++ = *imp2++;
			*imp++ = *src2++;
			*imp++ = *src2++;
		}
	}
	return unit * 0x10000u + unit;
}

