#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <uuid/uuid.h>
#include <math.h>

#include "judge.h"

static Judgements *judge = NULL;
static void dump(void);
static double ret_max(double *, int);
static double get_average(double *, int);
static double get_variance(double *, int);

int construct_judgements()
{
	int i;

	if(judge){
		return RET_SUCCESS;
	}

	judge = (Judgements *) malloc(sizeof(Judgements));
	if(!judge){
		return RET_ERROR;
	}

	judge->member_length = 0;

	return RET_SUCCESS;
}

/*
 * if judgements chrom array is fill, return 1, or 0.
 * */
int is_judgements_fill()
{
	if(! judge){
		return RET_ERROR;
	}

	if(judge->member_length >= JUDGEMENTS_LEN){
		return 1;
	}

	return 0;
}

void destruct_judgements()
{
	if(judge){
		free(judge);
	}
}

int push_judgement_chrom(Chromosome *new_chrom)
{
	int current_index;
	int i;

	if(is_judgements_fill()){
		return RET_ERROR;
	}

	for(i=0;i<judge->member_length;i++){
		if(uuid_compare(judge->chrom[i]->serial, new_chrom->serial) == 0 &&
				judge->chrom[i]->generation == new_chrom->generation){

			return RET_SUCCESS;
		}
	}

	tagu_debug("[push_judgement_chrom] new_chrom->fitness : %f\n", new_chrom->fitness);

	current_index = judge->member_length;
	judge->chrom[current_index] = new_chrom;
	judge->member_length++;

	// for debugging
	dump();

	return RET_SUCCESS;
}

static void dump()
{
	int i;
	char uuid_str[37];
	Chromosome *chrom;

	for(i=0;i<(judge->member_length);i++){
		chrom = judge->chrom[i];
		uuid_unparse(chrom->serial, uuid_str);

		tagu_debug("<judge:dump> [%d/%d] serial:%s, generation:%d, fitness:%f\n", i, judge->member_length, uuid_str, chrom->generation, chrom->fitness);
	}

	tagu_debug("<judge:dump> finish\n");
}

static double ret_max(double *array, int len)
{
	double ret = 0;
	int i;

	for(i=0;i<len;i++){
		if(ret < array[i]){
			ret = array[i];
		}
	}

	return ret;
}

/*
 * 染色体 (NN の構造) の評価を行う
 * @chro : 染色体
 * @input : 入力信号 (パワースペクトル)
 * @output : 出力信号 (学習結果)
 * */
int eval_chromosome(Chromosome *chro, double *input, double *output)
{
	double *src;
	double input_average;
	double input_variance;
	double input_deviation;
	int i,j,k;

	if(! chro){
		return RET_ERROR;
	}

	if(! output){
		return RET_ERROR;
	}

	if(! input){
		tagu_debug("[error] eval_chromosome > input is invalid\n");
		return RET_ERROR;
	}

	input_average = get_average(input, chro->len);
	input_variance = get_variance(input, chro->len);
	input_deviation = sqrt(input_variance);

	for(src=input, i=0;i<LayerDepth;i++){
		double max = ret_max(src, chro->len);
		if(max == 0){
			for(j=0;j<chro->len;j++){
				output[j] = 0;
			}

			return RET_SUCCESS;
		}

		for(j=0;j<chro->len;j++){ // for each schema
			double result = 0;

			for(k=0;k<chro->len+1;k++){ // for each weight
				if(k == 0){
					result = chro->schema[i][j][0];
				}else{
					//double value = (src[k-1]/max) > INPUT_THREASHOLD ? 1 : 0;
					double value;
					double deviation = 0;
					if(src == input){
						deviation = ((src[k-1] - input_average) / input_deviation) * 10 + 50;
						value = (deviation > 50) ? 1 : 0;
					}else{
						value = src[k-1];
					}

					result += value * chro->schema[i][j][k];
					tagu_debug("%f += %f(%f/%f, d:%f, va:%f, id:%f) * %f\n", result, value, src[k-1], max, deviation, input_variance, input_deviation, chro->schema[i][j][k]);
				}
			}

			result /= chro->len+1;

			output[j] = result > EXPOSE_SCALE  ? 1 : 0;

			tagu_debug("[eval_chromosome] result (%d:%d) > %f, %f\n", i, j, result, output[i]);
		}

		src = output;
	}

	return RET_SUCCESS;
}

static double get_average(double *input, int length)
{
	double average = 0;
	int i;

	for(i=0; i<length; i++){
		average += input[i];
	}
	average /= length;

	return average;
}

static double get_variance(double *input, int length)
{
	double variance = 0;
	double average = 0;
	int i;

	for(i=0; i<length; i++){
		average += input[i];
	}
	average /= length;

	for(i=0; i<length; i++){
		double tmp = input[i] - average;
		variance += tmp * tmp;
	}
	variance /= length;

	return variance;
}

/*
 * This routine judge voice who is.
 *
 * @return : user index
 * */
int judgement(double *spectrum, int index_length, int user_len)
{
	int i,j;
	double *candidate;
	double *output_buffer;
	int ret_index;
	double max = 0;

	if(! spectrum){
		return RET_ERROR;
	}

	candidate = (double *)malloc(sizeof(double) * user_len);
	if(! candidate){
		return RET_ERROR;
	}

	output_buffer = (double *)malloc(sizeof(double) * index_length);
	if(! output_buffer){
		free(candidate);
		return RET_ERROR;
	}

	memset(candidate, 0, sizeof(double) * user_len);

	for(i=0;i<judge->member_length;i++){
		eval_chromosome(judge->chrom[i], spectrum, output_buffer);

		for(j=0;j<user_len;j++){
			candidate[j] += output_buffer[j];
		}
	}

	for(ret_index=i=0;i<user_len;i++){
		tagu_debug("[judgement] candidate[%d] : %f\n", i, candidate[i]);

		if(candidate[i] > max){
			ret_index =	i;
			max = candidate[i];
		}
	}

	free(candidate);
	free(output_buffer);

	return ret_index;
}
