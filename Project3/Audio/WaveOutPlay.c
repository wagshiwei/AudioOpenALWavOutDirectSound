
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
	waveinfo.wfx.wFormatTag = WAVE_FORMAT_PCM;//设置波形声音的格式
	waveinfo.wfx.nChannels = 1;//设置音频文件的通道数量
	waveinfo.wfx.nSamplesPerSec = 8000;//设置每个声道播放和记录时的样本频率
	waveinfo.wfx.nAvgBytesPerSec = 16000;//设置请求的平均数据传输率,单位byte/s。这个值对于创建缓冲大小是很有用的
	waveinfo.wfx.nBlockAlign = 2;//以字节为单位设置块对齐
	waveinfo.wfx.wBitsPerSample = 16;
	waveinfo.wfx.cbSize = 0;//额外信息的大小
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
	//CRITICAL_SECTION cs;//定义临界区对象
	InitializeCriticalSection(&waveinfo.cs);//初始化临界区
	//waveOutOpen(&waveinfo.hwo, WAVE_MAPPER, &waveinfo.wfx, (DWORD)waveinfo.wait, 0, CALLBACK_EVENT);// (DWORD_PTR)PlayCallback, 0L, CALLBACK_FUNCTION);// CALLBACK_EVENT);//打开一个给定的波形音频输出装置来进行回放
	waveOutOpen(&waveinfo.hwo, WAVE_MAPPER, &waveinfo.wfx, (DWORD_PTR)PlayCallback, (DWORD_PTR)&waveinfo.bufferCount, CALLBACK_FUNCTION);// CALLBACK_EVENT);//打开一个给定的波形音频输出装置来进行回放
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
//while (cnt>0) {//这一部分需要特别注意的是在循环回来之后不能花太长的时间去做读取数据之类的工作，不然在每个循环的间隙会有“哒哒”的噪音
	(*waveH).lpData = buff;
	(*waveH).dwBufferLength = bufsize;
	//waveinfo.wh[current].dwFlags = 0L;
	(*waveH).dwLoops = 0L;
	if ((*waveH).dwFlags&&WHDR_PREPARED) {
		waveOutUnprepareHeader(waveinfo.hwo, (waveH), sizeof(WAVEHDR));
	}
	waveOutPrepareHeader(waveinfo.hwo, (waveH), sizeof(WAVEHDR));//准备一个波形数据块用于播放
	waveOutWrite(waveinfo.hwo, (waveH), sizeof(WAVEHDR));//在音频媒体中播放第二个函数wh指定的数据
	//WaitForSingleObject(waveinfo.wait, INFINITE);


	//WaitForSingleObject(wait, INFINITE);//用来检测hHandle事件的信号状态，在某一线程中调用该函数时，线程暂时挂起，如果在挂起的INFINITE毫秒内，线程所等待的对象变为有信号状态，则该函数立即返回
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
		Sleep(500); //等待所有buffer输出完成
		r = waveOutClose(waveinfo.hwo);//关闭设备
		printf("\nwaveOutClose1 %d\n", r);
	}
	else {
		r = waveOutClose(waveinfo.hwo);//关闭设备
		printf("\nwaveOutClose2 %d\n", r);
	}
	for (int i = 0; i < WAVE_BUFFER_SIZE; i++) {
		free(waveinfo.buff[i]);
	}
	DeleteCriticalSection(&waveinfo.cs);//删除临界区
	//waveOutUnprepareHeader(waveinfo.hwo, &waveinfo.wh, sizeof(WAVEHDR));
	//sem_destroy(&waveinfo.sem);
	/*if (sem_destroy(&waveinfo.sem) != 0)
	{
		perror("@Error sem_destroy ");
	}*/
}

