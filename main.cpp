#define _USE_MATH_DEFINES
#include <Novice.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <string>
#include <iomanip>
#include "GameData.h"
#include "Sound.h"

// 簡易 min / max 関数
float myMin(float a, float b) { return (a < b) ? a : b; }
float myMax(float a, float b) { return (a > b) ? a : b; }
size_t myMin(size_t a, size_t b) { return (a < b) ? a : b; }

// シーン管理用enum
enum GameScene {
    TITLE,     // タイトル画面
    EXPLAIN,   // 操作説明画面
    GAME,      // ゲーム本編
    RESULT     // リザルト画面
};

// パーティクル構造体
struct Particle {
    Vector2 pos;
    Vector2 vel;
    float lifeTime;
    bool active;
    Particle() : pos(), vel(), lifeTime(0.f), active(false) {}
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    srand(static_cast<unsigned int>(time(nullptr)));
    Novice::Initialize(kWindowTitle, WINDOW_W, WINDOW_H);

    // 音声読み込み（title.mp3 のみ）
    Sound::LoadAllSounds();
    // ゲーム開始時にすぐ titleBGM をループ再生開始
    Sound::PlayTitleBGM();

    // テクスチャ読み込み
    int wallTex = Novice::LoadTexture("./images/wall.png");
    int distortionCoreTex = Novice::LoadTexture("./images/distortion_core.png");
    int pinFlagTex = Novice::LoadTexture("./images/pin_flag.png");
    int distortionExplodeTex = Novice::LoadTexture("./images/distortion_exprode.png");

    int numberTex[10] = { 0 };
    for (int i = 0; i < 10; ++i) {
        char path[32];
        sprintf_s(path, "./images/%d.png", i);
        numberTex[i] = Novice::LoadTexture(path);
    }

    int levelLabelTex = Novice::LoadTexture("./images/level.png");
    int timeLabelTex = Novice::LoadTexture("./images/time.png");
    int secTex = Novice::LoadTexture("./images/sec.png");
    int titleTex = Novice::LoadTexture("./images/title.png");
    int explainTex = Novice::LoadTexture("./images/explain.png");
    int resultTex = Novice::LoadTexture("./images/result.png");
    int obstacleSmallTex = Novice::LoadTexture("./images/obstacle_small.png");
    int obstacleMediumTex = Novice::LoadTexture("./images/obstacle_medium.png");
    int obstacleLargeTex = Novice::LoadTexture("./images/obstacle_large.png");

    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    GameScene scene = TITLE;                    // 現在のゲームシーンを管理
    std::vector<float> highScores;              // ハイスコア保存用（最大20件）
    highScores.reserve(20);

    Player player = { {200.f, static_cast<float>(WINDOW_H) / 2.f}, {0.f, 0.f} };
    std::vector<Vector2> playerHistory;         // プレイヤーの移動履歴（リワインド用）
    playerHistory.reserve(HISTORY_MAX);

    std::vector<Obstacle> obstacles;            // 通常の障害物
    obstacles.reserve(120);
    std::vector<Distortion> distortions;        // 歪曲フィールド
    distortions.reserve(60);
    std::vector<DebrisZone> debrisZones;        // 爆発後のデブリゾーン
    debrisZones.reserve(30);
    std::vector<Star> stars(200);               // 背景の星
    std::vector<Particle> particles;            // パーティクルエフェクト
    particles.reserve(600);

    float spawnTimer = 0.f;                     // 障害物出現タイマー
    bool gameOver = false;                      // ゲームオーバーフラグ
    float gameTime = 0.f;                       // プレイ時間（スコア）
    std::vector<Fragment> fragments;            // 障害物破壊時の破片
    fragments.reserve(200);
    std::vector<Wall> walls;                    // 隙間ありの壁
    walls.reserve(5);
    float wallSpawnTimer = 0.f;                 // 壁出現タイマー
    int wallSpawnCount = 0;                     // 壁出現回数（難易度管理用）
    float bestTime = 0.f;                       // 自己ベストタイム

