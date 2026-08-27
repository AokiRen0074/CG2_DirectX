#include "Audio.h"
#include <fstream>
#include <cassert>


Audio* Audio::GetInstance() {
	static Audio instance;
	return &instance;
}

/*-------------------------------------
初期化
---------------------------------------------*/
void Audio::Initialize() {
	HRESULT result;

	// XAudioエンジンのインスタンスを生成
	result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));

	// マスターボイスを生成
	result = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(result));
}

/*-----------------------------------
WAVファイルの読み込み
----------------------------------------------*/
SoundData Audio::SoundLoadWave(const char* filename) {
	// ファイルオープン
	std::ifstream file;
	file.open(filename, std::ios_base::binary);
	assert(file.is_open());

	// wavデータ読み込み
	// RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	// ファイルがRIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) { assert(0); }
	// タイプがWAVEかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) { assert(0); }

	// Formatチャンクの読み込み
	FormatChunk format = {};
	file.read((char*)&format.chunk, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) { assert(0); }
	// チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	// Dataチャンクの読み込み
	ChunkHeader data;
	while (true) {
		file.read((char*)&data, sizeof(data));
		if (strncmp(data.id, "data", 4) == 0) {
			break;
		}

		file.seekg(data.size, std::ios_base::cur);
	}

	// Dataチャンクのデータ読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	// ファイルクローズ
	file.close();

	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

/*---------------------------
音声の再生
------------------------------------*/

IXAudio2SourceVoice* Audio::SoundPlayWave(const SoundData& soundData, bool loop) {
	HRESULT result;

	// 波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (loop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	// 波形データのキューへの送信と再生開始
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	result = pSourceVoice->Start();
	assert(SUCCEEDED(result)); 

	return pSourceVoice;
}

/*--------------------------------
音声データの解放
----------------------------------------------*/
void Audio::SoundUnload(SoundData* soundData) {
	// new で確保した波形データメモリを解放する
	delete[] soundData->pBuffer;
	soundData->pBuffer = nullptr;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

/*-----------------------------
終了処理
---------------------------------*/
void Audio::Finalize() {
	if (masterVoice_) {
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}

	xAudio2_.Reset();
}

void Audio::SoundStopWave(IXAudio2SourceVoice* pVoice) {
	if (pVoice) {
		pVoice->Stop();
		pVoice->DestroyVoice();
	}
}