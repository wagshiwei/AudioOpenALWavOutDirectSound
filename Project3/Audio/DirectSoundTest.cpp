#pragma once

/**
  2  * 最简单的DirectSound播放音频的例子（DirectSound播放PCM）
  3  * Simplest Audio Play DirectSound (DirectSound play PCM)
  4  *
  5  * 雷霄骅 Lei Xiaohua
  6  * leixiaohua1020@126.com
  7  * 中国传媒大学/数字电视技术
  8  * Communication University of China / Digital TV Technology
  9  * http://blog.csdn.net/leixiaohua1020
 10  *
 11  * 本程序使用DirectSound播放PCM音频采样数据。
 12  * 是最简单的DirectSound播放音频的教程。
 13  *
 14  * 函数调用步骤如下：
 15  *
 16  * [初始化]
 17  * DirectSoundCreate8()：创建一个DirectSound对象。
 18  * SetCooperativeLevel()：设置协作权限，不然没有声音。
 19  * IDirectSound8->CreateSoundBuffer()：创建一个主缓冲区对象。
 20  * IDirectSoundBuffer->QueryInterface(IID_IDirectSoundBuffer8..)：
 21  *            创建一个副缓冲区对象，用来存储要播放的声音数据文件。
 22  * IDirectSoundBuffer8->QueryInterface(IID_IDirectSoundNotify..)：
 23  *            创建通知对象，通知应用程序指定播放位置已经达到。
 24  * IDirectSoundNotify8->SetNotificationPositions()：设置通知位置。
 25  * IDirectSoundBuffer8->SetCurrentPosition()：设置播放的起始点。
 26  * IDirectSoundBuffer8->Play()：开始播放。
 27  *
 28  * [循环播放数据]
 29  * IDirectSoundBuffer8->Lock()：锁定副缓冲区，准备写入数据。
 30  * fread()：读取数据。
 31  * IDirectSoundBuffer8->Unlock()：解锁副缓冲区。
 32  * WaitForMultipleObjects()：等待“播放位置已经达到”的通知。
 33  *
 34  * This software plays PCM raw audio data using DirectSound.
 35  * It's the simplest tutorial about DirectSound.
 36  *
 37  * The process is shown as follows:
 38  *
 39  * [Init]
 40  * DirectSoundCreate8(): Init DirectSound object.
 41  * SetCooperativeLevel(): Must set, or we won't hear sound.
 42  * IDirectSound8->CreateSoundBuffer(): Create primary sound buffer.
 43  * IDirectSoundBuffer->QueryInterface(IID_IDirectSoundBuffer8..):
 44  *            Create secondary sound buffer.
 45  * IDirectSoundBuffer8->QueryInterface(IID_IDirectSoundNotify..):
 46  *            Create Notification object.
 47  * IDirectSoundNotify8->SetNotificationPositions():
 48  *            Set Notification Positions.
 49  * IDirectSoundBuffer8->SetCurrentPosition(): Set position to start.
 50  * IDirectSoundBuffer8->Play(): Begin to play.
 51  *
 52  * [Loop to play data]
 53  * IDirectSoundBuffer8->Lock(): Lock secondary buffer.
 54  * fread(): get PCM data.
 55  * IDirectSoundBuffer8->Unlock(): UnLock secondary buffer.
 56  * WaitForMultipleObjects(): Wait for Notifications.
 57  */
 #include <stdio.h>
 #include <stdlib.h>
 #include <windows.h>
 #include <dsound.h>
#include <vector>
#include <iostream>
#include  <mutex>
#pragma comment (lib,"dsound.lib")
#pragma comment (lib,"dxguid.lib") 
 #define MAX_AUDIO_BUF 20
 //#define BUFFERNOTIFYSIZE (1024 * 8 )