    Vector2 rewindMarkPos = { -9999.f, -9999.f }; // リワインドマーク位置
    float rewindMarkTime = 0.f;                 // リワインドマークを置いた時間
    bool markExpired = false;                   // マークが期限切れになったか

    // 背景星の初期配置
    for (auto& s : stars) {
        s.pos.x = static_cast<float>(rand() % WINDOW_W);
        s.pos.y = static_cast<float>(rand() % WINDOW_H);
    }

    while (Novice::ProcessMessage() == 0) {
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);
        Novice::BeginFrame();

        // 画面全体を黒でクリア
        Novice::DrawBox(0, 0, WINDOW_W, WINDOW_H, 0.f, 0x000000FF, kFillModeSolid);

        switch (scene) {
        case TITLE: // タイトル画面
            // タイトル画像の描画
            Novice::DrawSpriteRect(0, 0, 0, 0, WINDOW_W, WINDOW_H, titleTex, 1.0f, 1.0f, 0.0f, WHITE);
            // SPACEキーで操作説明画面へ遷移
            if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0) scene = EXPLAIN;
            break;

        case EXPLAIN: // 操作説明画面
            // 操作説明画像の描画
            Novice::DrawSpriteRect(0, 0, 0, 0, WINDOW_W, WINDOW_H, explainTex, 1.0f, 1.0f, 0.0f, WHITE);
            // SPACEキーでゲーム開始（全データをリセット）
            if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0) {
                player.pos = { 200.f, static_cast<float>(WINDOW_H) / 2.f };
                player.vel = { 0.f, 0.f };
                playerHistory.clear();
                obstacles.clear();
                distortions.clear();
                debrisZones.clear();
                fragments.clear();
                walls.clear();
                particles.clear();
                spawnTimer = 0.f;
                wallSpawnTimer = 0.f;
                wallSpawnCount = 0;
                gameTime = 0.f;
                gameOver = false;
                rewindMarkPos = { -9999.f, -9999.f };
                markExpired = false;
                scene = GAME;
            }
            break;

        case GAME: // ゲーム本編
            if (gameOver) {
                // ゲームオーバー時の処理（ハイスコア更新と自己ベスト更新）
                highScores.push_back(gameTime);
                std::sort(highScores.rbegin(), highScores.rend());
                if (highScores.size() > 20) highScores.resize(20);
                if (gameTime > bestTime) bestTime = gameTime;
                scene = RESULT;
            }
            else {
                // ----- ゲームメインループ -----

                gameTime += DT; // プレイ時間の進行

                // プレイヤーの移動履歴を保存（リワインド機能用）
                playerHistory.push_back(player.pos);
                if (playerHistory.size() > static_cast<size_t>(HISTORY_MAX)) {
                    playerHistory.erase(playerHistory.begin());
                }

                // WASDキー入力によるプレイヤー速度設定
                player.vel = { 0.f, 0.f };
                if (keys[DIK_W]) player.vel.y -= PLAYER_SPEED;
                if (keys[DIK_S]) player.vel.y += PLAYER_SPEED;
                if (keys[DIK_A]) player.vel.x -= PLAYER_SPEED;
                if (keys[DIK_D]) player.vel.x += PLAYER_SPEED;

                // プレイヤー位置の更新と画面端制限
                player.pos = player.pos + player.vel * DT;
                player.pos.x = myMax(16.f, myMin(static_cast<float>(WINDOW_W - 16), player.pos.x));
                player.pos.y = myMax(16.f, myMin(static_cast<float>(WINDOW_H - 16), player.pos.y));

                // 移動時のパーティクル発生
                if (player.vel.mag() > 10.f) {
                    for (int i = 0; i < 4; ++i) {
                        Particle p;
                        p.pos = player.pos;
                        p.vel = player.vel * -0.4f + Vector2(
                            static_cast<float>(rand() % 80 - 40),
                            static_cast<float>(rand() % 80 - 40)
                        );
                        p.lifeTime = 0.6f + static_cast<float>(rand() % 30) / 100.f;
                        p.active = true;
                        particles.push_back(p);
                    }
                }

                // パーティクルの更新と寿命切れの削除
                for (auto& p : particles) {
                    if (!p.active) continue;
                    p.pos = p.pos + p.vel * DT;
                    p.vel = p.vel * 0.94f;
                    p.lifeTime -= DT;
                    if (p.lifeTime <= 0.f) p.active = false;
                }
                particles.erase(std::remove_if(particles.begin(), particles.end(),
                    [](const Particle& p) { return !p.active; }), particles.end());

                // リワインドマークの期限切れ処理
                if (rewindMarkPos.x > -9000.f) {
                    float remaining = 10.f - (gameTime - rewindMarkTime);
                    if (remaining <= 0.f) {
                        // マーク消滅時のパーティクルエフェクト
                        for (int i = 0; i < 30; ++i) {
                            Particle p;
                            p.pos = rewindMarkPos;
                            float angle = static_cast<float>(rand()) / RAND_MAX * 2.f * static_cast<float>(M_PI);
                            float speed = 80.f + static_cast<float>(rand() % 100);
                            p.vel = { std::cosf(angle) * speed, std::sinf(angle) * speed };
                            p.lifeTime = 0.5f + static_cast<float>(rand() % 30) / 100.f;
                            p.active = true;
                            particles.push_back(p);
                        }
                        rewindMarkPos = { -9999.f, -9999.f };
                        markExpired = true;
                    }
                }

                // スペースキー入力処理（リワインドマークの設置／発動）
                if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0) {
                    if (rewindMarkPos.x < -9000.f) {
                        // マークを現在位置に設置
                        rewindMarkPos = player.pos;
                        rewindMarkTime = gameTime;
                    }
                    else if (gameTime - rewindMarkTime <= 10.f) {
                        // リワインド実行
                        Vector2 oldPos = player.pos;
                        player.pos = rewindMarkPos;
                        player.vel = { 0.f, 0.f };
                        // 残像として歪曲フィールドを生成
                        Distortion newDist;
                        newDist.pos = oldPos;
                        newDist.radius = 25.f;
                        newDist.merges = 1;
                        newDist.active = true;
                        newDist.absorbPoints = 0;
                        distortions.push_back(newDist);
                        rewindMarkPos = { -9999.f, -9999.f };
                    }
                }

                // ----- 各種オブジェクトの更新処理 -----

                // 歪曲フィールドによるプレイヤーへの重力影響
                for (auto& dist : distortions) {
                    if (!dist.active) continue;
                    Vector2 dir = dist.pos - player.pos;
                    float d = dir.mag();
                    if (d > 0.f && d < dist.radius * 8.f) {
                        float scale = (dist.merges >= 3) ? 3.f : 1.f;
                        float force = PLAYER_GRAVITY_STRENGTH * static_cast<float>(dist.merges) * scale / (d * d + 10.f);
                        player.vel = player.vel + dir.normalized() * force * DT;
                    }
                }

                // 歪曲フィールド同士の合体処理
                for (size_t i = 0; i < distortions.size(); ++i) {
                    if (!distortions[i].active) continue;
                    for (size_t j = i + 1; j < distortions.size(); ) {
                        if (!distortions[j].active) { ++j; continue; }
                        float d = (distortions[i].pos - distortions[j].pos).mag();
                        float mergeRange = distortions[i].radius * 2.0f + distortions[j].radius * 2.0f;
                        if (d < mergeRange && distortions[i].merges < 3 && distortions[j].merges < 3) {
                            distortions[i].merges += distortions[j].merges;
                            distortions[i].radius = myMin(150.f, distortions[i].radius + distortions[j].radius * 0.6f);
                            distortions[j].active = false;
                        }
                        else ++j;
                    }
                }
                distortions.erase(std::remove_if(distortions.begin(), distortions.end(),
                    [](const Distortion& d) { return !d.active; }), distortions.end());

                // プレイヤーと歪曲フィールドの衝突押し出し処理
                const float playerRadius = 16.0f;
                for (const auto& dist : distortions) {
                    if (!dist.active) continue;
                    Vector2 toPlayer = player.pos - dist.pos;
                    float distToCenter = toPlayer.mag();
                    float safeDist = dist.radius + playerRadius;
                    if (distToCenter < safeDist && distToCenter > 0.001f) {
                        Vector2 pushDir = toPlayer.normalized();
                        float overlap = safeDist - distToCenter;
                        float pushStrength = overlap * 300.f + static_cast<float>(dist.merges) * 150.f;
                        player.pos = player.pos + pushDir * pushStrength * DT;
                        player.vel = player.vel + pushDir * pushStrength * 0.5f * DT;
                    }
                }

                // デブリゾーンとプレイヤーの衝突押し出し処理
                for (auto& zone : debrisZones) {
                    if (!zone.active) continue;
                    Vector2 toPlayer = player.pos - zone.pos;
                    float d = toPlayer.mag();
                    float safe = zone.radius + playerRadius;
                    if (d < safe && d > 0.001f) {
                        Vector2 push = toPlayer.normalized();
                        float overlap = safe - d;
                        player.pos = player.pos + push * overlap * 400.f * DT;
                    }
                }

                // 障害物の出現処理
                float timeBasedInterval = myMax(0.3f, SPAWN_INTERVAL_BASE - gameTime * SPAWN_ACCEL);
                float wallInfluence = 1.f;
                if (wallSpawnCount >= 5) {
                    int steps = wallSpawnCount - 4;
                    wallInfluence = myMax(0.4f, 1.f - static_cast<float>(steps) * 0.08f);
                }
                float spawnInterval = timeBasedInterval * wallInfluence;

                spawnTimer += DT;
                if (spawnTimer >= spawnInterval) {
                    // 小・中・大の障害物を生成
                    Obstacle obs;
                    obs.pos = { static_cast<float>(WINDOW_W + 30), static_cast<float>(50 + rand() % (WINDOW_H - 100)) };
                    int type = rand() % 3;
                    if (type == 0) obs.radius = 12.f;
                    else if (type == 1) obs.radius = 24.f;
                    else obs.radius = 36.f;
                    obs.active = true;
                    obstacles.push_back(obs);
                    spawnTimer = 0.f;
                }

                // 壁の出現処理
                wallSpawnTimer += DT;
                if (wallSpawnTimer >= WALL_SPAWN_INTERVAL) {
                    wallSpawnCount++;
                    float baseGapHeight = 130.f + static_cast<float>(rand() % 40);
                    float shrinkFactor = 1.f;
                    if (wallSpawnCount >= 5) {
                        int steps = wallSpawnCount - 4;
                        shrinkFactor = myMax(0.f, 1.f - static_cast<float>(steps) * 0.12f);
                    }
                    float currentGapHeight = baseGapHeight * shrinkFactor;

                    // 右から左へ移動する壁
                    {
                        Wall newWall;
                        newWall.pos = { static_cast<float>(WINDOW_W + 100), 0.f };
                        newWall.gapY = 120.f + static_cast<float>(rand() % static_cast<int>(WINDOW_H - 240));
                        newWall.gapHeight = currentGapHeight;
                        newWall.width = 40.f;
                        newWall.active = true;
                        newWall.speed = -SCROLL_SPEED * WALL_SPEED_MULT;
                        walls.push_back(newWall);
                    }
                    // 左から右へ移動する壁（一定回数以降）
                    if (wallSpawnCount >= 3) {
                        Wall newWall2;
                        newWall2.pos = { -100.f, 0.f };
                        newWall2.gapY = 120.f + static_cast<float>(rand() % static_cast<int>(WINDOW_H - 240));
                        newWall2.gapHeight = currentGapHeight;
                        newWall2.width = 40.f;
                        newWall2.active = true;
                        newWall2.speed = +SCROLL_SPEED * WALL_SPEED_MULT;
                        walls.push_back(newWall2);
                    }
                    wallSpawnTimer = 0.f;
                }

                // 障害物の更新・歪曲フィールドによる吸い寄せと吸収処理
                for (auto& obs : obstacles) {
                    if (!obs.active) continue;
                    obs.pos.x -= SCROLL_SPEED;
                    bool absorbed = false;

                    for (auto& dist : distortions) {
                        if (!dist.active) continue;
                        Vector2 dir = dist.pos - obs.pos;
                        float d = dir.mag();
                        bool applyGravity = false;
                        bool canBeDestroyed = false;

                        if (dist.merges == 1) {
                            if (obs.radius <= 15.f) { applyGravity = true; canBeDestroyed = true; }
                        }
                        else if (dist.merges == 2) {
                            if (obs.radius <= 30.f) { applyGravity = true; canBeDestroyed = true; }
                        }
                        else {
                            applyGravity = true;
                            canBeDestroyed = true;
                        }

                        if (applyGravity && d > 0.f && d < 250.f) {
                            float scale = (dist.merges >= 3) ? 3.f : 1.f;
                            float force = OBSTACLE_GRAVITY_STRENGTH * 0.6f * static_cast<float>(dist.merges) * scale / (d * d + 10.f);
                            obs.pos = obs.pos + dir.normalized() * force * DT;
                        }

                        if (canBeDestroyed && d <= dist.radius + obs.radius) {
                            // 破壊時の破片生成
                            int fragmentCount = (obs.radius <= 15.f) ? 12 : (obs.radius <= 30.f) ? 20 : 30;
                            float baseSpeed = 650.f;
                            float speedRange = 200.f;
                            for (int i = 0; i < fragmentCount; ++i) {
                                float random01 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                                float angle = random01 * 2.f * static_cast<float>(M_PI);
                                float speed = baseSpeed + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * speedRange;
                                Vector2 vel = { std::cosf(angle) * speed, std::sinf(angle) * speed };
                                Fragment frag;
                                frag.pos = obs.pos;
                                frag.vel = vel;
                                frag.lifeTime = 0.8f + static_cast<float>(rand() % 40) / 100.f;
                                frag.radius = 1.2f + static_cast<float>(rand() % 16) / 10.f;
                                frag.active = true;
                                fragments.push_back(frag);
                            }

                            // 吸収ポイント加算と爆発判定
                            int pt = 0;
                            if (obs.radius <= 15.f) pt = 1;
                            else if (obs.radius <= 30.f) pt = 5;
                            else pt = 10;
                            dist.absorbPoints += pt;

                            int explodeThreshold = 0;
                            if (dist.merges == 1) explodeThreshold = 10;
                            else if (dist.merges == 2) explodeThreshold = 30;
                            else if (dist.merges == 3) explodeThreshold = 50;

                            if (explodeThreshold > 0 && dist.absorbPoints >= explodeThreshold) {
                                DebrisZone zone;
                                zone.pos = dist.pos;
                                zone.radius = dist.radius * 1.2f;
                                zone.active = true;
                                debrisZones.push_back(zone);
                                dist.active = false;
                            }
                            obs.active = false;
                            absorbed = true;
                            break;
                        }
                    }

                    if (!absorbed && (obs.pos.x < -50.f || obs.pos.y < -50.f || obs.pos.y > static_cast<float>(WINDOW_H + 50))) {
                        obs.active = false;
                    }
                }
                obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
                    [](const Obstacle& o) { return !o.active; }), obstacles.end());

                // 壁の更新と衝突判定
                for (auto& wall : walls) {
                    if (!wall.active) continue;
                    wall.pos.x += wall.speed;

                    if (player.pos.x + 16 > wall.pos.x && player.pos.x - 16 < wall.pos.x + wall.width) {
                        bool inGap = (player.pos.y > wall.gapY - wall.gapHeight / 2) &&
                            (player.pos.y < wall.gapY + wall.gapHeight / 2);
                        if (!inGap) gameOver = true;
                    }

                    if ((wall.speed < 0.f && wall.pos.x + wall.width < -50.f) ||
                        (wall.speed > 0.f && wall.pos.x > static_cast<float>(WINDOW_W) + 50.f)) {
                        wall.active = false;
                    }
                }
                walls.erase(std::remove_if(walls.begin(), walls.end(),
                    [](const Wall& w) { return !w.active; }), walls.end());

                // 破片の更新
                for (auto& frag : fragments) {
                    if (!frag.active) continue;
                    frag.pos = frag.pos + frag.vel * DT;
                    frag.lifeTime -= DT;
                    if (frag.lifeTime <= 0.f) frag.active = false;
                    frag.vel = frag.vel * 0.96f;
                }
                fragments.erase(std::remove_if(fragments.begin(), fragments.end(),
                    [](const Fragment& f) { return !f.active; }), fragments.end());

                // プレイヤーと障害物の衝突判定
                for (const auto& obs : obstacles) {
                    if (!obs.active) continue;
                    Vector2 diff = player.pos - obs.pos;
                    float distSq = diff.x * diff.x + diff.y * diff.y;
                    float minDist = playerRadius + obs.radius;
                    if (distSq < minDist * minDist) {
                        gameOver = true;
                        break;
                    }
                }

                if (gameTime > bestTime) bestTime = gameTime;

                // ----- 描画処理 -----

                // 壁の描画
                for (const auto& wall : walls) {
                    if (!wall.active) continue;
                    Novice::DrawSpriteRect(static_cast<int>(wall.pos.x), 0,
                        0, 0, 40, 720, wallTex, 1.0f, 1.0f, 0.0f, WHITE);
                    int gapTop = static_cast<int>(wall.gapY - wall.gapHeight / 2);
                    int gapHeight = static_cast<int>(wall.gapHeight);
                    Novice::DrawBox(static_cast<int>(wall.pos.x),
                        gapTop,
                        static_cast<int>(wall.width),
                        gapHeight,
                        0.f, 0x000000FF, kFillModeSolid);
                }

                // タイム表示
                {
                    int score = static_cast<int>(gameTime);
                    std::string scoreStr = std::to_string(score);
                    std::string paddedScore = std::string(3 - scoreStr.length(), '0') + scoreStr;
                    int digitWidth = 48;
                    float digitScale = 1.0f;
                    int scaledDigitWidth = static_cast<int>(digitWidth * digitScale);
                    int labelWidth = 144;
                    int startX = WINDOW_W - 20 - labelWidth - (3 * scaledDigitWidth) - 10;
                    int y = 20;
                    Novice::DrawSpriteRect(startX, y, 0, 0, 144, 64, timeLabelTex, 1.0f, 1.0f, 0.0f, WHITE);
                    for (size_t i = 0; i < 3; ++i) {
                        int digit = paddedScore[i] - '0';
                        Novice::DrawSpriteRect(
                            startX + labelWidth + static_cast<int>(i * scaledDigitWidth),
                            y,
                            0, 0, 48, 64,
                            numberTex[digit],
                            digitScale, digitScale,
                            0.0f,
                            WHITE
                        );
                    }
                }

                // レベル（壁出現回数）表示
                {
                    std::string levelStr = std::to_string(wallSpawnCount);
                    std::string paddedLevel = std::string(3 - levelStr.length(), '0') + levelStr;
                    int digitWidth = 48;
                    float digitScale = 1.0f;
                    int scaledDigitWidth = static_cast<int>(digitWidth * digitScale);
                    int labelWidth = 144;
                    int startX = WINDOW_W - 20 - labelWidth - (3 * scaledDigitWidth) - 10;
                    int y = 100;
                    Novice::DrawSpriteRect(startX, y, 0, 0, 144, 64, levelLabelTex, 1.0f, 1.0f, 0.0f, WHITE);
                    for (size_t i = 0; i < 3; ++i) {
                        int digit = paddedLevel[i] - '0';
                        Novice::DrawSpriteRect(
                            startX + labelWidth + static_cast<int>(i * scaledDigitWidth),
                            y,
                            0, 0, 48, 64,
                            numberTex[digit],
                            digitScale, digitScale,
                            0.0f,
                            WHITE
                        );
                    }
                }

                // パーティクル描画
                for (const auto& p : particles) {
                    if (!p.active) continue;
                    float alpha = p.lifeTime * 1.2f;
                    int a = static_cast<int>(alpha * 180.f + 0.5f);
                    unsigned int col = (static_cast<unsigned int>(a) << 24) | 0xFFFFFF;
                    Novice::DrawBox(static_cast<int>(p.pos.x - 1), static_cast<int>(p.pos.y - 1), 2, 2, 0.f, col, kFillModeSolid);
                }

                // プレイヤーの残像（軌跡）描画
                size_t trailCount = myMin(static_cast<size_t>(40), playerHistory.size());
                for (size_t i = 1; i < trailCount; ++i) {
                    Vector2 p1 = playerHistory[playerHistory.size() - i];
                    Vector2 p2 = playerHistory[playerHistory.size() - i - 1];
                    float alpha = (1.f - static_cast<float>(i) / trailCount) * 0.6f;
                    int a = static_cast<int>(alpha * 255);
                    unsigned int col = (static_cast<unsigned int>(a) << 24) | 0xAAAAAA;
                    Novice::DrawLine(static_cast<int>(p1.x), static_cast<int>(p1.y),
                        static_cast<int>(p2.x), static_cast<int>(p2.y), col);
                }

                // リワインドマーク描画
                if (rewindMarkPos.x > -9000.f) {
                    float remaining = 10.f - (gameTime - rewindMarkTime);
                    if (remaining < 0.f) remaining = 0.f;
                    float scale = remaining / 10.f;
                    int w = static_cast<int>(32 * scale);
                    int h = static_cast<int>(32 * scale);
                    Novice::DrawSpriteRect(static_cast<int>(rewindMarkPos.x - w / 2),
                        static_cast<int>(rewindMarkPos.y - h),
                        0, 0, 32, 32, pinFlagTex, scale, scale, 0.0f, WHITE);
                }

                // 歪曲フィールド描画
                for (const auto& d : distortions) {
                    if (!d.active) continue;
                    float coreScale = d.radius / 32.0f;
                    int coreW = 64;
                    int coreH = 64;
                    Novice::DrawSpriteRect(
                        static_cast<int>(d.pos.x - (coreW * coreScale) / 2),
                        static_cast<int>(d.pos.y - (coreH * coreScale) / 2),
                        0, 0, coreW, coreH,
                        distortionCoreTex,
                        coreScale, coreScale,
                        0.0f,
                        WHITE
                    );
                }

                // デブリゾーン描画
                for (const auto& zone : debrisZones) {
                    if (!zone.active) continue;
                    float zoneScale = zone.radius / 32.0f;
                    int zoneW = 64;
                    int zoneH = 64;
                    Novice::DrawSpriteRect(
                        static_cast<int>(zone.pos.x - (zoneW * zoneScale) / 2),
                        static_cast<int>(zone.pos.y - (zoneH * zoneScale) / 2),
                        0, 0, zoneW, zoneH,
                        distortionExplodeTex,
                        zoneScale, zoneScale,
                        0.0f,
                        WHITE
                    );
                }

                // 障害物描画
                for (const auto& obs : obstacles) {
                    if (!obs.active) continue;
                    int tex;
                    int texSize;
                    if (obs.radius <= 15.f) {
                        tex = obstacleSmallTex;
                        texSize = 24;
                    }
                    else if (obs.radius <= 30.f) {
                        tex = obstacleMediumTex;
                        texSize = 48;
                    }
                    else {
                        tex = obstacleLargeTex;
                        texSize = 72;
                    }
                    float scale = obs.radius / static_cast<float>(texSize / 2.0f);
                    Novice::DrawSpriteRect(
                        static_cast<int>(obs.pos.x - obs.radius),
                        static_cast<int>(obs.pos.y - obs.radius),
                        0, 0, texSize, texSize,
                        tex,
                        scale, scale,
                        0.0f,
                        WHITE
                    );
                }

                // 破片描画
                for (const auto& frag : fragments) {
                    if (!frag.active) continue;
                    float alpha = frag.lifeTime * 0.8f;
                    int a = static_cast<int>(alpha * 255.f + 0.5f);
                    unsigned int color = (static_cast<unsigned int>(a) << 24) | 0xFFFFFF;
                    Novice::DrawBox(static_cast<int>(frag.pos.x - frag.radius), static_cast<int>(frag.pos.y - frag.radius),
                        static_cast<int>(frag.radius * 2), static_cast<int>(frag.radius * 2), 0.f, color, kFillModeSolid);
                }

                // プレイヤー本体描画
                Novice::DrawBox(static_cast<int>(player.pos.x - 16), static_cast<int>(player.pos.y - 16),
                    32, 32, 0.f, 0xFFFFFFFF, kFillModeSolid);
            }
            break;

        case RESULT: // リザルト画面
            // リザルト画像の描画
            Novice::DrawSpriteRect(0, 0, 0, 0, WINDOW_W, WINDOW_H, resultTex, 1.0f, 1.0f, 0.0f, WHITE);

            // 今回のプレイタイム表示
            int yourTime = static_cast<int>(gameTime);
            std::string yourStr = std::to_string(yourTime);
            int digitWidth = 48;
            int startX = 550;
            int y = 170;
            for (size_t i = 0; i < yourStr.length(); ++i) {
                int digit = yourStr[i] - '0';
                Novice::DrawSpriteRect(
                    startX + static_cast<int>(i * digitWidth),
                    y,
                    0, 0, 48, 64,
                    numberTex[digit],
                    1.0f, 1.0f,
                    0.0f,
                    WHITE
                );
            }
            Novice::DrawSpriteRect(
                startX + static_cast<int>(yourStr.length() * digitWidth) + 10,
                y,
                0, 0, 48, 64, secTex, 1.0f, 1.0f, 0.0f, WHITE
            );

            // ハイスコア上位5件の表示
            if (!highScores.empty()) {
                for (size_t i = 0; i < myMin(static_cast<size_t>(5), highScores.size()); ++i) {
                    int rankY = 360 + static_cast<int>(i) * 70;
                    std::string rankStr = std::to_string(static_cast<int>(highScores[i]));
                    int rankX = 380;
                    for (size_t j = 0; j < rankStr.length(); ++j) {
                        int digit = rankStr[j] - '0';
                        Novice::DrawSpriteRect(
                            rankX + 60 + static_cast<int>(j * digitWidth),
                            rankY,
                            0, 0, 48, 64,
                            numberTex[digit],
                            1.0f, 1.0f,
                            0.0f,
                            WHITE
                        );
                    }
                    Novice::DrawSpriteRect(
                        rankX + 60 + static_cast<int>(rankStr.length() * digitWidth) + 10,
                        rankY,
                        0, 0, 48, 64, secTex, 1.0f, 1.0f, 0.0f, WHITE
                    );
                }
            }

            // Rキーでタイトル画面に戻る
            if (preKeys[DIK_R] == 0 && keys[DIK_R] != 0) {
                playerHistory.clear();
                obstacles.clear();
                distortions.clear();
                debrisZones.clear();
                fragments.clear();
                walls.clear();
                particles.clear();
                spawnTimer = 0.f;
                wallSpawnTimer = 0.f;
                wallSpawnCount = 0;
                gameTime = 0.f;
                gameOver = false;
                rewindMarkPos = { -9999.f, -9999.f };
                scene = TITLE;
            }
            break;
        }

        // 背景星の描画とスクロール
        for (auto& s : stars) {
            int px = static_cast<int>(s.pos.x);
            int py = static_cast<int>(s.pos.y);
            Novice::DrawBox(px, py, 1, 1, 0.f, 0xCCCCCCFF, kFillModeSolid);
            if (scene == GAME) {
                s.pos.x -= SCROLL_SPEED * 0.4f;
                if (s.pos.x < 0.f) s.pos.x += static_cast<float>(WINDOW_W);
            }
        }

        Novice::EndFrame();
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) break;
    }

    // 終了時にBGM停止
    Sound::StopBGM();
    Novice::Finalize();
    return 0;
}