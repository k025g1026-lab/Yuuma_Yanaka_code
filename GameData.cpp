#include "GameData.h"
// ── 定数の実体定義 ──
const char* kWindowTitle = "GC1A_08_ヤナカ_ユウマ_ブラックリワインド";
const int WINDOW_W = 1280;
const int WINDOW_H = 720;
const float DT = 1.f / 60.f;
const float SCROLL_SPEED = 180.f * DT;
const float PLAYER_SPEED = 300.f;
const float OBSTACLE_GRAVITY_STRENGTH = 120000.f; // かなり強く調整済み
const float PLAYER_GRAVITY_STRENGTH = 42000.f;
const int HISTORY_MAX = 600;
const float SPAWN_INTERVAL_BASE = 1.2f;
const float SPAWN_ACCEL = 0.0005f;
// 壁関連定数
const float WALL_THICKNESS = 40.f;
const float WALL_GAP_HEIGHT = 130.f;
const float WALL_SPAWN_INTERVAL = 10.f;
const float WALL_WARN_TIME = 2.f;
const float WALL_SPEED_MULT = 1.1f;