#define BUFFERNOTIFYSIZE (512 )
//192000


 static std::mutex mtx;

 WAVEFORMATEX* prepare_waveformat() {
	 int sample_rate = 8000;    //PCM sample rate
	 int channels = 1;            //PCM channel number
	 int bits_per_sample = 16;    //bits per sample
	 WAVEFORMATEX *lpwfxFormat=(WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
	 lpwfxFormat->wFormatTag = WAVE_FORMAT_PCM;
	 /* format type */
	 lpwfxFormat->nChannels = channels;
	 /* number of channels (i.e. mono, stereo...) */
	 lpwfxFormat->nSamplesPerSec = sample_rate;
	 /* sample rate */
	 lpwfxFormat->nAvgBytesPerSec = sample_rate * (bits_per_sample / 8)*channels;
	 /* for buffer estimation */
	 lpwfxFormat->nBlockAlign = (bits_per_sample / 8)*channels;
	 /* block size of data */
	 lpwfxFormat->wBitsPerSample = bits_per_sample;
	 /* number of bits per sample of mono data */
	 lpwfxFormat->cbSize = 0;
	 return lpwfxFormat;
 }


 WAVEFORMATEX* prepare_waveformat(DWORD sample_rate, WORD bits_per_sample, WORD channels) {

	 WAVEFORMATEX *lpwfxFormat = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
	 WORD        wFormatTag = WAVE_FORMAT_PCM;
	 if (sample_rate==48000) {
		 if (channels == 2) {
			 //wFormatTag = WAVE_FORMAT_48S16;
		 }
		 else if (channels == 1) {
			 //wFormatTag = WAVE_FORMAT_48M16;
		 }
		
	 }
	 else if (sample_rate == 96000) {
		 if (channels==2) {
			 //wFormatTag = WAVE_FORMAT_96S16;
		 }
		 else if(channels == 1) {
			 //wFormatTag = WAVE_FORMAT_96M16;
		 }
	 }

	 lpwfxFormat->wFormatTag = wFormatTag;
	 /* format type */
	 lpwfxFormat->nChannels = channels;
	 /* number of channels (i.e. mono, stereo...) */
	 lpwfxFormat->nSamplesPerSec = sample_rate;
	 /* sample rate */
	 lpwfxFormat->nAvgBytesPerSec = sample_rate * (bits_per_sample / 8)*channels;
	 /* for buffer estimation */
	 lpwfxFormat->nBlockAlign = (bits_per_sample / 8)*channels;
	 /* block size of data */
	 lpwfxFormat->wBitsPerSample = bits_per_sample;
	 /* number of bits per sample of mono data */
	 lpwfxFormat->cbSize = 0;
	 return lpwfxFormat;
 }

 typedef struct DirectSoundContext {
	 IDirectSound *ds = 0;
	 IDirectSoundBuffer *buffer2 = NULL;    //used to manage sound buffers.
	 IDirectSoundBuffer *buffer = NULL;
	 IDirectSoundNotify *notify = 0;
	 DSBPOSITIONNOTIFY notify_pos[MAX_AUDIO_BUF];
	 HANDLE event[MAX_AUDIO_BUF];
	 bool idle[MAX_AUDIO_BUF];
	 HANDLE tid;
	 bool threadBegin;
	 int currentPoint;
 }DSContext;

 static DSContext dsContext;
 static HWND hWnd;

 void DirectSound_SetHwnd(HWND hwnd) {
	 hWnd = hwnd;
 }

 BOOL CALLBACK enumerateCallback(LPGUID guid,
	 LPCWSTR deviceDescription,
	 LPCWSTR deviceDriverModule,
	 LPVOID context)
 {
	 //Q_UNUSED(context);

	 //  if primary device, skip it
	 if (guid == nullptr)        return TRUE;

	
 }

 extern "C" {
	 DWORD WINAPI playerThreadImpl(LPVOID params);
	 int DirectSound_init(DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels) {
#if 0
		 IDirectSound *m_pDS = 0;
		 IDirectSoundBuffer *m_pDSBuffer8 = NULL;    //used to manage sound buffers.
		 IDirectSoundBuffer *m_pDSBuffer = NULL;
		 IDirectSoundNotify *m_pDSNotify = 0;
		 DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];
		 HANDLE m_event[MAX_AUDIO_BUF];
#endif
		 //SetConsoleTitle(TEXT("Simplest Audio Play DirectSound"));//Console Title
			  //Init DirectSound
		 if (DirectSoundEnumerateW(enumerateCallback, nullptr) != DS_OK) {
			
		 }

		 if (FAILED(DirectSoundCreate(NULL, &(dsContext.ds), NULL)))
			 return FALSE;

		 DSCAPS deviceCapability = { sizeof(deviceCapability) };
		 if (dsContext.ds->GetCaps(&deviceCapability) != DS_OK) {
			 std::wcout << L"[error] GetCaps call error!";
			 return TRUE;
		 }
#if 0
		 if (FAILED(m_pDS->SetCooperativeLevel(hwnd, DSSCL_NORMAL))) ///FindWindow(NULL, TEXT("Simplest Audio Play DirectSound"))
		 {
			 return FALSE;
		 }
#endif
		 //hWnd;
		 if (hWnd) {
			 IDirectSound_SetCooperativeLevel(dsContext.ds, hWnd, DSSCL_PRIORITY/*DSSCL_NORMAL*/);
		 }else 
		 if ((hWnd = GetForegroundWindow()) || (hWnd = GetDesktopWindow()) || (hWnd = GetConsoleWindow())) {
			 IDirectSound_SetCooperativeLevel(dsContext.ds, hWnd, DSSCL_NORMAL);
		 }


		 DSBUFFERDESC dsbd;
		 memset(&dsbd, 0, sizeof(dsbd));
		 //dsbd.dwSize = sizeof(dsbd);
		 WAVEFORMATEX*  format = prepare_waveformat(nSamplesPerSec, wBitsPerSample, nChannels);

		 dsbd.dwSize = sizeof(DSBUFFERDESC);
		 dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
		 dsbd.dwBufferBytes = 0;
		 dsbd.lpwfxFormat = nullptr;

		 //dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
		 //dsbd.dwBufferBytes = MAX_AUDIO_BUF * BUFFERNOTIFYSIZE;

		 //Creates a sound buffer object to manage audio samples. 
		 HRESULT hr1 = IDirectSound_CreateSoundBuffer(dsContext.ds, &dsbd, &(dsContext.buffer), NULL);
		 if (FAILED(hr1)) {
			 return FALSE;
		 }
#if 0
		 if (FAILED(m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&m_pDSBuffer8))) {
			 return FALSE;
		 }


		 if (FAILED(IDirectSoundBuffer_QueryInterface(dsContext.buffer, IID_IDirectSoundBuffer, (LPVOID *)&(dsContext.buffer2), NULL))) {
			 return FALSE;
		 }
#endif	


#if 1
		 dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2; //| DSBCAPS_CTRLFREQUENCY
		 dsbd.dwBufferBytes = MAX_AUDIO_BUF * BUFFERNOTIFYSIZE;
		 dsbd.lpwfxFormat = format;

		 hr1 = IDirectSound_CreateSoundBuffer(dsContext.ds, &dsbd, (LPDIRECTSOUNDBUFFER *)&(dsContext.buffer2), NULL);
		 if (FAILED(hr1)) {
			 return FALSE;
		 }
#endif
		
		 //Get IDirectSoundNotify8
		 //IDirectSoundNotify_QueryInterface(m_pDSBuffer8, (LPVOID*)&m_pDSNotify);
#if 0
		 if (FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify))) {
			 return FALSE;

		 }
