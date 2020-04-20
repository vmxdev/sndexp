/*
 * $ cc -O3 -Wall -pedantic -Wextra anthem.c -o anthem -lm
 *
 * to play sound:
 * $ ./anthem | aplay -f S16_LE -r 44100 -c 2
 *
 * to save as .wav file:
 * generate .pcm first
 * $ ./anthem > sound.pcm
 * and convert using ffmpeg
 * $ ffmpeg -f s16le -ar 44.1k -ac 2 -i sound.pcm sound.wav
 *
 * to make video from single image
 * $ ffmpeg -loop 1 -i image.png -i sound.wav -c:v libx264 -tune stillimage \
 *     -c:a aac -b:a 192k -pix_fmt yuv420p -shortest sound.mp4
 */
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static const double R = 44100;        /* sample rate (samples per second) */
static const double maxamp = 1.0;     /* max. amplitude */

static double freq(int n)
{
	double f;

	f = pow(2.0, ((double)n - 49.0) / 12.0) * 440.0;
	return f;
}


static void
play_note(int note, double duration, double loudness)
{
	int t, tmax;

	tmax = duration * R;
	for (t=0; t<tmax; t++) {
		double tone;
		int16_t itone;

		tone = sin(t * 2 * M_PI / R * freq(note));

		tone *= loudness;

		if (tone > maxamp) {
			tone = maxamp;
		}
		if (tone < -maxamp) {
			tone = -maxamp;
		}

		 /* first channel */
		itone = tone * INT16_MAX;
		fwrite(&itone, sizeof(int16_t), 1, stdout);

		/* second channel */
		itone = tone * INT16_MAX;
		fwrite(&itone, sizeof(int16_t), 1, stdout);
	}
}

static void
play_major_chord(int startnote, double duration, double loudness)
{
	int t, tmax;

	tmax = duration * R;
	for (t=0; t<tmax; t++) {
		double tone1, tone2, tone3, tone4;
		double tone;
		int16_t itone;

		tone1 = sin(t * 2 * M_PI / R * freq(startnote + 0));
		tone2 = sin(t * 2 * M_PI / R * freq(startnote + 4));
		tone3 = sin(t * 2 * M_PI / R * freq(startnote + 7));
		tone4 = sin(t * 2 * M_PI / R * freq(startnote + 12));

		tone = (tone1 + tone2 + tone3 + tone4) / 4.0 * loudness;

		if (tone > maxamp) {
			tone = maxamp;
		}
		if (tone < -maxamp) {
			tone = -maxamp;
		}

		 /* first channel */
		itone = tone * INT16_MAX;
		fwrite(&itone, sizeof(int16_t), 1, stdout);

		/* second channel */
		itone = tone * INT16_MAX;
		fwrite(&itone, sizeof(int16_t), 1, stdout);
	}
}


int
main()
{
	double vol = 0.5;
	double notelen = 0.15;

	play_note(40, notelen * 1, vol);
	play_major_chord(45, notelen * 3, vol);

	play_note(40, notelen * 2, vol);
	play_note(42, notelen * 1, vol);
	play_major_chord(44, notelen * 3, vol);
	play_note(37, notelen * 2, vol);

	/* short pause */
	play_note(37, 0.01, vol);

	play_note(37, notelen * 1, vol);
	play_major_chord(42, notelen * 3, vol);

	play_note(40, notelen * 2, vol);
	play_note(38, notelen * 1, vol);
	play_major_chord(40, notelen * 3, vol);
	play_note(33, notelen * 2, vol);

	play_note(0, 0.01, vol);

	play_note(33, notelen * 1, vol);
	play_major_chord(35, notelen * 3, vol);

	play_note(35, notelen * 2, vol);
	play_note(37, notelen * 1, vol);
	play_major_chord(38, notelen * 3, vol);
	play_note(38, notelen * 2, vol);

	play_note(0, 0.01, vol);

	play_note(40, notelen * 1, vol);
	play_major_chord(42, notelen * 3, vol);

	play_note(44, notelen * 2, vol);
	play_note(45, notelen * 1, vol);
	play_major_chord(47, notelen * 4, vol);

	return 0;
}

