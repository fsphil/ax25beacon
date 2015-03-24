
/* Copyright 2015 Philip Heron <phil@sanslogic.co.uk>                    */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ao/ao.h>
#include "ax25.h"

void _usage(void)
{
	fprintf(stderr,
		"\n"
		"Usage: ax25frame -s CALLSIGN[-NN] -d CALLSIGN[-NN] [-p PATH[-TTL]] [-r SAMPLERATE] [-o OUTPUT.WAV] DATA\n"
		"\n"
		"   -s CALLSIGN[-NN]   Sender callsign and optional SSID\n"
		"   -d CALLSIGN[-NN]   Destination callsign and optional SSID\n"
		"   -p PATH[-TTL]      Add a path with optional TTL.\n"
		"                      Up to two paths can be specified.\n"
		"   -r SAMPLERATE      The sample rate to use. Defaults to 48000Hz.\n"
		"   -o OUTPUT.WAV      Output the audio to the specified WAV file.\n"
		"                      Defaults to the main audio device.\n"
		"   DATA               The packet contents.\n"
		"\n"
	);
	
	exit(-1);
}

void _die(char *message)
{
	fprintf(stderr, "%s", message);
	_usage();
}

void audio_callback(void *data, int16_t *wav, size_t wav_len)
{
	ao_device *ao = data;
	ao_play(ao, (char *) wav, wav_len * sizeof(int16_t));
}

int main(int argc, char *argv[])
{
	int x;
	ao_device *ao_out;
	ao_sample_format ao_format;
	ax25_t ax25;
	char *wavfile = NULL;
	char *src_callsign = NULL;
	char *dst_callsign = NULL;
	char *path1 = NULL;
	char *path2 = NULL;
	
	ax25_init(&ax25);
	
	while((x = getopt(argc, argv, "s:d:p:r:o:")) != -1)
	{
		switch(x)
		{
		case 's': /* Sender callsign */
			if(src_callsign) _die("Only one sender callsign can be used\n");
			src_callsign = strdup(optarg);
			break;
		case 'd': /* Destination callsign */
			if(dst_callsign) _die("Only one destination callsign can be used\n");
			dst_callsign = strdup(optarg);
			break;
		case 'p': /* Add a path */
			if(!path1) path1 = strdup(optarg);
			else if(!path2) path2 = strdup(optarg);
			else _die("Error: More than 2 paths specified\n");
			break;
		case 'r': /* Sample Rate */
			ax25.samplerate = atoi(optarg);
			break;
		case 'o': /* Output WAV filename */
			if(wavfile) _die("Only one output WAV file can be used\n");
			wavfile = strdup(optarg);
			break;
		case '?': _usage();
		}
	}
	
	if(!src_callsign) _die("No sender callsign specified\n");
	if(!dst_callsign) _die("No destination callsign specified\n");
	
	x = argc - optind;
	if(x != 1) _usage();
	
	/* Setup AO */
	ao_initialize();
	
	/* Setup the output format */
	memset(&ao_format, 0, sizeof(ao_format));
	ao_format.bits = 16;
	ao_format.channels = 1;
	ao_format.rate = ax25.samplerate;
	ao_format.byte_format = AO_FMT_NATIVE;
	
	/* Open either the output audio device or WAV file */
	if(wavfile)
	{
		/* A wav file has been specified */
		x = ao_driver_id("wav");
		ao_out = ao_open_file(x, wavfile, 1, &ao_format, NULL);
	}
	else
	{
		/* Output to the default sound device */
		x = ao_default_driver_id();
		ao_out = ao_open_live(x, &ao_format, NULL);
	}
	
	/* Was the driver opened OK? */
	if(!ao_out)
	{
		fprintf(stderr, "Error opening output\n");
		return(-1);
	}
	
	/* Set the audio callback to our AO player */
	ax25_set_audio_callback(&ax25, &audio_callback, (void *) ao_out);
	
	/* Warn if the sample rate doesn't divide cleanly into the bit rate */
	if(ax25.samplerate % ax25.bitrate != 0) printf("Warning: The sample rate %d does not divide evently into %d. The bit rate will be %.2f\n", ax25.samplerate, ax25.bitrate, (float) ax25.samplerate / (ax25.samplerate / ax25.bitrate));
	
	/* Generate the packet audio */
	ax25_frame(
		&ax25,
		src_callsign, dst_callsign,
		path1, path2,
		"%s", argv[optind]
	);
	
	/* Close and shutdown AO */
	ao_close(ao_out);
	ao_shutdown();
	
	return(0);
}