#endif

		 free(dsbd.lpwfxFormat);
		 IDirectSoundBuffer_QueryInterface(dsContext.buffer2, IID_IDirectSoundNotify, (LPVOID*)&(dsContext.notify));
		 //IDirectSoundBuffer_SetFrequency(dsContext.buffer2, nSamplesPerSec);

		 for (int i = 0; i < MAX_AUDIO_BUF; i++) {
			 dsContext.notify_pos[i].dwOffset = ((i+1) * BUFFERNOTIFYSIZE)-1;
			 dsContext.event[i] = ::CreateEvent(NULL, false, false, NULL);
			 dsContext.notify_pos[i].hEventNotify = dsContext.event[i];

		 }
		 dsContext.notify->SetNotificationPositions(MAX_AUDIO_BUF, dsContext.notify_pos);
		 dsContext.notify->Release();

		 //Start Playing
		/* BOOL isPlaying = TRUE;
		 LPVOID buf = NULL;
		 DWORD  buf_len = 0;
		 DWORD res = WAIT_OBJECT_0;
		 DWORD offset = BUFFERNOTIFYSIZE;*/

		 dsContext.buffer2->SetCurrentPosition(0);
		 dsContext.buffer2->Play(0, 0, DSBPLAY_LOOPING);
		 //Loop
		 for (int i = 0; i < MAX_AUDIO_BUF;i++) {
			 dsContext.idle[i] = true;
		 }
		 dsContext.threadBegin = true;
		 dsContext.tid = ::CreateThread(NULL, 0, playerThreadImpl, nullptr, 0, NULL);
		 dsContext.currentPoint =0 ;
	 }

	 DWORD WINAPI playerThreadImpl(LPVOID params) {
		 DWORD res = WAIT_OBJECT_0;
		 while (dsContext.threadBegin) {
			 res = WaitForMultipleObjects(MAX_AUDIO_BUF, dsContext.event, FALSE, INFINITE);

			 if ((res >= WAIT_OBJECT_0) && (res <= WAIT_OBJECT_0 + MAX_AUDIO_BUF)) {
				 mtx.lock();
				 dsContext.idle[res- WAIT_OBJECT_0] = true;
				 mtx.unlock();
			 }
		 }
		 return NULL;
	 }

	 void DirectSound_update(void*data, long size) {
		 BOOL isPlaying = TRUE;
		 LPVOID buf = NULL;
		 DWORD  buf_len = 0;
		 DWORD res = WAIT_OBJECT_0;
		 DWORD offset = 0;// BUFFERNOTIFYSIZE;

		 /* dsContext.buffer2->SetCurrentPosition(0);
		  dsContext.buffer2->Play(0, 0, DSBPLAY_LOOPING);*/
		  //Loop
		 long currentSize = 0;
		 while (isPlaying) {
			 
			

			 //for (int i = 0; i < MAX_AUDIO_BUF; i++) {
				 if (dsContext.idle[dsContext.currentPoint]) {
					 //offset = buf_len;
					 dsContext.buffer2->Lock(dsContext.currentPoint* BUFFERNOTIFYSIZE, BUFFERNOTIFYSIZE, &buf, &buf_len, NULL, NULL, 0); //DSBLOCK_FROMWRITECURSOR DSBLOCK_ENTIREBUFFER

					 // 如果是实时音频播放，那么下面的数据就可以把内存中buf_len大小的数据复制到buf指向的地址即可
					 /*for (int i = 0; i < buf_len; i++) {
						 ((UCHAR*)buf)[i] = rand() % 255;
					 }*/

					 int len = size - currentSize;
					 if (len > buf_len) {
						 len = buf_len;
					 }
					 memset(buf, 0, buf_len);
					 memcpy(buf, ((unsigned char*)data) + currentSize, len);

					 currentSize += len;
					 //offset += buf_len;
					 //offset %= (BUFFERNOTIFYSIZE * MAX_AUDIO_BUF);
					 //printf("this is %7d of buffer\n", offset);
					 dsContext.buffer2->Unlock(buf, buf_len, NULL, 0);
					 dsContext.idle[dsContext.currentPoint] = false;
					 dsContext.currentPoint = (dsContext.currentPoint + 1) % MAX_AUDIO_BUF;
					 if (currentSize >= size) {
						 
						 isPlaying = false;
						 break;
					 }
					
				 }
				 else {
					 //Sleep(10);
					 std::this_thread::yield();
				 }
			 //}
			
			 //res = WaitForMultipleObjects(MAX_AUDIO_BUF, dsContext.event, FALSE, INFINITE);

			 if ((res >= WAIT_OBJECT_0) && (res <= WAIT_OBJECT_0 + 3)) {
				 
			 }
			 //res = WaitForMultipleObjects(MAX_AUDIO_BUF, dsContext.event, FALSE, INFINITE);

		 }

	 }


	 void DirectSound_close() {
		 dsContext.threadBegin = false;
		 CloseHandle(dsContext.tid);
		 if (dsContext.buffer) {
			 IDirectSoundBuffer_Release(dsContext.buffer);
			 dsContext.buffer = NULL;
		 }
		 /*辅助缓冲*/
		 if (dsContext.buffer2) {
			 IDirectSoundBuffer_Stop(dsContext.buffer2);
			 IDirectSoundBuffer_Release(dsContext.buffer2);
			 dsContext.buffer2 = NULL;
		 }
		 /*声卡设备对象*/
		 if (dsContext.ds) {
			 IDirectSound_Release(dsContext.ds);
			 dsContext.ds = NULL;
		 }
		 /*通知事件*/
		 for (size_t i = 0; i < sizeof(dsContext.event) / sizeof(dsContext.event[0]); i++) {
			 if (dsContext.event[i]) {
				 CloseHandle(dsContext.event[i]);
				 dsContext.event[i] = NULL;
			 }
		 }

	 }

 }

