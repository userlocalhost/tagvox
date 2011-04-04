#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <uuid/uuid.h>

#include "nn_chromosome.h"
#include "judge.h"
#include "get_sound.h"

static Chromosome *create_gene(int, int);
static Chromosome *copy_gene(Chromosome *);
static Chromosome **init(int);
static int construct_userhash(void);

static void delete_gene(Chromosome *);
static void destroy(Chromosome **, int);

static void initChromosomeValue(Chromosome *);
static void __crossover(long *, long *, int);
static int compare_chrom_fitness(void *, void *);
static void local_sort(void **, int, int(*func)(void *, void*));
static int crossover_chromosome(Chromosome **);
static int compare_fitness(const Chromosome *, const Chromosome *);
static Chromosome **choice_chrom(Chromosome **);

/* debug functions */
static void dump_all_gene(Chromosome **, int);
static void dump_output(Chromosome *, double *);
static void test_eval(Chromosome **);
static int find_nearest_multiple(int);
static int fit_format(double *, double *, int, int);
static int export_chromosome(Chromosome *);

static void study(char *fpath);

static double calcFitness(Chromosome *);
static int main_loop(void);

// global value
static UserHash *uhash = NULL;
static gflag = 0;

static Chromosome **init(int gene_num)
{
	int i;
	Chromosome **genes;

	srand(time(NULL));

	genes = (Chromosome **)malloc(sizeof(Chromosome *) * gene_num);
	if(! genes) {
		return NULL;
	}

	if(construct_userhash() == RET_ERROR){
		printf("[ERROR] fail at construct_userhash()\n");
		return NULL;
	}

	if(construct_judgements() == RET_ERROR){
		printf("[ERROR] fail at construct_judgements()\n");
		return NULL;
	}

	tagu_debug("[init] genes : 0x%x\n", genes);

	for(i=0;i<gene_num;i++) {
		Chromosome *chro;
		chro = create_gene(InputLayerNodes, 0);
		if(! chro){
			destroy(genes, i);
			return NULL;
		}

		genes[i] = chro;
	}
	
	printf("[init] ==== Initialization is over ====\n");

	return genes;
}

static int regist_userinfo(User *user, const char *fname)
{
	int value;
	FILE *fd;
	double *input;
	char filepath[TMP_BUFLEN];
	char username[NAME_BUF];
	char *suffix_addr;
	int dirname_length;
	int name_length;
	int i, j, ret;
	int start_flag = 0;

	memset(filepath, 0, TMP_BUFLEN);
	dirname_length = strlen(StudyDataDir);

	strncpy(filepath, StudyDataDir, dirname_length);
	filepath[dirname_length] = '/';

	strncpy(filepath + (dirname_length + 1), fname, strlen(fname));

	fd = fopen(filepath, "r");
	if(! fd){
		printf("[error] regist_userinfo > fail to open : %s\n", filepath);
		return RET_ERROR;
	}

	suffix_addr = rindex(fname, '.');
	if(suffix_addr == NULL){
		return RET_ERROR;
	}

	name_length = suffix_addr - fname;
	if(name_length > (NAME_BUF - 1)){
		name_length = (NAME_BUF - 1);
	}

#ifdef __DEBUG__
	strncpy(username, fname, name_length);
	username[name_length] = '\0';

	printf("[onix_debug] regist_userinfo > filepath : %s\n", filepath);
	printf("[onix_debug] regist_userinfo > username : %s [%d]\n", username, name_length);
#endif

	input = (double *)malloc(SAMPLING_RATE * RTIME * sizeof(double) * 2);
	if(!input){
		return RET_ERROR;
	}

	for(j=0;j<STUDY_NUM;j++){
		for(i=0;i<(SAMPLING_RATE * RTIME);i++){
			ret = fscanf(fd, "%d\n", &value);
			if(start_flag == 0 && value == 0){
				i--;
				continue;
			}else if(start_flag == 0 && value != 0){
				start_flag = 1;
			}

			if(start_flag > 0){
				input[i] = (double)(value);
			}
		}

		int aligned_length = find_nearest_multiple(SAMPLING_RATE * RTIME);

		for(i=0;i<aligned_length;i++){
			input[(aligned_length-1) + i] = input[(aligned_length-1) - i];
		}
		get_spectrum(input, aligned_length);

		fit_format(user->spectrum[j], input, SPECTRUM_LENGTH, aligned_length/2);
	}
	
	free(input);

	ret = RET_SUCCESS;
out:
	fclose(fd);

	return ret;
}

