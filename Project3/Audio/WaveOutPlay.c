
#include <windows.h>
#include <mmsystem.h>
#include "mmreg.h"
#include "mmeapi.h"

#include <time.h>
//#include "../ffmpeg/pthread.h"
//#include "../ffmpeg/semaphore.h"
#include <stdio.h>

#pragma comment(lib, "winmm.lib")
#define WAVE_BUFFER_SIZE 3

typedef struct WaveInfo {
	HWAVEOUT        hwo;
	WAVEHDR         wh[WAVE_BUFFER_SIZE];
	WAVEFORMATEX    wfx;
	HANDLE          wait;
	int              bufferCount;
	int            index;
	CRITICAL_SECTION cs;
	void *buff[WAVE_BUFFER_SIZE];
	//pthread_mutex_t counter_mutex;
	//sem_t sem;
}WaveInfo;

static WaveInfo waveinfo;
//static LPCRITICAL_SECTION waveCriticalSection;
void CALLBACK PlayCallback(HWAVEOUT hwaveout, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
	if (uMsg != WOM_DONE) {
		return;
	}
	int* freeBlockCounter = (int*)dwInstance;
	//pthread_mutex_lock(&waveinfo.counter_mutex);
	EnterCriticalSection(&waveinfo.cs);
	waveinfo.bufferCount--;
	//*freeBlockCounter--;
	//printf("PlayCallback WOM_DONE \n");

	if (waveinfo.bufferCount < WAVE_BUFFER_SIZE) {
		/*int ret;
		if (0 == sem_getvalue(&waveinfo.sem, &ret)) {
			if (ret==0) {
				sem_post(&waveinfo.sem);
			}
		}
		else */
		{
			//sem_post(&waveinfo.sem);
		}
		SetEvent(waveinfo.wait);

	}
	LeaveCriticalSection(&waveinfo.cs);
	//pthread_mutex_unlock(&waveinfo.counter_mutex);

}




 void wave_init(DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels) {
#if 0
	waveinfo.wfx.wFormatTag = WAVE_FORMAT_PCM;//���ò��������ĸ�ʽ
	waveinfo.wfx.nChannels = 1;//������Ƶ�ļ���ͨ������
	waveinfo.wfx.nSamplesPerSec = 8000;//����ÿ���������źͼ�¼ʱ������Ƶ��
	waveinfo.wfx.nAvgBytesPerSec = 16000;//���������ƽ�����ݴ�����,��λbyte/s�����ֵ���ڴ��������С�Ǻ����õ�
	waveinfo.wfx.nBlockAlign = 2;//���ֽ�Ϊ��λ���ÿ����
	waveinfo.wfx.wBitsPerSample = 16;
	waveinfo.wfx.cbSize = 0;//������Ϣ�Ĵ�С
#endif 
	waveinfo.wfx.wFormatTag = WAVE_FORMAT_PCM;
	waveinfo.wfx.nChannels = nChannels;
	waveinfo.wfx.nSamplesPerSec = nSamplesPerSec;
	waveinfo.wfx.wBitsPerSample = wBitsPerSample;
	waveinfo.wfx.cbSize = 0;
	waveinfo.wfx.nBlockAlign = waveinfo.wfx.wBitsPerSample * waveinfo.wfx.nChannels / 8;
	waveinfo.wfx.nAvgBytesPerSec = waveinfo.wfx.nChannels * waveinfo.wfx.nSamplesPerSec * waveinfo.wfx.wBitsPerSample / 8;

	waveinfo.wait = CreateEvent(NULL, 0, 0, NULL);
	//waveinfo.counter_mutex = PTHREAD_MUTEX_INITIALIZER;
	//pthread_mutex_init(&waveinfo.counter_mutex, NULL);
	//sem_init(&waveinfo.sem, 0,0);

	waveinfo.index = 0;
	waveinfo.bufferCount = 0;
	for (int i = 0; i < WAVE_BUFFER_SIZE; i++) {
		waveinfo.buff[i] = malloc(1024 * 8);
	}
	//CRITICAL_SECTION cs;//�����ٽ�������
	InitializeCriticalSection(&waveinfo.cs);//��ʼ���ٽ���
	//waveOutOpen(&waveinfo.hwo, WAVE_MAPPER, &waveinfo.wfx, (DWORD)waveinfo.wait, 0, CALLBACK_EVENT);// (DWORD_PTR)PlayCallback, 0L, CALLBACK_FUNCTION);// CALLBACK_EVENT);//��һ�������Ĳ�����Ƶ���װ�������лط�
	waveOutOpen(&waveinfo.hwo, WAVE_MAPPER, &waveinfo.wfx, (DWORD_PTR)PlayCallback, (DWORD_PTR)&waveinfo.bufferCount, CALLBACK_FUNCTION);// CALLBACK_EVENT);//��һ�������Ĳ�����Ƶ���װ�������лط�
}



 void wave_out(char *buf, int bufsize) {

	//int dolenght = 0;
	//int playsize = bufsize;
	//int cnt = bufsize;
	int current = (waveinfo.index);
	waveinfo.index = (waveinfo.index + 1) % WAVE_BUFFER_SIZE;
	WAVEHDR *waveH = &waveinfo.wh[current];
	while (waveinfo.bufferCount >= WAVE_BUFFER_SIZE) {
		//Sleep(10);
		//sem_wait(&waveinfo.sem);

		WaitForSingleObject(waveinfo.wait, INFINITE);
	}


	void * buff = waveinfo.buff[current];
	memcpy(buff, buf, bufsize);

	//pthread_mutex_lock(&waveinfo.counter_mutex);
	EnterCriticalSection(&waveinfo.cs);
	waveinfo.bufferCount++;
	LeaveCriticalSection(&waveinfo.cs);
	//pthread_mutex_unlock(&waveinfo.counter_mutex);
//while (cnt>0) {//��һ������Ҫ�ر�ע�������ѭ������֮���ܻ�̫����ʱ��ȥ����ȡ����֮��Ĺ�������Ȼ��ÿ��ѭ���ļ�϶���С����ա�������
	(*waveH).lpData = buff;
	(*waveH).dwBufferLength = bufsize;
	//waveinfo.wh[current].dwFlags = 0L;
	(*waveH).dwLoops = 0L;
	if ((*waveH).dwFlags&&WHDR_PREPARED) {
		waveOutUnprepareHeader(waveinfo.hwo, (waveH), sizeof(WAVEHDR));
	}
	waveOutPrepareHeader(waveinfo.hwo, (waveH), sizeof(WAVEHDR));//׼��һ���������ݿ����ڲ���
	waveOutWrite(waveinfo.hwo, (waveH), sizeof(WAVEHDR));//����Ƶý���в��ŵڶ�������whָ��������
	//WaitForSingleObject(waveinfo.wait, INFINITE);


	//WaitForSingleObject(wait, INFINITE);//�������hHandle�¼����ź�״̬����ĳһ�߳��е��øú���ʱ���߳���ʱ��������ڹ����INFINITE�����ڣ��߳����ȴ��Ķ����Ϊ���ź�״̬����ú�����������
	/*dolenght = dolenght + playsize;
	cnt = cnt - playsize;*/
	//}
}

 void  wave_close() {
	while (waveinfo.bufferCount >= WAVE_BUFFER_SIZE) {
		Sleep(10);
	}
	//pthread_mutex_lock(&waveinfo.counter_mutex);
	//pthread_mutex_unlock(&waveinfo.counter_mutex);
	//WaitForSingleObject(waveinfo.wait, INFINITE);
	for (int i = 0; i < WAVE_BUFFER_SIZE; i++) {
		if (waveinfo.wh[i].dwFlags&&WHDR_PREPARED) {
			waveOutUnprepareHeader(waveinfo.hwo, &waveinfo.wh[i], sizeof(WAVEHDR));
		}
	}
	waveinfo.index = 0;
	waveinfo.bufferCount = 0;

	CloseHandle(waveinfo.wait);
	//pthread_mutex_destroy(&waveinfo.counter_mutex);
	int r = waveOutReset(waveinfo.hwo);
	if (!r)
	{
		Sleep(500); //�ȴ�����buffer������
		r = waveOutClose(waveinfo.hwo);//�ر��豸
		printf("\nwaveOutClose1 %d\n", r);
	}
	else {
		r = waveOutClose(waveinfo.hwo);//�ر��豸
		printf("\nwaveOutClose2 %d\n", r);
	}
	for (int i = 0; i < WAVE_BUFFER_SIZE; i++) {
		free(waveinfo.buff[i]);
	}
	DeleteCriticalSection(&waveinfo.cs);//ɾ���ٽ���
	//waveOutUnprepareHeader(waveinfo.hwo, &waveinfo.wh, sizeof(WAVEHDR));
	//sem_destroy(&waveinfo.sem);
	/*if (sem_destroy(&waveinfo.sem) != 0)
	{
		perror("@Error sem_destroy ");
	}*/
}

