#ifndef __NN_CHROMOSOME__
#define __NN_CHROMOSOME__

#include "common.h"

#define StaticGENES 100

#define GenerationNumber 10

#define BUFLEN 4096

#define LayerDepth 2

#define SPECTRUM_LENGTH 512
#define STUDY_NUM 50

#define InputLayerNodes SPECTRUM_LENGTH

#define LogFILE "/home/gakusei/prog/nn_ga/output.log"
#define SoundOUTPUT "/home/gakusei/prog/nn_ga/sound.data"
#define StudyDataDir "/home/gakusei/prog/nn_ga/study_data"
#define ExportDir "/home/gakusei/prog/nn_ga/export_genes/"

#define NAME_BUF 64
#define TMP_BUFLEN 512

#define ELITE_THREASHOLD (0.96)

// 交叉回数
#define CROSSOVER_COUNT 10

// 突然変異の確率
#define MUTATION_RATE 0.01

#define SAMPLING_RATE 44100

/* these flags is used for gene_create() */
#define GC_INITLESS 1

#define RTIME 1

typedef struct __Chromosome {
	int len;
	int generation;
	int depth;
	double fitness;
	double **schema[LayerDepth];

	uuid_t serial;
} Chromosome;

typedef struct __User {
	char name[NAME_BUF];
	int recoard_length;
	double spectrum[STUDY_NUM][SPECTRUM_LENGTH];
}User;

typedef struct __UserHash {
	User user[SPECTRUM_LENGTH];
	int userlen;
}UserHash;

#endif
