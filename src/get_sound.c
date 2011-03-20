#include <fcntl.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

#include "get_sound.h"

static int setup_dsp( int fd );
int recoard_sound(short *, int);
int check_warmup(short *, int);

int get_sound(short *buf, int rtime)
{
	int flag = 0;
	int output_count = 0;
	int i, ret;
	unsigned int buffer_size;

	if(! buf){
		return RET_ERROR;
	}

	buffer_size = BUFSIZE * rtime;
	ret = recoard_sound(buf, buffer_size * sizeof(short));
	if(ret == RET_ERROR){
		return RET_ERROR;
	}

	for(i=0;i<buffer_size/sizeof(short);i++){
		ret = check_warmup(buf, i);

		if(ret == RET_SUCCESS){
			SetFlag(flag, WARMUP_STATE);
		}

		/*
		if(GetFlag(flag, WARMUP_STATE)){
			if((++output_count % range) == 0){
				//printf("\n");

				if((i + range) > (buffer_size/sizeof(short))){
					break;
				}
			}

			//printf("%d ", buf[i]);
		}
		*/
	}

	return RET_SUCCESS; 
}

int check_warmup(short *buf, int current_index)
{
	int i;

	if(!buf){
		return RET_ERROR;
	}

	if(buf[current_index] == 0){
		return RET_ERROR;
	}

	for(i=current_index;i<(current_index + VIEW_RANGE);i++){
		if(buf[i] == 0){
			return RET_ERROR;
		}
	}

	return RET_SUCCESS;
}

int recoard_sound(short *buf, int len)
{
	int fd;

	if ( ( fd = open( "/dev/dsp", O_RDWR ) ) == -1 ) {
		perror( "open()" );
		return RET_ERROR;
	}

	if ( setup_dsp( fd ) != 0 ) {
		fprintf( stderr, "Setup /dev/dsp failed.\n" );
		close( fd );
		return RET_ERROR;
	}

	if ( read(fd, buf, len) == -1 ) {
		perror( "read()" );
		close( fd );
		return RET_ERROR;
	}

	/*
	if ( write( fd, buf, len ) == -1 ) {
		perror( "write()" );
		close( fd );
		return 1;
	}
	*/

	close( fd );

	return RET_SUCCESS;
}

void write_sound_log(short *buf, int len)
{
	int i;
	FILE *fd;

	fd = fopen(SoundOUTPUT, "a");
	if(! fd){
		return ;
	}	

	for(i=0;i<len;i++){
		fprintf(fd, "%d\n", buf[i]);
	}
	
	fclose(fd);
}


/*
 * /dev/dsp を以下の様に設定する。
 *
 * 量子化ビット数     : 16   bits
 * サンプリング周波数 : 44.1 KHz
 * チャンネル数       : 1
 * PCM データは符号付き、リトルエンディアン
 *
 */
static int setup_dsp( int fd )
{
	int fmt     = AFMT_S16_LE;
	int freq    = 44100;
	int channel = 1;

	if ( ioctl( fd, SOUND_PCM_SETFMT, &fmt ) == -1 ) {
		perror( "ioctl( SOUND_PCM_SETFMT )" );
		return -1;
	}

	if ( ioctl( fd, SOUND_PCM_WRITE_CHANNELS, &channel ) == -1 ) {
		perror( "ioctl( SOUND_PCM_WRITE_CHANNELS )" );
		return -1;
	}

	if ( ioctl( fd, SOUND_PCM_WRITE_RATE, &freq ) == -1 ) {
		perror( "ioctl( SOUND_PCM_WRITE_RATE )" );
		return -1;
	}

	return 0;
}
