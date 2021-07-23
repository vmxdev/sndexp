/*
 * $ cc -Wall -pedantic -Wextra random-cons8.c -o random-cons8 -lm
 * $ ./random-cons8 17 | aplay -f S16_LE -r 44100 -c 2
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>


static const double R = 44100;        /* sample rate (samples per second) */
static const double maxamp = 1.0;     /* max. amplitude */

typedef double (*instr_func)(double t, double fr);

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
freq(int n)
{
	double f;

	f = pow(2.0, ((double)n - 49.0) / 12.0) * 440.0;
	return f;
}


static double
instr_piano(double t, double fr)
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
	return tone;
}


static double
instr_sin(double t, double fr)
{
	double tone;

	t *= 2.0 * M_PI * fr;

	tone = sin(t);
                                                                
	return tone;
}

static void
add_note(struct timeline *tl, double start,
	double fr, double duration, double loudness, instr_func instr)
{
	size_t tstart, tend, t;

	tstart = start * R;
	tend = tstart + duration * R;

	for (t=tstart; t<tend; t++) {
		double tone;
		double k;

		k = (t - tstart) / R;

		tone = (*instr)(k, fr);

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

#define NHARM 12
#define SLEN 8
#define NOTESCHANGE 4

struct sample_n
{
	double fr;
	double len;
};

static void
sample_init(struct sample_n *sample, double *ks, size_t size, double notelen)
{
	size_t i;
	int harm;
	double fr = 440.0;

	for (i=0; i<size; i++) {
		double len;

		if ((rand() % 2) == 1) {
			harm = rand() % NHARM;
			fr *= ks[harm];
		} else {
			harm = rand() % NHARM;
			fr /= ks[harm];
		}
		len = notelen * (rand() % 8);

		sample[i].fr = fr;
		sample[i].len = len;
	}
}

int
main(int argc, char *argv[])
{
	int bpm = 144;

	double vol = 0.5;
	double notelen = 60.0 / bpm / 4.0;
	double where;
	double fr;
	double ks[NHARM] = {1.0,
		2.0/3.0, 3.0/2.0,
		3.0/4.0, 4.0/3.0,
		4.0/5.0, 5.0/4.0,
		5.0/6.0, 6.0/5.0,
		6.0/7.0, 7.0/6.0,
		2.0};
	int i;
	struct sample_n samples[SLEN];
	int harm;

	struct timeline *tl;

	tl = timeline_init(100 * R);
	if (!tl) {
		return EXIT_FAILURE;
	}

	where = 0.0;

	fr = 440.0;

	srand(atoi(argv[1]));
	sample_init(samples, ks, SLEN, notelen);

	for (i=0; i<SLEN; i++) {
		add_note(tl, where,
			samples[i].fr,
			samples[i].len,
			vol * 0.5, &instr_piano);
		where += samples[i].len;

		fprintf(stderr, "len: %f, freq: %f\n",
			samples[i].len,
			samples[i].fr);
	}
	/* pause */
	add_note(tl, where, 0, 0.2, 0.0, &instr_piano);
	where += 0.2;

	for (i=0; i<NOTESCHANGE; i++) {
		int n;

		n = rand() % SLEN;
		if ((rand() % 2) == 1) {
			harm = rand() % NHARM;
			samples[n].fr *= ks[harm];
		} else {
			harm = rand() % NHARM;
			samples[n].fr /= ks[harm];
		}
	}


	for (i=0; i<SLEN; i++) {
		add_note(tl, where, samples[i].fr, samples[i].len, vol * 0.5,
			&instr_piano);
		where += samples[i].len;
	}
	/* pause */
	add_note(tl, where, 0, 0.2, 0.0, &instr_piano);
	where += 0.2;

	for (i=0; i<NOTESCHANGE; i++) {
		int n;

		n = rand() % SLEN;
		if ((rand() % 2) == 1) {
			harm = rand() % NHARM;
			samples[n].fr *= ks[harm];
		} else {
			harm = rand() % NHARM;
			samples[n].fr /= ks[harm];
		}
	}


	for (i=0; i<SLEN; i++) {
		add_note(tl, where, samples[i].fr, samples[i].len, vol * 0.5,
			&instr_piano);
		where += samples[i].len;
	}
	/* pause */
	add_note(tl, where, 0, 0.2, 0.0, &instr_piano);
	where += 0.2;

	for (i=0; i<NOTESCHANGE; i++) {
		int n;

		n = rand() % SLEN;
		if ((rand() % 2) == 1) {
			harm = rand() % NHARM;
			samples[n].fr *= ks[harm];
		} else {
			harm = rand() % NHARM;
			samples[n].fr /= ks[harm];
		}
	}


	for (i=0; i<SLEN; i++) {
		add_note(tl, where, samples[i].fr, samples[i].len, vol * 0.5,
			&instr_piano);
		where += samples[i].len;
	}

	timeline_play(tl);

	timeline_free(tl);

	return EXIT_SUCCESS;
}