static int construct_userhash()
{
	DIR *dd;
	struct dirent *dir;
	int index;

	if(uhash){
		return RET_SUCCESS;
	}

	uhash = malloc(sizeof(UserHash));
	if(! uhash){
		return RET_ERROR;
	}

	memset(uhash, 0, sizeof(UserHash));

	dd = opendir(StudyDataDir);
	if(! dd){

		if(mkdir(StudyDataDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IXGRP|S_IRGRP|S_IROTH|S_IXOTH) < 0){
			tagu_debug("construct_userhash 01\n");
			free(uhash);
			uhash = NULL;
	
			return RET_ERROR;
		}
		dd = opendir(StudyDataDir);
	}

	for(index=0;(dir = readdir(dd)) != NULL;){
		if(dir->d_name[0] != '.'){
			regist_userinfo(&uhash->user[index], dir->d_name);

			tagu_debug("construct_userhash > [%d] %s\n", index, uhash->user[index].name);
			index++;
		}
	}

	closedir(dd);
	
	uhash->userlen = index;

	tagu_debug("[onix_debug] construct_userhash > uhash->userlen : %d\n", uhash->userlen);

	return RET_SUCCESS;
}

static Chromosome *create_gene(int node_num, int flag)
{
	Chromosome *chro;
	double *ret;
	int i,j,k;

	chro = (Chromosome *)malloc(sizeof(Chromosome));
	if(!chro){
		return NULL;
	}

	chro->len = node_num;
	chro->generation = 0;
	chro->depth = LayerDepth;

	for(i=0;i<LayerDepth;i++){

		ret = (double *)malloc(sizeof(double *) * (node_num));
		if(! ret){
			free(chro);
			return NULL;
		}
	
		chro->schema[i] = (double **)ret;
	
		for(j=0;j<node_num;j++){
			// the +1 is for baias
			ret = (double *)malloc(sizeof(double) * (node_num+1));
			if(! ret){
				for(k=0;k<j;k++){
					free(chro->schema[i][j]);
				}
	
				return NULL;
			}
	
			chro->schema[i][j] = ret;
		}
	}

	if(! GetFlag(flag, GC_INITLESS)){
		uuid_generate(chro->serial);

		initChromosomeValue(chro);
		chro->fitness = calcFitness(chro);
	}

	return chro;
}

static Chromosome *copy_gene(Chromosome *chro)
{
	Chromosome *ret;
	int len;
	int i,j,k;

	if(!chro){
		return NULL;
	}

	len = chro->len;
	ret = create_gene(len, GC_INITLESS);
	if(!ret){
		return NULL;
	}

	ret->fitness = chro->fitness;
	ret->generation = chro->generation;
	uuid_copy(ret->serial, chro->serial);

	for(i=0;i<LayerDepth;i++){
		for(j=0;j<len;j++){
			for(k=0;k<(len+1);k++){
				ret->schema[i][j][k] = chro->schema[i][j][k];
			}
		}
	}

	return ret;
}

/*
 * 遺伝子にランダムで初期値を設定
 * */
static void initChromosomeValue(Chromosome *chro)
{
	int i,j,k;

	for(i=0;i<LayerDepth;i++){
		for(j=0;j<chro->len;j++){ // for each node
			for(k=0;k<chro->len+1;k++){
				int rand_value = rand() % 100;
				chro->schema[i][j][k] = ((double)rand_value/100);
			}
		}
	}
}

static void destroy(Chromosome **genes, int len)
{
	int i;

	for(i=0;i<len;i++){
		delete_gene(genes[i]);
	}

	free(uhash);
	destruct_judgements();
}

static void delete_gene(Chromosome *chro)
{
	int i,j,k;

	if(!chro){
		return;
	}

	for(i=0;i<LayerDepth;i++){
		for(j=0;j<chro->len;j++){
			free(chro->schema[i][j]);
		}

		free(chro->schema[i]);
	}
}

