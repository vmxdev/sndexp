#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static const double R = 44100;        /* sample rate (samples per second) */
static const double maxamp = 1.0;     /* max. amplitude */


typedef double (*instr_func)(double t, int note);

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
instr_kick(double t, int note)
{
	double tone = exp(-t*4) * sin(2*M_PI*t*freq(note) * exp(-t*20));

	return tone;
}

static double
instr_metal(double t, int note)
{
	double tone = exp(-t*4) * (sin(2*M_PI*t*freq(note))
		+ sin(2*M_PI*t*freq(note)*2.13232)
		+ sin(2*M_PI*t*freq(note)*6.12342));

	return tone;
}

static double
instr_tom(double t, int note)
{
	double tone = exp(-t*4) * sin(2*M_PI*t*freq(note) * exp(-t*20))
		+ exp(-t*2) * sin(2*M_PI*t*freq(note) * exp(-t*5));

	return tone;
}

static double
instr_cym(double t, int note)
{
	double tone = exp(-t*70) * tan(2*M_PI*t*freq(note) * exp(-t*70));
	return tone;
}

static double
instr_cym2(double t, int note)
{
	double tone = exp(-t*10) * sin(2*M_PI*t*freq(note) * exp(-t*10));
	return tone;
}

static double
instr_piano(double t, int note)
{
	double tone;

	t *= 2.0 * M_PI * freq(note);

	tone = sin(t) * exp(-0.0004 * t);
                                                                
	tone += sin(2.0 * t) * exp(-0.0004 * t) / 2.0;
	tone += sin(3.0 * t) * exp(-0.0004 * t) / 4.0;
	tone += sin(4.0 * t) * exp(-0.0004 * t) / 8.0;
	tone += sin(5.0 * t) * exp(-0.0004 * t) / 16.0;
	tone += sin(6.0 * t) * exp(-0.0004 * t) / 32.0;
                                                                
	tone += tone * tone * tone;
	return tone;
}

static void
add_note(struct timeline *tl, double start,
	int note, double duration, double loudness, instr_func instr)
{
	size_t tstart, tend, t;

	tstart = start * R;
	tend = tstart + duration * R;

	for (t=tstart; t<tend; t++) {
		double tone;
		double k;

		k = (t - tstart) / R;

		tone = (*instr)(k, note);

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


int
main()
{
	int bpm = 190;

	double vol = 0.5;
	double notelen = 60.0 / bpm / 4.0;
	double where = 0.0;
	size_t i, j, k;

	struct timeline *tl;

	tl = timeline_init(100 * R);
	if (!tl) {
		return EXIT_FAILURE;
	}

	/* drums */
	where = 0.0;

	for (k=0; k<4; k++) {
		for (i=0; i<3; i++) {
			add_note(tl, where, 800, notelen * 8,
				vol * 0.15, &instr_cym2);
			add_note(tl, where + notelen, 90, notelen * 8,
				vol * 0.01, &instr_metal);

			for (j=0; j<4; j++) {
				add_note(tl, where, 30, notelen * 2,
					vol, &instr_kick);
				where += notelen * 1;

				add_note(tl, where, 30, notelen * 2,
					vol, &instr_kick);
				where += notelen * 1;

				add_note(tl, where, 30, notelen * 2,
					vol, &instr_kick);

				add_note(tl, where, 30, notelen * 2,
					vol, &instr_tom);
				add_note(tl, where, 700, notelen * 2,
					vol * 0.02, &instr_cym);

				where += notelen * 2;
			}
		}

		for (i=0; i<4; i++) {
			add_note(tl, where, 800, notelen * 8,
				vol * 0.15, &instr_cym2);
			add_note(tl, where + notelen, 110, notelen * 8,
				vol * 0.008, &instr_metal);

			for (j=0; j<4; j++) {
				add_note(tl, where, 40 - i * 3, notelen * 2,
					vol * 0.2, &instr_tom);
				add_note(tl, where, 800, notelen * 2,
					vol * 0.01, &instr_cym);
				where += notelen * 1;
			}
		}
	}

	/* piano */
	where = 0.0;

	for (j=0; j<2; j++) {
		for (k=0; k<4; k++) {
			for (i=0; i<4; i++) {
				add_note(tl, where, 40, notelen * 6,
					vol * 0.03, &instr_piano);
				add_note(tl, where, 44, notelen * 6,
					vol * 0.03, &instr_piano);
				add_note(tl, where, 47, notelen * 6,
					vol * 0.03, &instr_piano);
				add_note(tl, where, 52, notelen * 6,
					vol * 0.03, &instr_piano);

				where += notelen * 2;
			}

			where += notelen * 8;
		}

		for (k=0; k<4; k++) {
			for (i=0; i<4; i++) {
				add_note(tl, where, 37 + 0, notelen * 6,
					vol * 0.03, &instr_piano);
				add_note(tl, where, 37 + 4, notelen * 6,
					vol * 0.03, &instr_piano);
				add_note(tl, where, 37 + 7, notelen * 6,
					vol * 0.03, &instr_piano);
				add_note(tl, where, 37 + 12, notelen * 6,
					vol * 0.03, &instr_piano);

				where += notelen * 2;
			}

			where += notelen * 8;
		}
	}

	/* bass */
	where = 0.0;

	for (j=0; j<2; j++) {
		for (i=0; i<15; i++) {
			add_note(tl, where, 40 - 12*2, notelen * 2,
				vol * 0.2, &instr_piano);
			where += notelen * 2;

			add_note(tl, where, 40 - 7 - 12*2, notelen * 2,
				vol * 0.2, &instr_piano);
			where += notelen * 2;
		}
		add_note(tl, where, 40 - 12*2, notelen * 2,
			vol * 0.2, &instr_piano);
		where += notelen * 2;

		add_note(tl, where, 40 - 1 - 12*2, notelen * 2,
			vol * 0.2, &instr_piano);
		where += notelen * 2;


		for (i=0; i<15; i++) {
			add_note(tl, where, 37 - 12*2, notelen * 2,
				vol * 0.2, &instr_piano);
			where += notelen * 2;

			add_note(tl, where, 37 - 7 - 12*2, notelen * 2,
				vol * 0.2, &instr_piano);
			where += notelen * 2;
		}
		add_note(tl, where, 37 - 12*2, notelen * 2,
			vol * 0.2, &instr_piano);
		where += notelen * 2;

		add_note(tl, where, 37 + 2 - 12*2, notelen * 2,
			vol * 0.2, &instr_piano);
		where += notelen * 2;
	}

	timeline_play(tl);

	timeline_free(tl);

	return EXIT_SUCCESS;
}

