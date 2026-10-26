#include "Audio.h"
#include <fstream>
#include <cassert>
#include <algorithm>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <vector>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

namespace {

void CheckAudioResult(HRESULT result, const char* operation) {
	if (FAILED(result)) {
		std::ostringstream message;
		message << operation << " failed (HRESULT 0x"
			<< std::hex << static_cast<unsigned long>(result) << ")";
		throw std::runtime_error(message.str());
	}
}

std::wstring AudioFilePath(const char* filename) {
	if (!filename || filename[0] == '\0') {
		throw std::invalid_argument("Audio filename must not be empty");
	}

	const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, nullptr, 0);

	if (length == 0) {
		throw std::invalid_argument("Audio filename must be valid UTF-8");
	}

	std::wstring path(static_cast<size_t>(length), L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, path.data(), length);
	path.pop_back();

	return path;
}

WAVEFORMATEX GetPcmFormat(IMFSourceReader* reader) {
	Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
	CheckAudioResult(reader->GetCurrentMediaType(
		MF_SOURCE_READER_FIRST_AUDIO_STREAM, &mediaType), "Get decoded audio format");

	WAVEFORMATEX* allocatedFormat = nullptr;
	UINT32 formatSize = 0;
	CheckAudioResult(MFCreateWaveFormatExFromMFMediaType(
		mediaType.Get(), &allocatedFormat, &formatSize), "Convert decoded audio format");
	std::unique_ptr<WAVEFORMATEX, decltype(&CoTaskMemFree)> format(
		allocatedFormat, &CoTaskMemFree);

	// SoundDataには拡張領域を持たせず、標準の16bit PCMに統一する
	if (formatSize < sizeof(WAVEFORMATEX) ||
		format->wFormatTag != WAVE_FORMAT_PCM || format->cbSize != 0 ||
		format->wBitsPerSample != 16 || format->nChannels == 0 ||
		format->nBlockAlign == 0 || format->nSamplesPerSec == 0) {
		throw std::runtime_error("Unsupported decoded PCM format");
	}

	return *format;
}

bool SamePcmFormat(const WAVEFORMATEX& first, const WAVEFORMATEX& second) {
	return first.wFormatTag == second.wFormatTag &&
		first.nChannels == second.nChannels &&
		first.nSamplesPerSec == second.nSamplesPerSec &&
		first.nAvgBytesPerSec == second.nAvgBytesPerSec &&
		first.nBlockAlign == second.nBlockAlign &&
		first.wBitsPerSample == second.wBitsPerSample &&
		first.cbSize == second.cbSize;
}

}

Audio* Audio::GetInstance() {
	static Audio instance;
	return &instance;
}

/*-------------------------------------
初期化
---------------------------------------------*/
void Audio::Initialize() {
	if (mediaFoundationInitialized_) {
		return;
	}

	HRESULT result;

	// XAudioエンジンのインスタンスを生成
	result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	CheckAudioResult(result, "Create XAudio2 engine");

	// マスターボイスを生成
	IXAudio2MasteringVoice* masterVoice = nullptr;
	result = xAudio2_->CreateMasteringVoice(&masterVoice);
	masterVoice_.reset(masterVoice);

	if (FAILED(result)) {
		xAudio2_.Reset();
		CheckAudioResult(result, "Create mastering voice");
	}

	// 圧縮音声を読み込むためのMedia Foundationを開始する
	result = MFStartup(MF_VERSION, MFSTARTUP_FULL);

	if (FAILED(result)) {
		masterVoice_.reset();
		xAudio2_.Reset();
		CheckAudioResult(result, "Start Media Foundation");
	}

	mediaFoundationInitialized_ = true;
}