static void dump_all_gene(Chromosome **genes, int num)
{
	int i, j, k, l, len;
	Chromosome *chro;
	FILE *fp;
	char write_buffer[BUFLEN];

	if(! genes){
		return;
	}

	fp = fopen(LogFILE, "w+");
	if(!fp){
		return;
	}

	for(i=0;i<num;i++){ //for each node
		chro = genes[i];

		//** 各遺伝子のダンプを出力
		fprintf(fp, "(dump) ==== genes[%d] ====\n", i);

		for(l=0;l<LayerDepth;l++){
			for(j=0;j<chro->len;j++){ // for each schema
				fprintf(fp, "schema[%d][%d] >>>\n", l, j);
				memset(write_buffer, '\0', BUFLEN);
	
				for(len=0, k=0;k<chro->len+1;k++){ // for each weight
					len += sprintf((write_buffer + len), "%d, ", chro->schema[l][j][k]);
				}
	
				fprintf(fp, "%s\n", write_buffer);
			}
		}
	}

	fclose(fp);
}

/*
 * 交叉対象の染色体のアドレスを選択
 * */
static Chromosome **choice_chrom(Chromosome **genes)
{
	Chromosome **ret = NULL;
	int i, index;

	for(i=0;i<StaticGENES;i++){
		index = rand() % StaticGENES;
		ret = &genes[index];

		/*
		if(! (*ret)->fitness > ELITE_THREASHOLD){
			break;
		}
		*/
	}

	return ret;
}

static void __crossover(long *container1, long *container2, int length)
{
	int *tmp_buffer1;
	int *tmp_buffer2;
	int i;
	long buf1, buf2;

	if(!tmp_buffer1 || !tmp_buffer2){
		return ;
	}

	for(i=0;i<length;i++){
		if(rand()%2){
			buf1 = container1[i];
			buf2 = container2[i];
		}else{
			buf2 = container1[i];
			buf1 = container2[i];
		}

		container1[i] = buf1;
		container2[i] = buf2;
	}
}

static int compare_fitness(const Chromosome *a, const Chromosome *b)
{
	int va = (int)(a->fitness * 100000);
	int vb = (int)(b->fitness * 100000);

	return vb - va;
}

/*
 * if chromeA is bigger than chromeB, return 0, or 1.
 * */
static int compare_chrom_fitness(void *argA, void *argB)
{
	Chromosome *chromA = argA;
	Chromosome *chromB = argB;

	if(chromA->fitness < chromB->fitness){
		return 1;
	}

	return 0;
}

static void local_sort(void **array, int len, int (*func)(void *, void*))
{
	int i;
	int max_offset;
	Chromosome *change_tmp;

	if(len <= 1){
		return;
	}

	for(max_offset=0, i=1;i<len;i++){
		if(func(array[max_offset], array[i])){
			max_offset = i;
		}
	}

	if(max_offset != (len-1)){
		change_tmp = array[len-1];
		array[len-1] = array[max_offset];
		array[max_offset] = change_tmp;
	}

	return local_sort(array, len-1, func);
}

/*
 * 染色体の交叉
 * →　親の染色体をそれぞれ引数で受け取り、結果を引数で渡した染色体に格納させる
 * →　各ノードの中の重みを交叉させる
 *
 * */
static int crossover_chromosome(Chromosome **genes)
{
	int j, k, l;
	int len;
	double fitness;
	Chromosome **p1, **p2;
	Chromosome *copyed_chrom1, *copyed_chrom2;

	p1 = choice_chrom(genes);
	while((p2 = choice_chrom(genes)) == p1){
		// nope
	}

	if(! *p1 || ! *p2){
		printf("[error] (*p1):0x%x, (*p2):0x%x\n", *p1, *p2);
		return RET_ERROR;
	}

	/*old fitness*/
	(*p1)->fitness = calcFitness((*p1));
	(*p2)->fitness = calcFitness((*p2));

	len = (*p1)->len;

#ifdef __DEBUG__
	printf("[onix_debug] (*p1)->fitness : %f\n", (*p1)->fitness);
	printf("[onix_debug] (*p2)->fitness : %f\n", (*p2)->fitness);
#endif

	copyed_chrom1 = copy_gene(*p1);
	copyed_chrom2 = copy_gene(*p2);

	(*p1)->generation++;
	(*p2)->generation++;

	for(l=0;l<LayerDepth;l++){ // append for each layer
		for(j=0;j<len;j++){ // crossover for each node
			__crossover((long *)((*p1)->schema[l][j]), (long *)((*p2)->schema[l][j]), len+1);

		}
		//__crossover((long *)((*p1)->schema[l]), (long *)((*p2)->schema[l]), len);
	}

	(*p1)->fitness = calcFitness((*p1));
	(*p2)->fitness = calcFitness((*p2));

	if((*p1)->fitness == RET_ERROR || (*p2)->fitness == RET_ERROR){
		return RET_ERROR;
	}

	Chromosome *array[] = {*p1, *p2, copyed_chrom1, copyed_chrom2};

	local_sort((void **)array, 4, compare_chrom_fitness);

#ifdef __DEBUG__
	for(j=0;j<4;j++){
		printf("[onix_debug] array[%d]->fitness > (0x%x)%f\n", j, array[j], array[j]->fitness);
	}
#endif

	// bring down the best fitness chromosome and second best to the next generation
	(*p1) = array[2];
	(*p2) = array[3];

	/*
	if((rand() % 10000) == 0){
		int m = rand() % len;
		int n = rand() % (len+1);
		double value = (double)(rand() % (BIAS * 2));

		printf("[crossover_chromosome] <mutation> schema[0][%d][%d] <= %f\n", m, n, value);
		(*p1)->schema[0][m][n] = value;
	}
	*/

	// destroy bad chromosome ....
	delete_gene(array[0]);
	delete_gene(array[1]);

	return RET_SUCCESS;
}

