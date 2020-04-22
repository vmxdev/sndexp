#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static const double R = 44100;        /* sample rate (samples per second) */
static const double maxamp = 1.0;     /* max. amplitude */

#define RE_2   (42 - 12)
#define MI_2   (44 - 12)
#define FA_2   (45 - 12)
#define SOL_2  (47 - 12)
#define LA_2   (49 - 12)
#define SI_2   (51 - 12)

#define DO_3        40
#define RE_3        42
#define MI_3        44
#define FA_3        45
#define FA_DIES_3   46
#define SOL_3       47
#define LA_3        49
#define SI_3        51

#define DO_4   (DO_3 + 12)
#define RE_4   (RE_3 + 12)
#define MI_4   (MI_3 + 12)
#define FA_4   (FA_3 + 12)
#define SOL_4  (SOL_3 + 12)

#define DO_3_BASS  (MI_2 - 12*1)
#define RE_3_BASS  (FA_2 - 12*1)
#define MI_3_BASS  (SOL_2 - 12*1)
#define FA_3_BASS  (LA_2  - 12*1)
#define SOL_3_BASS (SI_2  - 12*1)
#define LA_3_BASS  (DO_3  - 12*1)
#define SI_3_BASS  (RE_3  - 12*1)
#define DO_4_BASS  (MI_3  - 12*1)
#define RE_4_BASS  (FA_3  - 12*1)

#define RE_DIES_4_BASS  (FA_DIES_3  - 12*1)

