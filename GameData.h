#pragma once
#define _USE_MATH_DEFINES
#include <Novice.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>

// 定数宣言（extern）
extern const char* kWindowTitle;
extern const int WINDOW_W;
extern const int WINDOW_H;
extern const float DT;
extern const float SCROLL_SPEED;
extern const float PLAYER_SPEED;
extern const float OBSTACLE_GRAVITY_STRENGTH;
extern const float PLAYER_GRAVITY_STRENGTH;
extern const int HISTORY_MAX;
extern const float SPAWN_INTERVAL_BASE;
extern const float SPAWN_ACCEL;
// ── 壁関連定数 ──
extern const float WALL_SPEED_MULT;
extern const float WALL_SPAWN_INTERVAL;
// ── 巻き戻しトレイル用 ──
extern const float REWIND_TRAIL_LIFETIME;
extern const float REWIND_TRAIL_FADE_SPEED;

// 構造体
struct Vector2 {
    float x, y;
    Vector2(float x_ = 0.f, float y_ = 0.f) : x(x_), y(y_) {}
    Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }
    Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }
    Vector2 operator*(float s) const { return { x * s, y * s }; }
    float mag() const { return std::sqrtf(x * x + y * y); }
    Vector2 normalized() const {
        float m = mag();
        if (m > 0.f) return *this * (1.f / m);
        return *this;
    }
};

struct Star {
    Vector2 pos;
};

struct Player {
    Vector2 pos;
    Vector2 vel;
};

struct Obstacle {
    Vector2 pos;
    float radius;
    bool active;
    Obstacle() : pos(), radius(0.f), active(false) {}
};

struct Distortion {
    Vector2 pos;
    float radius;
    int merges;
    bool active;
    int absorbPoints; // 吸収ポイント
    Distortion() : pos(), radius(0.f), merges(0), active(false), absorbPoints(0) {}
};

struct RewindTrail {
    Vector2 startPos;
    Vector2 endPos;
    float lifeTime;
    bool active;
};

struct Fragment {
    Vector2 pos;
    Vector2 vel;
    float lifeTime;
    float radius;
    bool active;
    Fragment() : pos(), vel(), lifeTime(0.f), radius(2.f), active(false) {}
};

struct Wall {
    Vector2 pos;          // 壁の左上の位置
    float width;          // 壁の厚さ（横幅）
    float gapY;           // 穴の中心Y位置
    float gapHeight;      // 穴の高さ
    bool active;
    float speed;          // 移動速度（負:右→左、正:左→右）
    Wall() : pos(), width(40.f), gapY(360.f), gapHeight(140.f), active(false), speed(0.f) {}
};

// 爆発後の残骸エリア（プレイヤーは入れないが障害物は通過可）
struct DebrisZone {
    Vector2 pos;
    float radius;
    float lifeTime; // 残存時間（秒）
    bool active;
    DebrisZone() : pos(), radius(0.f), lifeTime(0.f), active(false) {}
};