static void test_eval(Chromosome **genes)
{
	double maxfit = 0; // to reserve elite chromosome
	int j; // generation number
	int ret;
	int elite_index;
	int loop_flag = 0;
	char uuid_string[37];

	while(! loop_flag){
		printf("=== The main study processing ===\n");

		for(j=0;j<CROSSOVER_COUNT;j++){
			crossover_chromosome(genes);
		}

		local_sort((void **)genes, StaticGENES, compare_chrom_fitness);
	
		for(j=StaticGENES-1;j>=0;j--){
			uuid_unparse(genes[j]->serial, uuid_string);
		
#ifdef __DEBUG__
			printf("[test_eval] genes[%d]->fitness:%f(%s:%d)\n", j, genes[j]->fitness, uuid_string, genes[j]->generation);
#endif

			if(genes[j]->fitness > ELITE_THREASHOLD){

				Chromosome *copyed_chrom = copy_gene(genes[j]);
				if(! copyed_chrom){
					printf("[error] test_eval > fail to copy chromosome\n");
					return;
				}

				ret = push_judgement_chrom(copyed_chrom);
				if(ret == RET_ERROR){
					printf("[error] test_eval > fail to push_judgement_chrom()\n");
					return;
				}

				if(is_judgements_fill()){
					loop_flag = 1;
					break;
				}
			}
		}

		for(j=0;j<StaticGENES;j++){
			export_chromosome(genes[j]);
		}
	}
}

static void dump_output(Chromosome *chro, double *output)
{
	int i;

	if(! chro || ! output){
		return ;
	}

	for(i=0;i<chro->len;i++){
		printf("[debug] dump_output output[%d] > %f\n", i, output[i]);
	}
}

static double calcFitness(Chromosome *chro)
{
	double output[SPECTRUM_LENGTH];
	double fit = 0;
	int i,j,l;

	if(! chro){
		return RET_ERROR;
	}

	tagu_debug("[calcFitness] uhash : 0x%x\n", uhash);

	for(i=0;i<uhash->userlen;i++){
		User user = uhash->user[i];
		double val = 0;

		//dump_output(chro, output);

		for(j=0;j<STUDY_NUM;j++){
			eval_chromosome(chro, user.spectrum[j], output);

			val = (i == j) ? (1 - output[j]) : (0 - output[j]);

			fit += (val * val);
		}

		fit = fit/STUDY_NUM;
	}

	return (1 - fit/uhash->userlen);
}

/*
 * The export format is specified at ~/doc/export_spec.txt
 * */
static int export_chromosome(Chromosome *chrom)
{
	int i,j,k;
	FILE *fp;
	char filepath[TMP_BUFLEN];
	char uuid_string[37];
	int dirname_len;

	if(! chrom){
		return RET_ERROR;
	}

	dirname_len = strlen(ExportDir);
	strncpy(filepath, ExportDir, dirname_len);
			
	uuid_unparse(chrom->serial, uuid_string);
	strncpy(filepath + dirname_len, uuid_string, 36);
	filepath[dirname_len + 36] = '\0';

	fp = fopen(filepath, "w");
	if(!fp){
		printf("[error] export_chromsome > fail to open %s\n", filepath);
		return RET_ERROR;
	}
	
	// write down configuration-line
	fprintf(fp, "%d:%d:%d\n", chrom->depth, chrom->len, chrom->generation);
	for(i=0;i<chrom->depth;i++){
	
		for(j=0;j<chrom->len;j++){
			for(k=0;k<(chrom->len+1);k++){
				fprintf(fp, "%f:", chrom->schema[i][j][k]);
			}
			fprintf(fp, "\n");
		}

	}

	fclose(fp);
}

