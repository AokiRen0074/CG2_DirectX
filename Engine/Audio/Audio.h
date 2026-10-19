#pragma once
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib") 
#include <wrl.h>
#include <string>
#include <list>


// チャンクヘッダ
struct ChunkHeader {
	char id[4];   // チャンク毎のID
	int32_t size; // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk; // "RIFF"
	char type[4];      // "WAVE"
};

// FMTチャンク
struct FormatChunk {
	ChunkHeader chunk; // "fmt "
	WAVEFORMATEX fmt;  // 波形フォーマット
};

// 音声データ
struct SoundData {
	WAVEFORMATEX wfex{};       // 波形フォーマット
	BYTE* pBuffer = nullptr;           // バッファの先頭アドレス
	unsigned int bufferSize = 0; // バッファのサイズ
};


class Audio {

private:
	Audio() = default;
	~Audio() = default;
public:

	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;


	static Audio* GetInstance();
	// 初期化
	void Initialize();

	// 音声データの読み込み
	SoundData SoundLoadWave(const char* filename);

	// WAVや圧縮音声をMedia FoundationでPCMへ変換して読み込む
	// 読み込み済みデータはSoundPlayで繰り返し再生し、最後にSoundUnloadで解放する
	SoundData SoundLoad(const char* filename);

	// 音声データの解放
	void SoundUnload(SoundData* soundData);

	// 音声の再生関数
	IXAudio2SourceVoice* SoundPlayWave(const SoundData& soundData, bool loop = false);

	// ファイル形式に依存せず、読み込み済みのPCMデータを再生する
	IXAudio2SourceVoice* SoundPlay(const SoundData& soundData, bool loop = false);



	// 鳴っている音を途中で止める関数
	void SoundStopWave(IXAudio2SourceVoice* pVoice);

	void Finalize();

private:
	// XAudio2エンジンのインスタンス
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	// マスターボイス
	IXAudio2MasteringVoice* masterVoice_ = nullptr;

	std::list<IXAudio2SourceVoice*> voices_;

	// MFStartupとMFShutdownを一度ずつ対応させる
	bool mediaFoundationInitialized_ = false;
};