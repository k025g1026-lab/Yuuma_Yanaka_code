// Sound.h
#pragma once
#include <Novice.h>

class Sound {
public:
    static int titleBgm;           // title.mp3 のみ
    static bool isBgmPlaying;

    static void LoadAllSounds();   // title.mp3 だけ読み込み
    static void PlayTitleBGM();    // タイトルBGMを再生（ループ）
    static void StopBGM();         // 停止（必要に応じて）
};