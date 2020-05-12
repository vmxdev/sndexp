#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static const double R = 44100;        /* sample rate (samples per second) */
static const double maxamp = 1.0;     /* max. amplitude */

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
calc_tone(double t, int note)
{
	double tone;

	%%CODE_INSTR%%

	return tone;
}

static void
add_note(struct timeline *tl, double start,
	int note, double duration, double loudness)
{
	size_t tstart, tend, t;

	tstart = start * R;
	tend = tstart + duration * R;

	for (t=tstart; t<tend; t++) {
		double tone;
		double k;

		k = (t - tstart) / R;

		tone = calc_tone(k, note);

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
	size_t i;

	struct timeline *tl;

	tl = timeline_init(100 * R);
	if (!tl) {
		return EXIT_FAILURE;
	}

	where = 0.0;

	%%CODE_NOTES%%

	timeline_play(tl);

	timeline_free(tl);

	return EXIT_SUCCESS;
}

