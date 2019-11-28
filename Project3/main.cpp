

#include <windows.h>
#include <stdio.h>

//#define WAVE_SOUND
#define DIRECT_SOUND
//#define OPENAL_SOUND

extern "C" {
	void openal_init(DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels);
	void openal_out(char *buf, int bufsize);
	void openal_close();


	void wave_init(DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels);
	void wave_out(char *buf, int bufsize);
	void wave_close();

	int DirectSound_init(DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels);
	void DirectSound_update(void*data, long size);
	void DirectSound_close();
}



typedef struct WAVE_INFO {
	unsigned char  fileId[4];// 4B “RIFF”
	INT32  fileLen;// 头后文件长度 4B 非负整数(= 文件长度－8)
	INT32  waveId; //数据类型标识符 波形文件标识符
}WAVE_INFO;

typedef struct WAVE_HEADER_INFO {
	INT32 chkid;// "fmt"
	INT32 chkLen;//非负整数(= 16或18)
	short wFormatTag;//2B 非负短整数(PCM=1)
	short wChannels;//2B  非负短整数(= 1或2)
	INT32 dwSampleRate;//4B 非负整数(单声道采样数/秒)
	INT32 dwAvgBytesRate;//4B 非负整数(字节数/秒)
	short wBlockAlign;//2B 非负短整数(不足补零)
	short wBitsPerSample;//2B 非负短整数(PCM时才有)
	//short wExtSize;//2B
	 //extraInfo;//extSizeB
}WAVE_HEADER_INFO;

typedef struct WAVE_DATE_HEADER_INFO {
	INT32 chkId;//4B
	INT32 chkLen;//4B;
}WAVE_DATE_HEADER_INFO;


void testWaveOut(const char *path) {
	FILE * f = fopen(path, "rb");
	WAVE_INFO info;
	WAVE_HEADER_INFO hInfo;
	WAVE_DATE_HEADER_INFO dataInfo;
	fread(&info, 1, sizeof(info), f);
	const unsigned  char *RIFF = reinterpret_cast<const unsigned  char *>("RIFF");
	/*if (strcmp( &(info.fileId), RIFF)) {
		return;
	}*/
	fread(&hInfo, 1, sizeof(hInfo), f);
	if (hInfo.chkLen == 18) {
		short wExtSize;
		fread(&wExtSize, 1, sizeof(short), f);
		fseek(f, wExtSize, SEEK_CUR);
		//fread(&wExtSize, 1, sizeof(short), f);
	}
	fread(&dataInfo, 1, sizeof(WAVE_DATE_HEADER_INFO), f);
	unsigned char*buff = (unsigned char*)malloc(dataInfo.chkLen);
	fread(buff, 1, dataInfo.chkLen, f);
	fclose(f);

#ifdef WAVE_SOUND
	wave_init(hInfo.dwSampleRate, hInfo.wBitsPerSample, hInfo.wChannels);
#elif defined OPENAL_SOUND
	openal_init(hInfo.dwSampleRate, hInfo.wBitsPerSample, hInfo.wChannels);
#else
	DirectSound_init(hInfo.dwSampleRate, hInfo.wBitsPerSample, hInfo.wChannels);
#endif
	int e = 1;
	int current = 0;
	while (e) {
		int len = 1024 * 8;
		if (current + len > dataInfo.chkLen) {
			len = dataInfo.chkLen - current;
		}
#ifdef WAVE_SOUND
		wave_out((char *)(buff + current), len);
#elif defined OPENAL_SOUND
		openal_out((char *)(buff + current), len);
#else
		DirectSound_update((char *)(buff + current), len);
#endif
		current += len;
		if (current >= dataInfo.chkLen) {
			e = 0;
		}
	}
#ifdef WAVE_SOUND
	wave_close();
#elif defined OPENAL_SOUND
	openal_close();
#else
	DirectSound_close();
#endif

	free(buff);
}

int main() {

	testWaveOut("1.wav");


}