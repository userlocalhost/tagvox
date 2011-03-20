#include <fcntl.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * 16 (bits) * 44100 (Hz) * 5 (sec) = 3528000 (bits)
 *                                  = 441000  (bytes)
 */
#define BUFSIZE 44100
#define SAMPLING_RATE 44100
#define QUANTIZATION_BIT 16

static int setup_dsp(int);

int main(void)
{
	int fd;
	short *value;
	int i = 0;

	if ( ( fd = open( "/dev/dsp", O_RDWR ) ) == -1 ) {
		perror( "open()" );
		return 1;
	}

	if ( setup_dsp( fd ) != 0 ) {
		fprintf( stderr, "Setup /dev/dsp failed.\n" );
		close( fd );
		return 1;
	}

	while(read(fd, value, sizeof(short) > 0)){
		printf("%d\n", *value);
	}

	close( fd );

	return 0;
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