int testDirectSound(HWND hwnd)//int argc, char * argv[]
{
	int i;
#if 0
	FILE * fp;
	if ((fp = fopen("../out.pcm", "rb")) == NULL) {
		 printf("cannot open this file\n");
		 return -1;
	}
#endif	

	IDirectSound8 *m_pDS = 0;
	IDirectSoundBuffer8 *m_pDSBuffer8 = NULL;    //used to manage sound buffers.
	IDirectSoundBuffer *m_pDSBuffer = NULL;
	IDirectSoundNotify8 *m_pDSNotify = 0;
	DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];
	HANDLE m_event[MAX_AUDIO_BUF];
	
	//SetConsoleTitle(TEXT("Simplest Audio Play DirectSound"));//Console Title
	     //Init DirectSound
	if (FAILED(DirectSoundCreate8(NULL, &m_pDS, NULL)))
		return FALSE;
#if 0
	if (FAILED(m_pDS->SetCooperativeLevel(hwnd, DSSCL_NORMAL))) ///FindWindow(NULL, TEXT("Simplest Audio Play DirectSound"))
	{
		return FALSE;
	}
#endif
	IDirectSound_SetCooperativeLevel(m_pDS, hwnd, DSSCL_NORMAL);
		
	DSBUFFERDESC dsbd;
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwBufferBytes = MAX_AUDIO_BUF * BUFFERNOTIFYSIZE;
	dsbd.lpwfxFormat = prepare_waveformat();
	
	//Creates a sound buffer object to manage audio samples. 
	HRESULT hr1;
	if (FAILED(m_pDS->CreateSoundBuffer(&dsbd, &m_pDSBuffer, NULL))) {
		return FALSE;
	}
