
#include <al.h>
#include <alc.h>
#include "iostream"
#include "vector"
#include <windows.h>
#include "TestOpenAL.h"
/*
struct WAVE_Data {
	char subChunkID[4]; //should contain the word data
	long subChunk2Size; //Stores the size of the data block
};

struct WAVE_Format {
	char subChunkID[4];
	long subChunkSize;
	short audioFormat;
	short numChannels;
	long sampleRate;
	long byteRate;
	short blockAlign;
	short bitsPerSample;
};

struct RIFF_Header {
	char chunkID[4];
	long chunkSize;//size not including chunkSize or chunkID
	char format[4];
};

*/


static FILE* soundFile = NULL;


bool TestOpenAL::initWaveFile(const char*  filename) {
	try {
		soundFile = fopen(filename, "rb");
		if (!soundFile)
			throw (filename);

		// Read in the first chunk into the struct
		fread(&riff_header, sizeof(RIFF_Header), 1, soundFile);

		//check for RIFF and WAVE tag in memeory
		if ((riff_header.chunkID[0] != 'R' ||
			riff_header.chunkID[1] != 'I' ||
			riff_header.chunkID[2] != 'F' ||
			riff_header.chunkID[3] != 'F') ||
			(riff_header.format[0] != 'W' ||
				riff_header.format[1] != 'A' ||
				riff_header.format[2] != 'V' ||
				riff_header.format[3] != 'E'))
			throw ("Invalid RIFF or WAVE Header");

		//Read in the 2nd chunk for the wave info
		fread(&wave_format, sizeof(WAVE_Format), 1, soundFile);
		//check for fmt tag in memory
		if (wave_format.subChunkID[0] != 'f' ||
			wave_format.subChunkID[1] != 'm' ||
			wave_format.subChunkID[2] != 't' ||
			wave_format.subChunkID[3] != ' ')
			throw ("Invalid Wave Format");

		//check for extra parameters;
		if (wave_format.subChunkSize > 16)
			fseek(soundFile, sizeof(short), SEEK_CUR);

		//Read in the the last byte of data before the sound file
		fread(&wave_data, sizeof(WAVE_Data), 1, soundFile);
		//check for data tag in memory
		if (wave_data.subChunkID[0] != 'd' ||
			wave_data.subChunkID[1] != 'a' ||
			wave_data.subChunkID[2] != 't' ||
			wave_data.subChunkID[3] != 'a') {
			throw ("Invalid data header");
		}
		
		//ALenum format;
		//Now we set the variables that we passed in with the
		//data from the structs
		//*size = wave_data.subChunk2Size;
		//*frequency = wave_format.sampleRate;
		//The format is worked out by looking at the number of
		//channels and the bits per sample.
		if (wave_format.numChannels == 1) {
			if (wave_format.bitsPerSample == 8)
				format = AL_FORMAT_MONO8;
			else if (wave_format.bitsPerSample == 16)
				format = AL_FORMAT_MONO16;
		}
		else if (wave_format.numChannels == 2) {
			if (wave_format.bitsPerSample == 8)
				format = AL_FORMAT_STEREO8;
			else if (wave_format.bitsPerSample == 16)
				format = AL_FORMAT_STEREO16;
		}
		
		//errorCheck();
		//clean up and return true if successful
		currentSize = 0;
		sampleRate = wave_format.sampleRate;
		return true;
	}
	catch (std::string error) {
		//our catch statement for if we throw a string
		std::cout << error.c_str() << " : trying to load "
			<< filename << std::endl;
		//clean up memory if wave loading fails
		if (soundFile != NULL)
			fclose(soundFile);
		//return false to indicate the failure to load wave
		return false;
	}
}

int TestOpenAL::updateWaveBuffer(ALuint * buffer,void *data,long size) {
	alBufferData(*buffer, format, (void*)data, size/*wave_data.subChunk2Size*/, sampleRate);
	//Allocate memory for data
	//unsigned char* data = (unsigned char*)malloc(wave_data.subChunk2Size); // new unsigned char[wave_data.subChunk2Size];

	// Read in the sound data into the soundData variable
	return true;
}

int TestOpenAL::updateWaveBuffer(ALuint * buffer) {
	char data[4096];
	//create our openAL buffer and check for success
	//alGenBuffers(1, buffer);
	//errorCheck();
	//now we put our data into the openAL buffer and
	//check for success
	long size= 4096;
	int len = 0;
	if (!soundFile) {
		return false;
	}
	if (currentSize) {
	
	}
	if ((len=fread(data, 1, size, soundFile))>0) {
		//throw ("error loading WAVE data into struct!");
		size = len;
	}
	else {
		return false;
	}

	alBufferData(*buffer, format, (void*)data,size/*wave_data.subChunk2Size*/, wave_format.sampleRate);
	//Allocate memory for data
	//unsigned char* data = (unsigned char*)malloc(wave_data.subChunk2Size); // new unsigned char[wave_data.subChunk2Size];

	// Read in the sound data into the soundData variable
	return true;
}

