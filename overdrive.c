/*
 * $ cc -Wall -pedantic -Wextra overdrive.c -o overdrive -lm
 * $ ./overdrive 12345 | aplay -f S16_LE -r 44100 -c 2
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>


static const double R = 44100;        /* sample rate (samples per second) */
static const double maxamp = 1.0;     /* max. amplitude */

typedef double (*instr_func)(double t, double fr, double overdrive);

struct timeline
{
	size_t n;
	size_t end;

	float *left, *right;
	float min, max;
};

static struct timeline *
timeline_init(size_t n)
{
	struct timeline *t;

	t = malloc(sizeof(struct timeline));
	if (!t) {
		goto tmalloc_failed;
	}

	t->left = calloc(n, sizeof(float));
	if (!t->left) {
		goto left_malloc_failed;
	}

	t->right = calloc(n, sizeof(float));
	if (!t->right) {
		goto right_malloc_failed;
	}

	t->n = n;
	t->end = 0;

	return t;

right_malloc_failed:
	free(t->left);

left_malloc_failed:
	free(t);

tmalloc_failed:
	return NULL;
}

static void
timeline_free(struct timeline *tl)
{
	free(tl->left);
	free(tl->right);

	tl->left = tl->right = NULL;
	tl->n = tl->end = 0;

	free(tl);
}

static void
timeline_play(struct timeline *tl)
{
	size_t t;
	for (t=0; t<tl->end; t++) {
		int16_t tone;

		if (tl->left[t] > maxamp) {
			tone = INT16_MAX;
		} else if (tl->left[t] < -maxamp) {
			tone = -INT16_MAX;
		} else {
			tone = tl->left[t] * INT16_MAX;
		}
		fwrite(&tone, sizeof(int16_t), 1, stdout);

		if (tl->right[t] > maxamp) {
			tone = INT16_MAX;
		} else if (tl->right[t] < -maxamp) {
			tone = -INT16_MAX;
		} else {
			tone = tl->right[t] * INT16_MAX;
		}
		fwrite(&tone, sizeof(int16_t), 1, stdout);
	}
}


static double
instr_piano(double t, double fr, double overdrive)
{
	double tone;

	t *= 2.0 * M_PI * fr;

	tone = sin(t) * exp(-0.0004 * t);
                                                                
	tone += sin(2.0 * t) * exp(-0.0004 * t) / 2.0;
	tone += sin(3.0 * t) * exp(-0.0004 * t) / 4.0;
	tone += sin(4.0 * t) * exp(-0.0004 * t) / 8.0;
	tone += sin(5.0 * t) * exp(-0.0004 * t) / 16.0;
	tone += sin(6.0 * t) * exp(-0.0004 * t) / 32.0;
 
	tone += tone * tone * tone;

	if (tone >= overdrive) {
		tone = overdrive;
	} else if (tone <= -overdrive) {
		tone = -overdrive;
	}
	return tone;
}

static double
instr_cym2(double t, double fr, double overdrive)
{
	double tone;
	(void)overdrive;

	tone = exp(-t*8) *
	(
		sin(2 * M_PI * t * fr * exp(-t*8))
		+ sin(2 * M_PI * t * fr) * 0.9f
	);
	return tone;
}

static double
instr_tom(double t, double fr, double overdrive)
{
	double tone;
	(void)overdrive;

	tone = exp(-t*4) * sin(2*M_PI*t*fr * exp(-t*20))
		+ exp(-t*2) * sin(2*M_PI*t*fr * exp(-t*5));

	return tone;
}

static void
add_note(struct timeline *tl, double start,
	double fr, double duration, double loudness, double overdrive,
	instr_func instr)
{
	size_t tstart, tend, t;

	tstart = start * R;
	tend = tstart + duration * R;

	for (t=tstart; t<tend; t++) {
		double tone;
		double k;

		k = (t - tstart) / R;

		tone = (*instr)(k, fr, overdrive);

		tone *= loudness;

		if (t >= tl->n) {
			break;
		}
		 /* first channel */
		tl->left[t] += tone;

		/* second channel */
		tl->right[t] += tone;
	}

	if (t > tl->end) {
		tl->end = t;
	}
}

static void
add_major_chord(struct timeline *tl, double where, double fr, double notelen,
	double vol)
{
	double k1 = 5.0/4.0;
	double k2 = 3.0/2.0;
	double k3 = 2.0;
	double dist = 0.1;

	add_note(tl, where, fr,      notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k1, notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k2, notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k3, notelen, vol, dist, &instr_piano);

	fr *= 0.99998;
	add_note(tl, where, fr,      notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k1, notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k2, notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k3, notelen, vol, dist, &instr_piano);

	fr *= 1.00001;
	add_note(tl, where, fr,      notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k1, notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k2, notelen, vol, dist, &instr_piano);
	add_note(tl, where, fr * k3, notelen, vol, dist, &instr_piano);
}

int
main()
{
	int bpm = 144;

	double vol = 0.7;
	double notelen = 60.0 / bpm / 4.0;
	double where, where_cym;
	double fr;
	double frr;
	double speedup = 0.99;
	int i, j;

	struct timeline *tl;

	tl = timeline_init(100 * R);
	if (!tl) {
		return EXIT_FAILURE;
	}

	where = 0.0;

	{
		fr = 440.0 / 4;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 3;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 6;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 3;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 6;

		for (i=0; i<8; i++) {
			add_major_chord(tl, where, fr, notelen * 1.5, vol);
			where += notelen * 2;
			fr *= 1.03;
		}
	}
	where_cym = where;

	{
		fr = 440.0 / 4;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 3;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 6;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 3;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 6;

		for (i=0; i<8; i++) {
			add_major_chord(tl, where, fr, notelen * 1.5, vol);
			where += notelen * 2;
			fr *= 1.03;
		}
	}

	frr = 440.0 / 4;
	for (j=0; j<8; j++) {
		fr = 440.0 / 4;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 3;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 6;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 3;

		add_major_chord(tl, where, fr, notelen * 2, vol);
		where += notelen * 6;

		for (i=0; i<8; i++) {
			add_major_chord(tl, where, frr, notelen * 1.5, vol);
			where += notelen * 2;

			frr /= 1.01;
		}

		notelen *= speedup;
	}

	/* drums */
	notelen = 60.0 / bpm / 4.0;
	where_cym += notelen * 2;
	{
		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		for (i=0; i<4; i++) {
			add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
			where_cym += notelen * 4;
		}
	}

	for (j=0; j<8; j++) {
		double tom_freq = 300;

		add_note(tl, where_cym - notelen * 2, tom_freq, notelen * 3, vol, 100.0, &instr_tom);
		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		add_note(tl, where_cym - notelen * 2, tom_freq, notelen * 3, vol, 100.0, &instr_tom);
		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		add_note(tl, where_cym - notelen * 2, tom_freq, notelen * 3, vol, 100.0, &instr_tom);
		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		add_note(tl, where_cym - notelen * 2, tom_freq, notelen * 3, vol, 100.0, &instr_tom);
		add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
		where_cym += notelen * 4.5;

		for (i=0; i<4; i++) {
			add_note(tl, where_cym - notelen * 2, tom_freq, notelen * 3, vol, 100.0, &instr_tom);
			add_note(tl, where_cym, 10e7, notelen * 1, vol, 100.0, &instr_cym2);
			where_cym += notelen * 4;
		}
		notelen *= speedup;
	}

	timeline_play(tl);

	timeline_free(tl);

	return EXIT_SUCCESS;
}