#if 0
	if (FAILED(m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&m_pDSBuffer8))) {
		return FALSE;
	}
	if (FAILED(IDirectSound_CreateSoundBuffer(m_pDS, &dsbd, (LPDIRECTSOUNDBUFFER *)&m_pDSBuffer8, NULL))) {
		return FALSE;
	}
#endif

	if (FAILED(IDirectSoundBuffer_QueryInterface(m_pDSBuffer, IID_IDirectSoundBuffer, (LPVOID *)&m_pDSBuffer8, NULL))) {
		return FALSE;
	}
	//Get IDirectSoundNotify8
	//IDirectSoundNotify_QueryInterface(m_pDSBuffer8, (LPVOID*)&m_pDSNotify);
#if 0
	if (FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify))) {
		return FALSE;
		
	}
#endif
	IDirectSoundBuffer_QueryInterface(m_pDSBuffer8, IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify);

	for (i = 0; i < MAX_AUDIO_BUF; i++) {
		m_pDSPosNotify[i].dwOffset = i * BUFFERNOTIFYSIZE;
		m_event[i] = ::CreateEvent(NULL, false, false, NULL);
		m_pDSPosNotify[i].hEventNotify = m_event[i];
		
	}
	m_pDSNotify->SetNotificationPositions(MAX_AUDIO_BUF, m_pDSPosNotify);
	m_pDSNotify->Release();
	
		     //Start Playing
	BOOL isPlaying = TRUE;
	LPVOID buf = NULL;
	DWORD  buf_len = 0;
	DWORD res = WAIT_OBJECT_0;
	DWORD offset = BUFFERNOTIFYSIZE;
	
	m_pDSBuffer8->SetCurrentPosition(0);
	m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
	    //Loop
	while (isPlaying) {
		 if ((res >= WAIT_OBJECT_0) && (res <= WAIT_OBJECT_0 + 3)) {
			m_pDSBuffer8->Lock(offset, BUFFERNOTIFYSIZE, &buf, &buf_len, NULL, NULL, 0);
			
			// 如果是实时音频播放，那么下面的数据就可以把内存中buf_len大小的数据复制到buf指向的地址即可
			for (int i = 0; i < buf_len;i++) {
				((UCHAR*)buf)[i] = rand() % 255;
			}
#if 0
			if (fread(buf, 1, buf_len, fp) != buf_len) {
			//File End
			//Loop:
				fseek(fp, 0, SEEK_SET);
				fread(buf, 1, buf_len, fp);
				                //Close:
					                //isPlaying=0;
					
			}
#endif
		    offset += buf_len;
		    offset %= (BUFFERNOTIFYSIZE * MAX_AUDIO_BUF);
		    printf("this is %7d of buffer\n", offset);
			m_pDSBuffer8->Unlock(buf, buf_len, NULL, 0);
			
		 }
		 res = WaitForMultipleObjects(MAX_AUDIO_BUF, m_event, FALSE, INFINITE);
		 
	}
	return 0;
}