int TestOpenAL::updateWaveBuffer(BOOL forced) {
	if (forced) {
		for (int i = 0; i < 3; i++) {
			if (updateWaveBuffer(&(alBuffer[i]))) {

				alSourceQueueBuffers(source, 1, &(alBuffer[i]));
			}
		}
		return true;
	}

	ALint stateVaue = 0;
	int m_numprocessed;
	int m_numqueued;
	bool isPlay = true;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &m_numprocessed);
	//获取缓存队列，缓存的队列数量  
	alGetSourcei(source, AL_BUFFERS_QUEUED, &m_numqueued);

	//获取播放状态，是不是正在播放  
	alGetSourcei(source, AL_SOURCE_STATE, &stateVaue);
	//Sleep(10);
	if (m_numprocessed > 0) {
		ALuint alBuffer1;
		//alSourcei(source, AL_BUFFERS_QUEUED, alBuffer);
		alSourceUnqueueBuffers(source, 1, &alBuffer1);
		//for (int i = 0; i < 3; i++) {
		if (updateWaveBuffer(&alBuffer1)) {

			alSourceQueueBuffers(source, 1, &(alBuffer1));
		}
		else {
			isPlay = false;
		}
		//}
	}
	else  if (m_numqueued == 0) {
		isPlay = false;
	}
	return isPlay;
}

void TestOpenAL::closeWaveFile() {
	if (soundFile!=nullptr) {
		fclose(soundFile);
		soundFile = nullptr;
	}
}

bool TestOpenAL::initSilentBuffer() {
	char data[512];
	
	long size = 512;
	memset(data,0, 512);
	for (int i = 0; i < 3; i++) {
		if (updateWaveBuffer(&(alBuffer[i]),data, 512)) {

			alSourceQueueBuffers(source, 1, &(alBuffer[i]));
		}
	}
	alSourcePlay(source);
	return true;
}

int TestOpenAL::updateBuffer(void*data, long size) {
	ALint stateVaue = 0;
	int m_numprocessed;
	int m_numqueued;
	bool isPlay = true;
	long currentSize = 0;
	while (isPlay) {
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &m_numprocessed);
		alGetSourcei(source, AL_BUFFERS_QUEUED, &m_numqueued);
		alGetSourcei(source, AL_SOURCE_STATE, &stateVaue);
		
		if (m_numprocessed > 0) {
			if (currentSize >= size) {
				isPlay = false;
				break;
			}
			for (int i = 0; i < m_numprocessed;i++) {
				if (currentSize >= size) {
					isPlay = false;
					break;
				}
				ALuint alBuffer1;
				//alSourcei(source, AL_BUFFERS_QUEUED, alBuffer);
				int len = 4096;
				if (currentSize+len>size) {
					len = size - currentSize;
				}
				alSourceUnqueueBuffers(source, 1, &alBuffer1);
				updateWaveBuffer(&alBuffer1, ((unsigned char*)data) + currentSize, len);
				currentSize += len;
				alSourceQueueBuffers(source, 1, &(alBuffer1));

				
			}

			if (stateVaue == AL_PLAYING) {
				printf("");
			}
			else {
				printf("openal updateBuffer stop");
				alSourcePlay(source);
			}
		}
		Sleep(10);
	}

	return true;
}



void testOpenAL()
{
	
	TestOpenAL testOpenAL;
	testOpenAL.initWaveFile("1.wav");
	

	testOpenAL.updateWaveBuffer(true);
	alSourcePlay(testOpenAL.source);

	
	while (testOpenAL.updateWaveBuffer()) {
		printf("alSourcePlay  ");
		Sleep(10);

		
	}

	testOpenAL.closeWaveFile();

}

void cleanAL() {
	

}




static TestOpenAL *openAL;
extern "C" {

//#include "TestOpenALC.h"

	void openal_init(DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels) {
		openAL = new TestOpenAL();
		openAL->initFormat(nChannels, wBitsPerSample, nSamplesPerSec);
		openAL->initSilentBuffer();

	}

	void openal_out(char *buf, int bufsize) {
		openAL->updateBuffer(buf, bufsize);
	}

	void  openal_close() {
		delete openAL;
	}

}


//————————————————
//版权声明：本文为CSDN博主「GreenArrowMan」的原创文章，遵循 CC 4.0 BY - SA 版权协议，转载请附上原文出处链接及本声明。
//原文链接：https ://blog.csdn.net/u011417605/article/details/49666465