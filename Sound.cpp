// Sound.cpp
#include "Sound.h"

// 静的メンバの実体定義
int Sound::titleBgm = -1;
bool Sound::isBgmPlaying = false;

// 音声読み込み（title.mp3 のみ）
void Sound::LoadAllSounds() {
    titleBgm = Novice::LoadAudio("./sounds/title.mp3");
}

// titleBGM をループ再生（同じなら何もしない）
void Sound::PlayTitleBGM() {
    if (titleBgm == -1) return;
    if (isBgmPlaying) return;  // すでに再生中ならスルー

    Novice::PlayAudio(titleBgm, true, 0.5f);  // true=ループ, 音量0.5
    isBgmPlaying = true;
}

// BGM停止（必要なら呼ぶ）
void Sound::StopBGM() {
    if (titleBgm == -1) return;
    Novice::StopAudio(titleBgm);
    isBgmPlaying = false;
}