// Media Foundationによる音声ファイルの読み込み
SoundData Audio::SoundLoad(const char* filename) {
	if (!mediaFoundationInitialized_) {
		throw std::logic_error("Audio must be initialized before loading sound");
	}

	const std::wstring path = AudioFilePath(filename);
	Microsoft::WRL::ComPtr<IMFSourceReader> reader;
	CheckAudioResult(MFCreateSourceReaderFromURL(
		path.c_str(), nullptr, &reader), "Open audio file");

	// 動画を含むファイルでも、最初の音声ストリームだけを読み込む
	CheckAudioResult(reader->SetStreamSelection(
		MF_SOURCE_READER_ALL_STREAMS, FALSE), "Deselect media streams");
	CheckAudioResult(reader->SetStreamSelection(
		MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE), "Select audio stream");

	// XAudio2で使える16bit PCMを指定し、圧縮音声のデコーダーを選択させる
	Microsoft::WRL::ComPtr<IMFMediaType> requestedType;
	CheckAudioResult(MFCreateMediaType(&requestedType), "Create PCM media type");
	CheckAudioResult(requestedType->SetGUID(
		MF_MT_MAJOR_TYPE, MFMediaType_Audio), "Set audio media type");
	CheckAudioResult(requestedType->SetGUID(
		MF_MT_SUBTYPE, MFAudioFormat_PCM), "Set PCM subtype");
	CheckAudioResult(requestedType->SetUINT32(
		MF_MT_AUDIO_BITS_PER_SAMPLE, 16), "Set PCM bit depth");
	CheckAudioResult(reader->SetCurrentMediaType(
		MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, requestedType.Get()),
		"Configure PCM decoding");

	const WAVEFORMATEX format = GetPcmFormat(reader.Get());
	std::vector<BYTE> decodedBytes;

	while (true) {
		DWORD flags = 0;
		Microsoft::WRL::ComPtr<IMFSample> sample;
		CheckAudioResult(reader->ReadSample(
			MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample),
			"Decode audio sample");

		if ((flags & MF_SOURCE_READERF_ERROR) != 0) {
			throw std::runtime_error("Audio decoder reported a stream error");
		}

		// 途中でフォーマットが変わった場合、異なるPCMを混ぜて再生しない
		if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0 &&
			!SamePcmFormat(format, GetPcmFormat(reader.Get()))) {
			throw std::runtime_error("Audio format changed while decoding");
		}

		if (sample) {
			Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
			CheckAudioResult(sample->ConvertToContiguousBuffer(
				&buffer), "Collect decoded audio buffers");

			const size_t previousSize = decodedBytes.size();
			DWORD length = 0;
			CheckAudioResult(buffer->GetCurrentLength(&length), "Get decoded sample size");

			if (length > XAUDIO2_MAX_BUFFER_BYTES - previousSize) {
				throw std::length_error("Decoded audio exceeds the XAudio2 buffer limit");
			}

			// 配列の拡張に失敗しても、Media Foundationのバッファをロックしたままにしない
			decodedBytes.resize(previousSize + length);
			BYTE* sourceBytes = nullptr;
			CheckAudioResult(buffer->Lock(&sourceBytes, nullptr, nullptr), "Lock decoded audio");

			if (length != 0) {
				std::memcpy(decodedBytes.data() + previousSize, sourceBytes, length);
			}

			CheckAudioResult(buffer->Unlock(), "Unlock decoded audio");
		}

		// 最後のサンプルにも音声データがある場合は、保存してから終了する
		if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
			break;
		}
	}

	if (decodedBytes.empty() || decodedBytes.size() % format.nBlockAlign != 0) {
		throw std::runtime_error("Decoded audio is empty or has incomplete PCM frames");
	}

	auto pcmBuffer = std::make_shared<BYTE[]>(decodedBytes.size());
	std::memcpy(pcmBuffer.get(), decodedBytes.data(), decodedBytes.size());

	SoundData soundData{};
	soundData.wfex = format;
	soundData.bufferSize = static_cast<unsigned int>(decodedBytes.size());
	soundData.pBuffer = std::move(pcmBuffer);

	return soundData;
}

IXAudio2SourceVoice* Audio::SoundPlay(const SoundData& soundData, bool loop) {
	return SoundPlayWave(soundData, loop);
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
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	// タイプがWAVEかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	// Formatチャンクの読み込み
	FormatChunk format = {};
	file.read((char*)&format.chunk, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}
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
	auto pBuffer = std::make_shared<BYTE[]>(data.size);
	file.read(reinterpret_cast<char*>(pBuffer.get()), data.size);

	// ファイルクローズ
	file.close();

	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBuffer = std::move(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

/*---------------------------
音声の再生
------------------------------------*/

IXAudio2SourceVoice* Audio::SoundPlayWave(const SoundData& soundData, bool loop) {
	if (!xAudio2_ || !soundData.pBuffer || soundData.bufferSize == 0) {
		throw std::invalid_argument("Sound playback requires an initialized engine and audio data");
	}


	for (auto it = voices_.begin(); it != voices_.end(); ) {
		XAUDIO2_VOICE_STATE state;
		it->voice->GetState(&state);
		if (state.BuffersQueued == 0) { // 再生が終わっていたら
			it->voice.reset();      // スピーカーを壊す
			it = voices_.erase(it);     // リストから削除
		}
		else {
			++it;
		}
	}

	HRESULT result;

	// 波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	std::unique_ptr<IXAudio2SourceVoice, AudioVoiceDeleter> ownedVoice(pSourceVoice);
	CheckAudioResult(result, "Create source voice");

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer.get();
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (loop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	// 波形データのキューへの送信と再生開始
	result = pSourceVoice->SubmitSourceBuffer(&buf);

	if (FAILED(result)) {
		ownedVoice.reset();
		CheckAudioResult(result, "Submit audio buffer");
	}

	result = pSourceVoice->Start();

	if (FAILED(result)) {
		ownedVoice.reset();
		CheckAudioResult(result, "Start source voice");
	} 

	voices_.push_back({ soundData.pBuffer, std::move(ownedVoice) });

	return pSourceVoice;
}

/*--------------------------------
音声データの解放
----------------------------------------------*/
void Audio::SoundUnload(SoundData* soundData) {
	if (!soundData) {
		return;
	}

	// new で確保した波形データメモリを解放する
	// 所有を外すだけにし、再生中のボイスがあればその終了まで波形を保持する
	soundData->pBuffer.reset();
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

/*-----------------------------
終了処理
---------------------------------*/
void Audio::Finalize() {
	// 再生中のボイスを破棄してから、出力先とエンジンを解放する
	for (auto& playingVoice : voices_) {
		playingVoice.voice->Stop();
	}

	voices_.clear();

	if (masterVoice_) {
		masterVoice_.reset();
	}

	xAudio2_.Reset();

	if (mediaFoundationInitialized_) {
		MFShutdown();
		mediaFoundationInitialized_ = false;
	}
}

void Audio::SoundStopWave(IXAudio2SourceVoice* pVoice) {
	auto it = std::find_if(voices_.begin(), voices_.end(), [pVoice](const PlayingVoice& playing) {
		return playing.voice.get() == pVoice;
	});

	if (it != voices_.end()) {
		it->voice->Stop();
		voices_.erase(it);
	}
}