/*
 * $ cc -g -Wall -pedantic -Wextra random-rhythm.c  -o random-rhythm -lm
 * $ ./random-rhythm | aplay -f S16_LE -r 44100 -c 2
 * or
 * $ ./random-rhythm <SEED> | aplay -f S16_LE -r 44100 -c 2
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define L 12

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

static double
instr_cym(double t, double fr, double overdrive)
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

		init_waveform[c] = ((init_waveform[c] + prev) / 2.0) * 0.9999f;
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

int
main(int argc, char *argv[])
{
	int bpm = 100;

	double vol = 0.5;
	double notelen = 60.0 / bpm / 4.0;
	double where;
	int i, j;
	int seed;

	struct timeline *tl;
	double notes[L];
	int play_notes[L];

	if (argc == 1) {
		seed = time(NULL);
	} else {
		seed = atoi(argv[1]);
	}
	fprintf(stderr, "seed: %d\n", seed);
	srand(seed);

	tl = timeline_init(100 * R);
	if (!tl) {
		return EXIT_FAILURE;
	}


	for (i=0; i<L; i++) {
		int d; 
		notes[i] = rand() % 3000 + 300;

		d = rand() % 2;
		if (d) {
			play_notes[i] = 1;
		} else {
			play_notes[i] = 0;
		}
	}

	/* tom */
#if 1
	where = 0.0;
	where += (notelen * 6) * 2/* + notelen*/;

	for (i=0; i<7; i++) {
		for (j=0; j<L; j++) {
			if (play_notes[j] && (notes[j] > ((3000 + 300) / 2))) {
				add_note(tl, where, 250, notelen * 0.5, vol * 0.3, 1.0, &instr_tom);
			}
			where += notelen * 1;
		}
	}

	for (i=0; i<7; i++) {
		for (j=0; j<L; j++) {
			if (play_notes[j] && (notes[j] > ((3000 + 300) / 8))) {
				add_note(tl, where, 250, notelen * 0.5, vol * 0.3, 1.0, &instr_tom);
			}
			where += notelen * 1;
		}
	}
#endif

	/* cymbals */
#if 1
	where = 0.0;
	where += (notelen * 6) * 2/* + notelen*/;

	for (i=0; i<14*L; i++) {
		add_note(tl, where, 10e9, notelen * 0.2, vol * 0.07, 1.0, &instr_cym);
		where += notelen * 1;
	}

	where = 0.0;
	where += (notelen * 6) * 2 + notelen;
	for (i=0; i<7*L; i++) {
		add_note(tl, where, 10e7, notelen * 0.3, vol * 0.1, 1.0, &instr_cym);
		where += notelen * 2;
	}
#endif

	where = 0.0;
	for (i=0; i<16; i++) {
		for (j=0; j<L; j++) {
			if (play_notes[j]) {
				karplus_strong(tl, where, notes[j], notelen * 1, vol * 1.7);
			}
			where += notelen * 1;
		}
	}

	timeline_play(tl);

	timeline_free(tl);

	return EXIT_SUCCESS;
}

