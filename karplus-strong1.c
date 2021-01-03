/*
 * $ cc -g -Wall -pedantic -Wextra karplus-strong1.c  -o karplus-strong1 -lm
 * $ ./karplus-strong1 | aplay -f S16_LE -r 44100 -c 2
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


static void
karplus_strong(struct timeline *tl, double start, double fr,
	double duration, double loudness)
{
	size_t i, c;
	double prev;
	size_t init_len = R / fr;
	double *init_waveform;
	size_t tstart, tend, t;

	init_waveform = malloc(sizeof(double) * init_len);

	for (i=0; i<init_len; i++) {
		init_waveform[i] = (double)rand() / (double)RAND_MAX;
	}

	tstart = start * R;
	tend = tstart + duration * R;

	c = 0;
	prev = 0.0f;
	for (t=tstart; t<tend; t++) {
		double tone;

		init_waveform[c] = ((init_waveform[c] + prev) / 2.0) * 0.99f;
		tone = init_waveform[c];
		prev = tone;
		c = (c + 1) % init_len;

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

	free(init_waveform);
}

static void
karplus_major_chord(struct timeline *tl, double start, double fr,
	double duration, double loudness)
{
	karplus_strong(tl, start, fr, duration, loudness);
	karplus_strong(tl, start, fr * 1.26f, duration, loudness);
	karplus_strong(tl, start, fr * 1.26f * 1.19f, duration, loudness);
	karplus_strong(tl, start, fr * 2.0f, duration, loudness);
}

int
main()
{
	int bpm = 190;

	double vol = 0.5;
	double notelen = 60.0 / bpm / 4.0;
	double where;
	double fr;
	int i;

	struct timeline *tl;

	tl = timeline_init(100 * R);
	if (!tl) {
		return EXIT_FAILURE;
	}

	where = 0.0;
	fr = 261.63f;

	for (i=0; i<8; i++) {
		karplus_major_chord(tl, where, fr, notelen * 1, vol);
		where += notelen * 1;
	}

	for (i=0; i<8; i++) {
		karplus_major_chord(tl, where, fr * 1.33f, notelen * 1, vol);
		where += notelen * 1;
	}

	for (i=0; i<8; i++) {
		karplus_major_chord(tl, where, fr * 1.33f * 1.33f,
			notelen * 1, vol);
		where += notelen * 1;
	}

	for (i=0; i<8; i++) {
		karplus_major_chord(tl, where, fr, notelen * 1, vol);
		where += notelen * 1;
	}


	timeline_play(tl);

	timeline_free(tl);

	return EXIT_SUCCESS;
}