#define MI_4_BASS  (SOL_3 - 12*1)
#define FA_4_BASS  (LA_3  - 12*1)
#define SOL_4_BASS (SI_3  - 12*1)
#define LA_4_BASS  (DO_4  - 12*1)
#define SI_4_BASS  (RE_4  - 12*1)
#define DO_5_BASS  (MI_4  - 12*1)
#define RE_5_BASS  (FA_4  - 12*1)

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

		k = 2.0 * M_PI / R * freq(note) * (t - tstart);

		/* piano */
		tone = sin(k) * exp(-0.0004 * k);

		tone += sin(2.0 * k) * exp(-0.0004 * k) / 2.0;
		tone += sin(3.0 * k) * exp(-0.0004 * k) / 4.0;
		tone += sin(4.0 * k) * exp(-0.0004 * k) / 8.0;
		tone += sin(5.0 * k) * exp(-0.0004 * k) / 16.0;
		tone += sin(6.0 * k) * exp(-0.0004 * k) / 32.0;

		tone += tone * tone * tone;

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
	double vol = 0.20;
	double notelen = 0.18;
	double where = 0.0, where_bass = 0.0;

	struct timeline *tl;

	tl = timeline_init(100 * R);
	if (!tl) {
		return EXIT_FAILURE;
	}


	/* short intro */
	add_note(tl, where, SOL_3, notelen * 4, vol / 4);
	add_note(tl, where, DO_4, notelen * 4, vol / 4);

	/* bass */
	add_note(tl, where, 28, notelen * 4, vol / 4);
	add_note(tl, where, 35, notelen * 4, vol / 4);

	where += notelen * 4;

	add_note(tl, where, MI_3, notelen * 1, vol / 2);

	/* bass */
	add_note(tl, where, DO_3, notelen * 1, vol / 2);

	where += notelen * 1;

	add_note(tl, where, MI_3, notelen * 4, vol / 6);
	add_note(tl, where, SOL_3, notelen * 4, vol / 6);
	add_note(tl, where, DO_4, notelen * 4, vol / 6);

	/* bass */
	add_note(tl, where, 28, notelen * 4, vol / 6);
	add_note(tl, where, 35, notelen * 4, vol / 6);
	add_note(tl, where, DO_3, notelen * 4, vol / 6);

	where += notelen * 4;

	/* pause */
	where += notelen * 2;


	/* start */
	add_note(tl, where, SOL_3, notelen * 1, vol);
	where += notelen * 1;

	/* 1 section */
	where_bass = where;
	add_note(tl, where, MI_3, notelen * 3, vol / 2);
	add_note(tl, where, DO_4, notelen * 3, vol / 2);

	where += notelen * 3;

	add_note(tl, where, MI_3, notelen * 2, vol / 2);
	add_note(tl, where, SOL_3, notelen * 2, vol / 2);
	where += notelen * 2;


	add_note(tl, where, LA_3, notelen * 1, vol);
	where += notelen * 1;

	add_note(tl, where, MI_3, notelen * 3, vol / 3);
	add_note(tl, where, SOL_3, notelen * 3, vol / 3);
	add_note(tl, where, SI_3, notelen * 3, vol / 3);
	where += notelen * 3;

	add_note(tl, where, MI_3, notelen * 1, vol / 1);
	where += notelen * 2;
	add_note(tl, where, MI_3, notelen * 1, vol / 1);
	where += notelen * 1;

	/* 1 section bass */
	add_note(tl, where_bass, LA_3_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, MI_4_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, LA_4_BASS, notelen * 6, vol / 3);
	where_bass += notelen * 6;

	add_note(tl, where_bass, DO_4_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, MI_4_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, SOL_4_BASS, notelen * 6, vol / 3);
	where_bass += notelen * 6;

	/* 2 section */
	where_bass = where;
	add_note(tl, where, DO_3, notelen * 3, vol / 2);
	add_note(tl, where, LA_3, notelen * 3, vol / 2);
	where += notelen * 3;

	add_note(tl, where, DO_3, notelen * 2, vol / 2);
	add_note(tl, where, SOL_3, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, FA_3, notelen * 1, vol / 1);
	where += notelen * 1;

	add_note(tl, where, DO_3, notelen * 3, vol / 2);
	add_note(tl, where, SOL_3, notelen * 3, vol / 2);
	where += notelen * 3;

	/* add short pause */
	add_note(tl, where, DO_3, notelen * 2 - 0.05, vol / 1);
	where += notelen * 2;

	add_note(tl, where, DO_3, notelen * 1, vol / 1);
	where += notelen * 1;

	/* 2 section bass */
	add_note(tl, where_bass, RE_4_BASS, notelen * 6, vol / 2);
	add_note(tl, where_bass, FA_4_BASS, notelen * 6, vol / 2);
	where_bass += notelen * 6;

	add_note(tl, where_bass, DO_4_BASS, notelen * 6, vol / 2);
	add_note(tl, where_bass, MI_4_BASS, notelen * 6, vol / 2);
	where_bass += notelen * 6;

	/* 3 section */
	where_bass = where;
	add_note(tl, where, RE_3, notelen * 3 - 0.05, vol / 1);
	where += notelen * 3;

	add_note(tl, where, RE_3, notelen * 2, vol / 1);
	where += notelen * 2;

	add_note(tl, where, MI_3, notelen * 1, vol / 1);
	where += notelen * 1;

	add_note(tl, where, RE_3, notelen * 3 - 0.05, vol / 2);
	add_note(tl, where, FA_3, notelen * 3 - 0.05, vol / 2);
	where += notelen * 3;

	add_note(tl, where, RE_3, notelen * 2, vol / 2);
	add_note(tl, where, FA_3, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, MI_3, notelen * 1, vol / 2);
	add_note(tl, where, SOL_3, notelen * 1, vol / 2);
	where += notelen * 1;

	/* 3 section bass */
	add_note(tl, where_bass, SI_3_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, RE_4_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, FA_4_BASS, notelen * 6, vol / 3);
	where_bass += notelen * 6;

	add_note(tl, where_bass, LA_3_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, RE_4_BASS, notelen * 6, vol / 3);
	add_note(tl, where_bass, FA_4_BASS, notelen * 6, vol / 3);
	where_bass += notelen * 6;

	/* 4 section */
	where_bass = where;

	add_note(tl, where, FA_3, notelen * 3, vol / 2);
	add_note(tl, where, LA_3, notelen * 3, vol / 2);
	where += notelen * 3;

	add_note(tl, where, SOL_3, notelen * 2, vol / 2);
	add_note(tl, where, SI_3, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, LA_3, notelen * 1, vol / 2);
	add_note(tl, where, DO_4, notelen * 1, vol / 2);
	where += notelen * 1;

	add_note(tl, where, SOL_3, notelen * 3, vol / 3);
	add_note(tl, where, SI_3, notelen * 3, vol / 3);
	add_note(tl, where, RE_4, notelen * 3, vol / 3);
	where += notelen * 5;

	add_note(tl, where, SOL_3, notelen * 1, vol);
	where += notelen * 1;

	/* 4 section bass */
	add_note(tl, where_bass, LA_3_BASS, notelen * 12.0 / 8.0, vol / 3);
	add_note(tl, where_bass, RE_4_BASS, notelen * 12.0 / 8.0, vol / 3);
	add_note(tl, where_bass, FA_4_BASS, notelen * 12.0 / 8.0, vol / 3);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, LA_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, SOL_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, FA_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, MI_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, RE_4_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, DO_4_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, SI_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	/* second line */
	/* 1 section */
	where_bass = where;
	add_note(tl, where, SOL_3, notelen * 3, vol / 3);
	add_note(tl, where, DO_4, notelen * 3, vol / 3);
	add_note(tl, where, MI_4, notelen * 3, vol / 3);
	where += notelen * 3;

	add_note(tl, where, SOL_3, notelen * 2, vol / 2);
	add_note(tl, where, RE_4, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, LA_3, notelen * 1, vol / 2);
	add_note(tl, where, DO_4, notelen * 1, vol / 2);
	where += notelen * 1;

	add_note(tl, where, SOL_3, notelen * 3, vol / 3);
	add_note(tl, where, SI_3, notelen * 3, vol / 3);
	add_note(tl, where, RE_4, notelen * 3, vol / 3);
	where += notelen * 3;

	add_note(tl, where, SOL_3, notelen * 2, vol / 2);
	add_note(tl, where, SI_3, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, SOL_3, notelen * 1, vol / 1);
	where += notelen * 1;

	/* 1 section bass */
	add_note(tl, where_bass, LA_3_BASS, notelen * 3, vol / 1);
	where_bass += notelen * 3;

	add_note(tl, where_bass, DO_4_BASS, notelen * 3, vol / 1);
	where_bass += notelen * 3;

	add_note(tl, where_bass, MI_4_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, SI_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, SOL_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, MI_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	/* 2 section */
	where_bass = where;
	add_note(tl, where, MI_3, notelen * 3, vol / 3);
	add_note(tl, where, LA_3, notelen * 3, vol / 3);
	add_note(tl, where, DO_4, notelen * 3, vol / 3);
	where += notelen * 3;

	add_note(tl, where, MI_3, notelen * 2, vol / 2);
	add_note(tl, where, SI_3, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, FA_DIES_3, notelen * 1, vol / 2);
	add_note(tl, where, LA_3, notelen * 1, vol / 2);
	where += notelen * 1;

	add_note(tl, where, MI_3, notelen * 3, vol / 3);
	add_note(tl, where, SOL_3, notelen * 3, vol / 3);
	add_note(tl, where, SI_3, notelen * 3, vol / 3);
	where += notelen * 3;

	add_note(tl, where, MI_3, notelen * 2 - 0.05, vol / 1);
	where += notelen * 2;

	add_note(tl, where, MI_3, notelen * 1, vol / 1);
	where += notelen * 1;

	/* 2 section bass */
	add_note(tl, where_bass, FA_3_BASS, notelen * 3, vol / 1);
	where_bass += notelen * 3;

	add_note(tl, where_bass, LA_3_BASS, notelen * 3, vol / 1);
	where_bass += notelen * 3;

	add_note(tl, where_bass, DO_4_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, SOL_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, MI_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, DO_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	/* 3 section */
	where_bass = where;
	add_note(tl, where, DO_3, notelen * 3, vol / 2);
	add_note(tl, where, LA_3, notelen * 3, vol / 2);
	where += notelen * 3;

	add_note(tl, where, DO_3, notelen * 2, vol / 2);
	add_note(tl, where, SOL_3, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, FA_DIES_3, notelen * 1, vol / 1);
	where += notelen * 1;

	add_note(tl, where, DO_3, notelen * 3, vol / 2);
	add_note(tl, where, SOL_3, notelen * 3, vol / 2);
	where += notelen * 3;

	add_note(tl, where, DO_3, notelen * 2 - 0.05, vol / 1);
	where += notelen * 2;

	add_note(tl, where, DO_3, notelen * 1, vol / 1);
	where += notelen * 1;

	/* 3 section bass */
	add_note(tl, where_bass, RE_3_BASS, notelen * 3, vol / 2);
	add_note(tl, where_bass, RE_4_BASS, notelen * 3, vol / 2);
	where_bass += notelen * 3;

	add_note(tl, where_bass, SI_3_BASS, notelen * 3, vol / 2);
	add_note(tl, where_bass, FA_4_BASS, notelen * 3, vol / 2);
	where_bass += notelen * 3;

	add_note(tl, where_bass, DO_4_BASS, notelen * 6, vol / 2);
	add_note(tl, where_bass, MI_4_BASS, notelen * 6, vol / 2);
	where_bass += notelen * 6;

	/* 4 section */
	where_bass = where;
	add_note(tl, where, DO_3, notelen * 3, vol / 3);
	add_note(tl, where, FA_DIES_3, notelen * 3, vol / 3);
	add_note(tl, where, DO_4, notelen * 3, vol / 3);
	where += notelen * 3;

	add_note(tl, where, MI_3, notelen * 2, vol / 2);
	add_note(tl, where, SI_3, notelen * 2, vol / 2);
	where += notelen * 2;

	add_note(tl, where, FA_3, notelen * 1, vol / 2);
	add_note(tl, where, LA_3, notelen * 1, vol / 2);
	where += notelen * 1;

	add_note(tl, where, SOL_3, notelen * 3, vol / 1);
	where += notelen * 3;


	/* 4 section bass */
	add_note(tl, where_bass, SI_3_BASS, notelen * 3, vol / 2);
	add_note(tl, where_bass, FA_4_BASS, notelen * 3, vol / 2);
	where_bass += notelen * 3;

	add_note(tl, where_bass, SI_3_BASS, notelen * 3, vol / 2);
	add_note(tl, where_bass, LA_4_BASS, notelen * 3, vol / 2);
	where_bass += notelen * 3;

	add_note(tl, where_bass, MI_4_BASS, notelen * 12.0 / 8.0, vol / 2);
	add_note(tl, where_bass, SOL_4_BASS, notelen * 12.0 / 8.0, vol / 2);
	where_bass += notelen * 12.0 / 8.0;

	add_note(tl, where_bass, RE_DIES_4_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, DO_4_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;
	add_note(tl, where_bass, SI_3_BASS, notelen * 12.0 / 8.0, vol / 1);
	where_bass += notelen * 12.0 / 8.0;

	timeline_play(tl);

	timeline_free(tl);

	return EXIT_SUCCESS;
}

