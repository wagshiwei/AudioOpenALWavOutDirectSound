#pragma once

#include <al.h>
#include <alc.h>
//#include "iostream"
//#include "vector"


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

class TestOpenAL {

public :
	TestOpenAL() {
		device = alcOpenDevice(NULL);
		context = alcCreateContext(device, NULL);
		alcMakeContextCurrent(context);

		alGenSources(1, &source);
		//alSourcei(source, AL_BUFFER, alBuffer);
		alGenBuffers(3, alBuffer);
	}

	~TestOpenAL() {
		alDeleteSources(1, &source);
		alDeleteBuffers(3, alBuffer);
		alcDestroyContext(context);
		alcCloseDevice(device);
	}

	bool initWaveFile(const char* filename);
	bool initSilentBuffer();
	int updateWaveBuffer(ALuint * buffer, void *data, long size);
	int updateWaveBuffer(ALuint * buffer);
	int updateWaveBuffer(BOOL forced=false);

	int updateBuffer(void*data,long size);
	void closeWaveFile();
	void initFormat(ALsizei frequent, ALenum format) { this->sampleRate = frequent; this->format = format; }

	void initFormat(short channel, short bitsPerSample,long sampleRate) {
		if (channel == 1) {
			if (bitsPerSample == 8)
				format = AL_FORMAT_MONO8;
			else if (bitsPerSample == 16)
				format = AL_FORMAT_MONO16;
		}
		else if (channel == 2) {
			if (bitsPerSample == 8)
				format = AL_FORMAT_STEREO8;
			else if (bitsPerSample == 16)
				format = AL_FORMAT_STEREO16;
		}
		this->sampleRate = sampleRate; this->format = format;
	}

	ALCdevice * device;
	ALCcontext *context;

	ALuint alBuffer[3];
	ALuint source;
	//ALsizei size2;
	ALsizei sampleRate;

	 WAVE_Format wave_format;
	 RIFF_Header riff_header;
	 WAVE_Data wave_data;
	
	 ALenum format;
	 long currentSize;
};