static void study(char *fpath)
{
	char *str;
	int fd;
	int len, i;
	char str_buf[BUFLEN];
	char sound_buf[BUFLEN];
	char tmp_buf[BUFLEN];

	if((fd = open(fpath, O_RDONLY)) == -1){
		printf("fail to open(%s)\n", fpath);
		return;
	}

	while(len = read(fd, str_buf, BUFLEN)){
		int count = 0;
		int flag_num = 0;

		memset(tmp_buf, '\0', BUFLEN);

		for(i=0;i<len;i++){
			printf("[debug] message : <%c>\n", str_buf[i]);

			if((str_buf[i] >= '0' && str_buf[i] <= '9') || str_buf[i] == '-'){
				flag_num = 1;
				tmp_buf[count++] = str_buf[i];
			}else if(flag_num > 0){
				tmp_buf[count] = '\0';
				printf("num : %d\n", atoi(tmp_buf));

				memset(tmp_buf, '\0', BUFLEN);
				count = flag_num = 0;
			}
		}
	}

	close(fd);
}

static int find_nearest_multiple(int value)
{
	int ret;

	for(ret=1;(ret * 2)<=value;ret*=2);

	return ret;
}

static int fit_format(double *dst, double *src, int dst_length, int src_length)
{
	int window_size;
	int window_total;
	int aligned_length;
	int i,j;

	if(!dst || !src){
		return RET_ERROR;
	}

	src_length = find_nearest_multiple(src_length);

	if(dst_length > src_length){
		window_size = 1;
	}else{
		window_size = src_length / dst_length;
	}
	
	for(window_total=0, j=0, i=1;i<=src_length;i++){
		window_total += src[i-1];

		if((i % window_size) == 0){
			dst[j++] = window_total / window_size;
			window_total = 0;
		}
	}

	return RET_SUCCESS;
}

static int main_loop()
{
	int i,j,flg;
	int ret = RET_SUCCESS;
	short *read_buffer;
	double *double_buffer;
	double judge_input[SPECTRUM_LENGTH];

	read_buffer = malloc(SAMPLING_RATE * RTIME * sizeof(short));
	double_buffer = malloc(SAMPLING_RATE * RTIME * sizeof(double) * 2);

	if(!read_buffer || !double_buffer){
		printf("[error] fail to get memory any more.\n");

		return RET_ERROR;
	}

	gflag = 1;

	while(1){
		int start_index;
		memset(read_buffer, 0, SAMPLING_RATE * RTIME);
		get_sound(read_buffer, RTIME);

		for(flg=0, j=0, i=0;i<(SAMPLING_RATE * RTIME);i++){
			if(read_buffer[i] != 0 && flg == 0){
				flg = 1;
				start_index = i;
			}
			
			if(flg > 0){
				double_buffer[j++] = read_buffer[i];
			}
		}

		printf("[main_loop] write length : %d\n", j - start_index);
		write_sound_log(read_buffer + start_index, j - start_index);

		int aligned_length = find_nearest_multiple(j - start_index);
		for(j=0;j<aligned_length;j++){
			double_buffer[(aligned_length-1) + j + 1] = double_buffer[(aligned_length-1) - j];
		}

		get_spectrum(double_buffer, aligned_length);
		
		fit_format(judge_input, double_buffer, SPECTRUM_LENGTH, aligned_length/2);

		ret = judgement(judge_input, SPECTRUM_LENGTH, uhash->userlen);
		if(ret == RET_ERROR){
			break;
		}

		printf("judgement %s (%d)\n", uhash->user[ret].name, ret);
	}

	free(read_buffer);
	free(double_buffer);

	return ret;
}

int main()
{
	int i;
	Chromosome **genes;

	genes = init(StaticGENES);
	if(! genes || !uhash) {
		return 1;
	}

	for(i=0;i<StaticGENES;i++){
		export_chromosome(genes[i]);
	}

	test_eval(genes);

	printf("===== study finish =====\n");

	main_loop();

	destructPSWindow();
	destroy(genes, StaticGENES);

	return 0;
}
