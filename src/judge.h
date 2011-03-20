#ifndef __JUDGE__
#define __JUDGE__

#include "nn_chromosome.h"

#define JUDGEMENTS_LEN 5
#define EXPLOSION_BIAS 100

#define INPUT_THREASHOLD 0.4
#define EXPOSE_SCALE 0.3

typedef struct __Judgements {
	Chromosome *chrom[JUDGEMENTS_LEN];
	int member_length;
}Judgements;

int construct_judgements(void);
void destruct_judgements(void);

int get_judge_members(void);
int push_judgement_chrom(Chromosome *);
int eval_chromosome(Chromosome *, double *, double *);
int judgement(double *, int, int);

#endif
