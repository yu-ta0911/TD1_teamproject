#include <Novice.h>
#include "FontRenderer.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>


const char kWindowTitle[] = "LC1C_20_ナカムラユウタ_タイトル";

// ============================================================
// 定数
// ============================================================
const int kScreenW = 1280;
const int kScreenH = 720;

// 物理
const float kGravity = 0.5f;       // 重力
const float kBuoyancy = -0.9f;     // 浮力(スペース押下中)
const float kMaxFallSpeed = 12.0f; // 最大落下速度
const float kMaxRiseSpeed = -8.0f; // 最大上昇速度

// プレイヤー
const int kPlayerMaxHp = 5;
const int kInvincibleFrames = 90; // 被弾後の無敵時間
const int kChargeNeed = 30;       // チャージ弾に必要なフレーム数

// 弾
const int kMaxBullets = 64;
const float kScrollSpeed = 4.0f; // 障害物・コイン・ゴールが左へ流れる速度

// ゴール
const int kGoalWidth = 60;       // ゴールの幅
const int kGoalTileH = 30;       // ゴール画像1枚ぶんの高さ(縦にこの高さで並べる)
const int kClearHpBonus = 100;   // クリア時、残りHP1つにつくボーナス

// ボス
const float kBossW = 120.0f;
const float kBossH = 120.0f;
const float kBossStopX = 900.0f; // ボスが止まって戦うX座標

// 背景
const float kBgScrollSpeed = 1.0f; // 背景のスクロール速度(前景より遅いと奥行きが出る)

// 配列サイズ
const int kMaxObstacles = 32;
const int kMaxEnemies = 12;
const int kMaxCoins = 32;
const int kMaxBigCoins = 8;
const int kMaxHeals = 8;
const int kMaxTilesPerObstacle = 24; // 1つの障害物を構成するタイルの最大数(高さ ÷ 幅)
const int kTileScore = 5;            // ブロックのタイルを1つ壊したときの得点

// 画像フォルダ(プロジェクトの実行フォルダからの位置)
const char kImageDir[] = "./images/";

// 色(画像が読み込めなかったときの代わりの色、画面表示用)
const unsigned int kColorYellow = 0xFFFF00FF;
const unsigned int kColorOrange = 0xFFA500FF;
const unsigned int kColorCyan = 0x00FFFFFF;
const unsigned int kColorBrown = 0xCC8844FF;
const unsigned int kColorPurple = 0x9944DDFF;
const unsigned int kColorGray = 0x888888FF;
const unsigned int kColorPink = 0xFF66CCFF;
const unsigned int kColorBlue = 0x3366CCFF;
const unsigned int kColorDarkGray = 0x333333FF;
const unsigned int kColorPanel = 0x000000BBu; // 結果画面の半透明の黒
const unsigned int kColorGold = 0xFFD700FF;
const unsigned int kColorHeal = 0x44DD66FF;
const unsigned int kColorDarkRed = 0xAA2222FF;
const unsigned int kColorHpBar = 0x44DD44FF;

// デバッグ表示(当たり判定)の色。下2桁(66)が透明度
const unsigned int kDebugPlayer = 0x00FF0066;
const unsigned int kDebugEnemy = 0xFF000066;
const unsigned int kDebugBoss = 0xFF000066;
const unsigned int kDebugBlockNormal = 0xFFAA0066; // 通常弾で壊せるブロック
const unsigned int kDebugBlockCharge = 0xAA00FF66; // チャージ弾でのみ壊せるブロック
const unsigned int kDebugBlockSolid = 0xFFFFFF66;  // 壊せないブロック
const unsigned int kDebugItem = 0xFFFF0066;        // コイン・大コイン・回復
const unsigned int kDebugPlayerBullet = 0x00FFFF66;
const unsigned int kDebugEnemyBullet = 0xFF880066;
const unsigned int kDebugGoal = 0x00FF0044;

// 画面(シーン)
enum Scene {
	kSceneTitle,       // タイトル画面
	kSceneStageSelect, // ステージ選択
	kScenePlay,        // プレイ中(ゲームオーバー・クリア表示も含む)
};

// ============================================================
// 構造体
// ============================================================
struct Player {
	float x, y, size, vy;
	int hp;
	int invincible; // 無敵残りフレーム
	int charge;     // チャージ中のフレーム数
	int score;
	int coins;
	int bigCoins; // 取った大コインの数
};

struct Bullet {
	float x, y;
	float vx, vy;
	float radius;
	bool isCharged;
	bool isActive;
};

struct Obstacle {
	float x, y, w, h;
	int type; // 0: 通常弾で壊せる / 1: チャージ弾でのみ壊せる / 2: 壊せない
	bool isActive;
	int tileCount;                         // 縦に並ぶタイルの数
	int aliveTiles;                        // 壊れていないタイルの数(0になったら障害物ごと消える)
	bool tileAlive[kMaxTilesPerObstacle];  // タイルごとの「まだ残っているか」
};

struct Enemy {
	float x, baseY, y;
	float size;
	int hp;
	int maxHp;
	int shotTimer;
	int frame;
	int type; // 0: 揺れて撃つ / 1: 揺れず撃たない
	bool isActive;
};

struct Coin {
	float x, y, radius;
	bool isActive;
};

// ゴール(画面の上から下までの縦長のゲート。触れるとステージクリア)
struct Goal {
	float x;
	bool isActive;
};

// ボス(出現させるにはステージデータに MakeBoss を書く)
struct Boss {
	float x, y;
	float baseY;     // 上下に動くときの中心Y
	float wave;      // 上下運動の位相
	int hp, maxHp;
	int frame;       // 登場からの経過フレーム
	int aimTimer;    // 狙い撃ちの間隔カウンタ
	int fanTimer;    // 扇状弾の間隔カウンタ
	int hitFlash;    // 被弾したときの画像切り替えフレーム
	bool isArrived;  // 戦う位置に到着したか
	bool isActive;
};

// 画像1枚ぶんの情報
struct Sprite {
	int handle = -1; // Novice::LoadTexture が返す番号
	int w = 0;       // 画像の幅(px)
	int h = 0;       // 画像の高さ(px)
	bool IsValid() const { return handle >= 0 && w > 0 && h > 0; }
};

// ============================================================
// ステージデータ(ここを書き換えて配置をカスタマイズ!)
// ============================================================
// ・frame : 出現するフレーム(60フレーム = 1秒)。画面の右端から出てきます。
//           表の上から下へ、小さい順に並べてください。
// ・y     : 出現するY座標(画面の高さは720。上が0)
// ・横方向の位置は frame で調整します。
//   障害物・コイン・ゴールは 4px/フレーム、敵は 2px/フレームで左へ流れます。
//   (右端から自機のX=300付近まで、障害物は約245フレームで届きます)
// ・ゴール(MakeGoal)を置かないステージはクリアできません。必ず1つ入れてください。
// ・ボス(MakeBoss)は、倒すまでゴールが出なくなります。ボスより後ろにゴールを書いてください。
// ・大コイン(MakeBigCoin)は収集要素です。ステージをクリアすると取得数が記録されます。
// ・回復アイテム(MakeHeal)を取るとHPが1回復します(最大HPまで)。
// ・壊せないブロック(kObstacleSolid)は、どんな弾も防ぎます。すき間を通って避けてください。

enum SpawnKind {
	kSpawnObstacle,
	kSpawnEnemy,
	kSpawnCoin,
	kSpawnGoal,
	kSpawnBigCoin,
	kSpawnHeal,
	kSpawnBoss,
};

// 障害物の種類
const int kObstacleNormal = 0;     // 通常弾で壊せる        (画像: blockB)
const int kObstacleChargeOnly = 1; // チャージ弾でしか壊せない (画像: blockC)
const int kObstacleSolid = 2;      // 壊せない              (画像: blockA)

// 敵の種類
const int kEnemyShooter = 0; // 上下に揺れながら、自機を狙って弾を撃つ (画像: enemyA)
const int kEnemyStatic = 1;  // 揺れない・弾も撃たない               (画像: enemyB)

struct SpawnData {
	int frame;
	SpawnKind kind;
	float y;
	int type;   // 障害物・敵の種類
	float w, h; // 障害物の大きさ
	int hp;     // 敵のHP
	int count;  // コインの枚数
};

// 障害物: MakeObstacle(出現フレーム, Y座標, 幅, 高さ, 種類)
SpawnData MakeObstacle(int frame, float y, float w, float h, int type) {
	SpawnData d = {};
	d.frame = frame;
	d.kind = kSpawnObstacle;
	d.y = y;
	d.w = w;
	d.h = h;
	d.type = type;
	return d;
}

// 敵: MakeEnemy(出現フレーム, Y座標, 種類, HP)
SpawnData MakeEnemy(int frame, float y, int type, int hp) {
	SpawnData d = {};
	d.frame = frame;
	d.kind = kSpawnEnemy;
	d.y = y;
	d.type = type;
	d.hp = hp;
	return d;
}

// コイン: MakeCoin(出現フレーム, Y座標, 枚数)  ※横一列に並べて出ます
SpawnData MakeCoin(int frame, float y, int count) {
	SpawnData d = {};
	d.frame = frame;
	d.kind = kSpawnCoin;
	d.y = y;
	d.count = count;
	return d;
}

// ゴール: MakeGoal(出現フレーム)
SpawnData MakeGoal(int frame) {
	SpawnData d = {};
	d.frame = frame;
	d.kind = kSpawnGoal;
	return d;
}

// 大コイン: MakeBigCoin(出現フレーム, Y座標)  ※ステージごとに3枚置く想定
SpawnData MakeBigCoin(int frame, float y) {
	SpawnData d = {};
	d.frame = frame;
	d.kind = kSpawnBigCoin;
	d.y = y;
	return d;
}

// 回復アイテム: MakeHeal(出現フレーム, Y座標)
SpawnData MakeHeal(int frame, float y) {
	SpawnData d = {};
	d.frame = frame;
	d.kind = kSpawnHeal;
	d.y = y;
	return d;
}

// ボス: MakeBoss(出現フレーム, HP)
SpawnData MakeBoss(int frame, int hp) {
	SpawnData d = {};
	d.frame = frame;
	d.kind = kSpawnBoss;
	d.hp = hp;
	return d;
}

// ---------- STAGE 1:はじめの空 ----------
const SpawnData kStage1Data[] = {
	MakeCoin(60, 360.0f, 3),

	MakeObstacle(150, 0.0f, 40.0f, 250.0f, kObstacleNormal),   // 上から
	MakeObstacle(210, 470.0f, 40.0f, 250.0f, kObstacleNormal), // 下から
	MakeCoin(240, 300.0f, 3),

	MakeBigCoin(330, 110.0f), // 大コイン1枚目(画面の上のほう)
	MakeEnemy(360, 250.0f, kEnemyShooter, 3),
	MakeCoin(400, 450.0f, 2),

	MakeObstacle(480, 250.0f, 40.0f, 200.0f, kObstacleChargeOnly),
	MakeEnemy(540, 500.0f, kEnemyStatic, 3),

	// 同じフレームに2つ出すと、すき間のある壁になります
	MakeObstacle(600, 0.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeObstacle(600, 420.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeCoin(660, 360.0f, 3),

	MakeEnemy(780, 150.0f, kEnemyShooter, 3),
	MakeEnemy(780, 550.0f, kEnemyShooter, 3),
	MakeObstacle(840, 280.0f, 40.0f, 120.0f, kObstacleSolid), // 壊せないブロック
	MakeHeal(880, 360.0f), // 回復アイテム
	MakeCoin(900, 200.0f, 3),

	MakeObstacle(960, 300.0f, 40.0f, 120.0f, kObstacleNormal),
	MakeBigCoin(1000, 620.0f), // 大コイン2枚目(画面の下のほう)
	MakeEnemy(1020, 360.0f, kEnemyStatic, 5),
	MakeCoin(1100, 500.0f, 3),

	MakeObstacle(1200, 0.0f, 40.0f, 330.0f, kObstacleNormal),
	MakeObstacle(1200, 410.0f, 40.0f, 310.0f, kObstacleChargeOnly),
	MakeEnemy(1320, 200.0f, kEnemyShooter, 3),
	MakeCoin(1400, 350.0f, 3),

	MakeBigCoin(1450, 150.0f), // 大コイン3枚目
	MakeEnemy(1500, 450.0f, kEnemyStatic, 3),
	MakeObstacle(1560, 100.0f, 40.0f, 150.0f, kObstacleChargeOnly),
	MakeCoin(1650, 300.0f, 3),

	MakeGoal(1800),
};

// ---------- STAGE 2:砲台の谷(敵が多い) ----------
const SpawnData kStage2Data[] = {
	MakeCoin(60, 300.0f, 3),
	MakeEnemy(100, 200.0f, kEnemyShooter, 3),
	MakeEnemy(100, 500.0f, kEnemyShooter, 3),

	MakeObstacle(200, 0.0f, 40.0f, 280.0f, kObstacleNormal),
	MakeObstacle(200, 400.0f, 40.0f, 320.0f, kObstacleNormal),
	MakeBigCoin(230, 80.0f), // 大コイン1枚目
	MakeCoin(260, 340.0f, 4),

	MakeEnemy(360, 360.0f, kEnemyStatic, 5),
	MakeObstacle(420, 100.0f, 40.0f, 160.0f, kObstacleChargeOnly),
	MakeObstacle(420, 460.0f, 40.0f, 160.0f, kObstacleChargeOnly),
	MakeCoin(500, 360.0f, 3),

	MakeEnemy(560, 150.0f, kEnemyShooter, 3),
	MakeEnemy(620, 350.0f, kEnemyShooter, 3),
	MakeEnemy(680, 550.0f, kEnemyShooter, 3),
	MakeHeal(700, 450.0f), // 回復アイテム1つ目
	MakeCoin(740, 250.0f, 3),

	MakeObstacle(800, 0.0f, 40.0f, 350.0f, kObstacleChargeOnly),
	MakeObstacle(800, 430.0f, 40.0f, 290.0f, kObstacleNormal),
	MakeEnemy(860, 520.0f, kEnemyStatic, 3),
	MakeBigCoin(900, 640.0f), // 大コイン2枚目
	MakeCoin(950, 300.0f, 3),

	MakeObstacle(1000, 300.0f, 40.0f, 120.0f, kObstacleSolid), // 壊せないブロック
	MakeEnemy(1040, 200.0f, kEnemyStatic, 3),
	MakeEnemy(1040, 500.0f, kEnemyShooter, 3),
	MakeObstacle(1100, 250.0f, 40.0f, 220.0f, kObstacleNormal),
	MakeCoin(1180, 450.0f, 3),

	MakeEnemy(1260, 120.0f, kEnemyShooter, 3),
	MakeEnemy(1260, 360.0f, kEnemyShooter, 3),
	MakeEnemy(1260, 600.0f, kEnemyShooter, 3),
	MakeObstacle(1300, 320.0f, 40.0f, 100.0f, kObstacleSolid), // 壊せないブロック

	MakeObstacle(1360, 0.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeObstacle(1360, 420.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeHeal(1400, 250.0f), // 回復アイテム2つ目
	MakeCoin(1440, 360.0f, 3),
	MakeEnemy(1500, 360.0f, kEnemyStatic, 5),
	MakeBigCoin(1560, 120.0f), // 大コイン3枚目
	MakeCoin(1620, 200.0f, 3),

	MakeGoal(2000),
};

// ---------- STAGE 3:壁の連続 + ボス ----------
const SpawnData kStage3Data[] = {
	MakeCoin(60, 360.0f, 3),

	MakeObstacle(120, 0.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeObstacle(120, 400.0f, 40.0f, 320.0f, kObstacleNormal),
	MakeBigCoin(120, 350.0f), // 大コイン1枚目(壁のすき間の中)

	MakeObstacle(200, 0.0f, 40.0f, 400.0f, kObstacleNormal),
	MakeObstacle(200, 480.0f, 40.0f, 240.0f, kObstacleChargeOnly),
	MakeCoin(260, 440.0f, 3),

	MakeEnemy(300, 300.0f, kEnemyShooter, 3),
	MakeObstacle(320, 0.0f, 40.0f, 200.0f, kObstacleChargeOnly),
	MakeObstacle(320, 300.0f, 40.0f, 420.0f, kObstacleChargeOnly),

	MakeObstacle(400, 0.0f, 40.0f, 420.0f, kObstacleNormal),
	MakeObstacle(400, 500.0f, 40.0f, 220.0f, kObstacleNormal),
	MakeBigCoin(400, 460.0f), // 大コイン2枚目(壁のすき間の中)
	MakeCoin(460, 460.0f, 3),

	MakeEnemy(520, 200.0f, kEnemyStatic, 3),
	MakeEnemy(520, 520.0f, kEnemyShooter, 3),
	MakeObstacle(560, 0.0f, 40.0f, 250.0f, kObstacleChargeOnly),
	MakeObstacle(560, 350.0f, 40.0f, 370.0f, kObstacleChargeOnly),

	MakeObstacle(640, 0.0f, 40.0f, 330.0f, kObstacleNormal),
	MakeObstacle(640, 410.0f, 40.0f, 310.0f, kObstacleChargeOnly),
	MakeCoin(700, 370.0f, 3),

	MakeHeal(760, 360.0f), // 回復アイテム1つ目
	MakeObstacle(780, 0.0f, 40.0f, 200.0f, kObstacleNormal),
	MakeObstacle(780, 280.0f, 40.0f, 440.0f, kObstacleChargeOnly),
	MakeEnemy(840, 400.0f, kEnemyShooter, 3),

	MakeObstacle(900, 0.0f, 40.0f, 420.0f, kObstacleChargeOnly),
	MakeObstacle(900, 500.0f, 40.0f, 220.0f, kObstacleNormal),
	MakeCoin(960, 460.0f, 3),

	MakeEnemy(1040, 150.0f, kEnemyShooter, 3),
	MakeEnemy(1040, 600.0f, kEnemyShooter, 3),
	MakeObstacle(1100, 0.0f, 40.0f, 300.0f, kObstacleNormal),
	MakeObstacle(1100, 380.0f, 40.0f, 340.0f, kObstacleChargeOnly),
	MakeBigCoin(1100, 340.0f), // 大コイン3枚目(せまいすき間の中)
	MakeCoin(1180, 340.0f, 3),

	// 壊せないブロックの壁(すき間を通り抜ける)
	MakeObstacle(1220, 0.0f, 40.0f, 280.0f, kObstacleSolid),
	MakeObstacle(1220, 440.0f, 40.0f, 280.0f, kObstacleSolid),

	MakeEnemy(1260, 360.0f, kEnemyStatic, 5),
	MakeObstacle(1340, 0.0f, 40.0f, 340.0f, kObstacleChargeOnly),
	MakeObstacle(1340, 420.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeCoin(1420, 380.0f, 3),

	MakeHeal(1480, 360.0f), // ボス前の回復アイテム

	MakeBoss(1600, 40), // ボス登場(HP40)。倒すとゴールが出現します
	MakeGoal(1700),     // ボスが生きている間は、ここまで来ても出現を待ちます
};

// ---------- STAGE 4:要塞(壊せないブロックが多い) ----------
const SpawnData kStage4Data[] = {
	MakeCoin(60, 360.0f, 3),
	MakeEnemy(100, 200.0f, kEnemyShooter, 3),

	// 壊せないブロックの柱(すき間を通り抜ける)
	MakeObstacle(160, 0.0f, 40.0f, 300.0f, kObstacleSolid),
	MakeObstacle(160, 400.0f, 40.0f, 320.0f, kObstacleSolid),
	MakeBigCoin(160, 350.0f), // 大コイン1枚目(壊せないブロックのすき間の中)
	MakeCoin(230, 360.0f, 3),

	MakeEnemy(300, 500.0f, kEnemyShooter, 3),
	MakeObstacle(340, 0.0f, 40.0f, 240.0f, kObstacleChargeOnly),
	MakeObstacle(340, 340.0f, 40.0f, 380.0f, kObstacleNormal),
	MakeCoin(400, 290.0f, 3),

	MakeEnemy(460, 150.0f, kEnemyStatic, 5),
	MakeEnemy(460, 600.0f, kEnemyStatic, 5),
	MakeObstacle(520, 200.0f, 40.0f, 160.0f, kObstacleSolid),
	MakeObstacle(560, 440.0f, 40.0f, 160.0f, kObstacleSolid),
	MakeHeal(600, 320.0f), // 回復アイテム1つ目
	MakeCoin(640, 400.0f, 3),

	MakeEnemy(700, 250.0f, kEnemyShooter, 3),
	MakeEnemy(700, 450.0f, kEnemyShooter, 3),
	MakeObstacle(760, 0.0f, 40.0f, 360.0f, kObstacleNormal),
	MakeObstacle(760, 440.0f, 40.0f, 280.0f, kObstacleChargeOnly),
	MakeBigCoin(820, 650.0f), // 大コイン2枚目(画面の下のほう)
	MakeCoin(880, 400.0f, 3),

	MakeEnemy(940, 100.0f, kEnemyShooter, 3),
	MakeEnemy(940, 360.0f, kEnemyShooter, 3),
	MakeEnemy(940, 620.0f, kEnemyShooter, 3),
	MakeObstacle(1000, 0.0f, 40.0f, 200.0f, kObstacleSolid),
	MakeObstacle(1000, 300.0f, 40.0f, 420.0f, kObstacleChargeOnly),
	MakeHeal(1060, 250.0f), // 回復アイテム2つ目
	MakeCoin(1100, 250.0f, 3),

	MakeObstacle(1160, 0.0f, 40.0f, 400.0f, kObstacleChargeOnly),
	MakeObstacle(1160, 480.0f, 40.0f, 240.0f, kObstacleSolid),
	MakeBigCoin(1160, 440.0f), // 大コイン3枚目(すき間の中)
	MakeEnemy(1220, 300.0f, kEnemyStatic, 5),
	MakeCoin(1280, 440.0f, 3),

	MakeEnemy(1360, 200.0f, kEnemyShooter, 3),
	MakeEnemy(1360, 520.0f, kEnemyShooter, 3),
	MakeObstacle(1420, 0.0f, 40.0f, 300.0f, kObstacleNormal),
	MakeObstacle(1420, 380.0f, 40.0f, 340.0f, kObstacleNormal),
	MakeObstacle(1500, 100.0f, 40.0f, 160.0f, kObstacleSolid),
	MakeObstacle(1540, 460.0f, 40.0f, 160.0f, kObstacleSolid),
	MakeHeal(1580, 360.0f), // 回復アイテム3つ目
	MakeCoin(1620, 300.0f, 3),

	MakeGoal(1800),
};

// ---------- STAGE 5:最終ステージ(総まとめ + 強いボス) ----------
const SpawnData kStage5Data[] = {
	MakeCoin(60, 360.0f, 3),
	MakeEnemy(80, 150.0f, kEnemyShooter, 3),
	MakeEnemy(80, 570.0f, kEnemyShooter, 3),

	MakeObstacle(150, 0.0f, 40.0f, 260.0f, kObstacleSolid),
	MakeObstacle(150, 360.0f, 40.0f, 360.0f, kObstacleChargeOnly),
	MakeCoin(210, 310.0f, 3),

	MakeObstacle(260, 0.0f, 40.0f, 400.0f, kObstacleNormal),
	MakeObstacle(260, 480.0f, 40.0f, 240.0f, kObstacleSolid),
	MakeBigCoin(260, 440.0f), // 大コイン1枚目(すき間の中)
	MakeEnemy(320, 360.0f, kEnemyStatic, 5),
	MakeObstacle(380, 0.0f, 40.0f, 180.0f, kObstacleChargeOnly),
	MakeObstacle(380, 260.0f, 40.0f, 460.0f, kObstacleChargeOnly),
	MakeCoin(440, 220.0f, 3),

	MakeEnemy(500, 200.0f, kEnemyShooter, 3),
	MakeEnemy(500, 500.0f, kEnemyShooter, 3),
	MakeObstacle(560, 160.0f, 40.0f, 120.0f, kObstacleSolid),
	MakeObstacle(600, 420.0f, 40.0f, 120.0f, kObstacleSolid),
	MakeHeal(640, 340.0f), // 回復アイテム1つ目
	MakeCoin(680, 540.0f, 3),

	MakeObstacle(740, 0.0f, 40.0f, 340.0f, kObstacleNormal),
	MakeObstacle(740, 420.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeEnemy(780, 100.0f, kEnemyShooter, 3),
	MakeEnemy(780, 360.0f, kEnemyShooter, 3),
	MakeEnemy(780, 620.0f, kEnemyShooter, 3),
	MakeBigCoin(840, 90.0f), // 大コイン2枚目(画面の上のほう)
	MakeObstacle(860, 0.0f, 40.0f, 200.0f, kObstacleChargeOnly),
	MakeObstacle(860, 300.0f, 40.0f, 420.0f, kObstacleSolid),
	MakeCoin(920, 250.0f, 3),

	MakeEnemy(980, 300.0f, kEnemyStatic, 5),
	MakeEnemy(980, 500.0f, kEnemyStatic, 5),
	MakeObstacle(1040, 0.0f, 40.0f, 440.0f, kObstacleChargeOnly),
	MakeObstacle(1040, 520.0f, 40.0f, 200.0f, kObstacleNormal),
	MakeHeal(1100, 480.0f), // 回復アイテム2つ目
	MakeCoin(1140, 480.0f, 3),

	MakeObstacle(1200, 0.0f, 40.0f, 240.0f, kObstacleSolid),
	MakeObstacle(1200, 320.0f, 40.0f, 400.0f, kObstacleNormal),
	MakeEnemy(1260, 180.0f, kEnemyShooter, 3),
	MakeEnemy(1260, 540.0f, kEnemyShooter, 3),
	MakeObstacle(1320, 0.0f, 40.0f, 300.0f, kObstacleNormal),
	MakeObstacle(1320, 380.0f, 40.0f, 340.0f, kObstacleSolid),
	MakeBigCoin(1320, 340.0f), // 大コイン3枚目(せまいすき間の中)
	MakeCoin(1400, 340.0f, 3),

	MakeEnemy(1460, 120.0f, kEnemyShooter, 3),
	MakeEnemy(1460, 360.0f, kEnemyShooter, 3),
	MakeEnemy(1460, 600.0f, kEnemyShooter, 3),
	MakeObstacle(1520, 0.0f, 40.0f, 320.0f, kObstacleChargeOnly),
	MakeObstacle(1520, 400.0f, 40.0f, 320.0f, kObstacleChargeOnly),
	MakeObstacle(1600, 200.0f, 40.0f, 140.0f, kObstacleSolid),
	MakeCoin(1660, 500.0f, 3),
	MakeObstacle(1720, 0.0f, 40.0f, 300.0f, kObstacleNormal),
	MakeObstacle(1720, 400.0f, 40.0f, 320.0f, kObstacleNormal),
	MakeHeal(1800, 360.0f), // ボス前の回復アイテム

	MakeBoss(2000, 60), // ボス登場(HP60。ステージ3のボスより頑丈)
	MakeGoal(2100),     // ボスを倒すまで、出現を待ちます
};

// ---------- ステージ一覧(ステージ選択画面に並ぶ順) ----------
// ステージを増やすときは、上に kStage6Data[] を作って、ここに1行足すだけです。
struct StageInfo {
	const char* name;
	const SpawnData* data;
	int count;
};

const StageInfo kStages[] = {
	{"Sky Walk", kStage1Data, static_cast<int>(sizeof(kStage1Data) / sizeof(kStage1Data[0]))},
	{"Cannon Valley", kStage2Data, static_cast<int>(sizeof(kStage2Data) / sizeof(kStage2Data[0]))},
	{"Wall Rush", kStage3Data, static_cast<int>(sizeof(kStage3Data) / sizeof(kStage3Data[0]))},
	{"Fortress Run", kStage4Data, static_cast<int>(sizeof(kStage4Data) / sizeof(kStage4Data[0]))},
	{"Final Skies", kStage5Data, static_cast<int>(sizeof(kStage5Data) / sizeof(kStage5Data[0]))},
};
const int kStageCount = static_cast<int>(sizeof(kStages) / sizeof(kStages[0]));

// ============================================================
// ゲームデータ
// ============================================================
Player player;
Bullet playerBullets[kMaxBullets];
Bullet enemyBullets[kMaxBullets];
Obstacle obstacles[kMaxObstacles];
Enemy enemies[kMaxEnemies];
Coin coins[kMaxCoins];
Goal goal;
Boss boss;
Coin bigCoins[kMaxBigCoins]; // 大コイン(収集要素)
Coin heals[kMaxHeals];       // 回復アイテム

int stageFrame; // ステージ開始からの経過フレーム
int spawnIndex; // 次に出現させるステージデータの番号

Scene scene = kSceneTitle;
int titleFrame = 0;     // タイトル画面の演出用カウンタ
int selectedStage = 0;  // ステージ選択でカーソルがあるステージ
int currentStage = 0;   // プレイ中のステージ
bool isGameOver = false;
bool isStageClear = false;
int clearBonus = 0;

bool stageCleared[kStageCount]; // 一度でもクリアしたか
int bestScore[kStageCount];     // ステージごとのハイスコア
int bigCoinRecord[kStageCount]; // ステージごとの大コイン取得記録(クリア時に更新)

// デバッグ表示:true の間、当たり判定を半透明の図形で表示する(F1キーで切り替え)
// ※完成版では false にしておくこと
bool debug = true;

float bgOffset = 0.0f; // 背景のスクロール量(px)

// ============================================================
// 画像(スプライト)
// ============================================================
Sprite sprBg;          // 背景
Sprite sprPlayer;      // 自機
Sprite sprEnemyA;      // 敵(揺れて撃つ)
Sprite sprEnemyB;      // 敵(揺れず撃たない)
Sprite sprBoss1;       // ボス(通常)
Sprite sprBoss2;       // ボス(ダメージを受けたとき)
Sprite sprNormalShot;  // 自機の通常弾
Sprite sprChargeShot;  // 自機のチャージ弾
Sprite sprEnemyShot;   // 敵・ボスの弾
Sprite sprBlockA;      // 壊せないブロック
Sprite sprBlockB;      // 通常弾で壊せるブロック
Sprite sprBlockC;      // チャージ弾でのみ壊せるブロック
Sprite sprGoalTile;    // ゴール
Sprite sprCoin;        // コイン
Sprite sprBigCoin;     // 大コイン
Sprite sprHeal;        // 回復アイテム
Sprite sprHeart1;      // HP(あり)
Sprite sprHeart2;      // HP(なし)

// PNGファイルの先頭(IHDR)から、画像の幅と高さを読み取る
bool ReadPngSize(const char* path, int* outW, int* outH) {
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) return false;

	unsigned char header[24] = {};
	ifs.read(reinterpret_cast<char*>(header), sizeof(header));
	if (ifs.gcount() < 24) return false;

	// PNGの目印(先頭4バイト)を確認
	if (header[0] != 0x89 || header[1] != 'P' || header[2] != 'N' || header[3] != 'G') return false;

	// 幅と高さは 16〜23 バイト目に、上位バイトから順に入っている
	*outW = (header[16] << 24) | (header[17] << 16) | (header[18] << 8) | header[19];
	*outH = (header[20] << 24) | (header[21] << 16) | (header[22] << 8) | header[23];
	return true;
}

// images フォルダから画像を読み込む(見つからなければ「無効な画像」を返す)
Sprite LoadSprite(const char* fileName) {
	Sprite s;
	std::string path = std::string(kImageDir) + fileName;
	if (!ReadPngSize(path.c_str(), &s.w, &s.h)) {
		return s; // 読み込めない → 描画時は代わりの四角形になる
	}
	s.handle = Novice::LoadTexture(path.c_str());
	return s;
}

// すべての画像を読み込む(Novice::Initialize の後に1回だけ呼ぶ)
void LoadSprites() {
	sprBg = LoadSprite("td1-1_bg_1.png");
	sprPlayer = LoadSprite("td1-1_player1.png");
	sprEnemyA = LoadSprite("td1-1_enemyA.png");
	sprEnemyB = LoadSprite("td1-1_enemyB.png");
	sprBoss1 = LoadSprite("td1-1_boss1.png");
	sprBoss2 = LoadSprite("td1-1_boss2.png");
	sprNormalShot = LoadSprite("td1-1_normal_shot.png");
	sprChargeShot = LoadSprite("td1-1_charge_shot.png");
	sprEnemyShot = LoadSprite("td1-1_enemy_shot.png");
	sprBlockA = LoadSprite("td1-1_blockA.png");
	sprBlockB = LoadSprite("td1-1_blockB.png");
	sprBlockC = LoadSprite("td1-1_blockC.png");
	sprGoalTile = LoadSprite("td1-1_goal_tile.png");
	sprCoin = LoadSprite("td1-1_coin.png");
	sprBigCoin = LoadSprite("td1-1_big_coin.png");
	sprHeal = LoadSprite("td1-1_heal.png");
	sprHeart1 = LoadSprite("td1-1_ui_heart1.png");
	sprHeart2 = LoadSprite("td1-1_ui_heart2.png");
}

// 画像を (x, y) を左上として、幅w・高さh にぴったり合わせて描く
// 画像が読み込めていないときは、代わりに fallbackColor の四角形を描く
void DrawSpriteFit(const Sprite& s, int x, int y, int w, int h, unsigned int fallbackColor, unsigned int color = WHITE) {
	if (!s.IsValid()) {
		Novice::DrawBox(x, y, w, h, 0.0f, fallbackColor, kFillModeSolid);
		return;
	}
	Novice::DrawSprite(x, y, s.handle, static_cast<float>(w) / s.w, static_cast<float>(h) / s.h, 0.0f, color);
}

// 円形の物(コイン・弾など)を、中心(cx, cy)・半径r で描く
void DrawSpriteCircle(const Sprite& s, float cx, float cy, float r, unsigned int fallbackColor, unsigned int color = WHITE) {
	if (!s.IsValid()) {
		Novice::DrawEllipse(static_cast<int>(cx), static_cast<int>(cy), static_cast<int>(r), static_cast<int>(r), 0.0f, fallbackColor, kFillModeSolid);
		return;
	}
	int d = static_cast<int>(r * 2.0f);
	DrawSpriteFit(s, static_cast<int>(cx - r), static_cast<int>(cy - r), d, d, fallbackColor, color);
}

// 画像を縦に count 枚ならべて、(x, y) から 幅w・高さh の範囲をうめる
// (高さがぴったり割り切れなくても、すき間なく収まるよう少しだけ伸縮する)
void DrawSpriteColumn(const Sprite& s, int x, int y, int w, int h, int count, unsigned int fallbackColor) {
	if (count < 1) count = 1;
	for (int i = 0; i < count; i++) {
		int top = y + h * i / count;
		int bottom = y + h * (i + 1) / count;
		DrawSpriteFit(s, x, top, w, bottom - top, fallbackColor);
	}
}

// 背景を描く(画面の高さに合わせて拡大し、横にくり返し並べてスクロールさせる)
void DrawBackground() {
	if (!sprBg.IsValid()) return; // 画像がなければ黒背景のまま

	float scale = static_cast<float>(kScreenH) / sprBg.h;
	int tileW = static_cast<int>(sprBg.w * scale);
	if (tileW <= 0) return;

	int offset = static_cast<int>(bgOffset) % tileW;
	for (int x = -offset; x < kScreenW; x += tileW) {
		Novice::DrawSprite(x, 0, sprBg.handle, scale, scale, 0.0f, WHITE);
	}
}

// ============================================================
// ユーティリティ
// ============================================================
// 円と矩形の当たり判定
bool CircleRectHit(float cx, float cy, float r, float rx, float ry, float rw, float rh) {
	float nx = cx;
	float ny = cy;
	if (nx < rx) nx = rx;
	if (nx > rx + rw) nx = rx + rw;
	if (ny < ry) ny = ry;
	if (ny > ry + rh) ny = ry + rh;
	float dx = cx - nx;
	float dy = cy - ny;
	return dx * dx + dy * dy <= r * r;
}

// 矩形と矩形の当たり判定
bool RectRectHit(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
	return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

// 障害物の i 番目のタイルの境目のY座標(i = 0 が上端、i = tileCount が下端)
// 描画と当たり判定の両方で同じ式を使うので、見た目と判定がずれない
float TileEdgeY(const Obstacle& o, int i) {
	return o.y + o.h * i / o.tileCount;
}

// タイルを1つ壊す。全部壊れたら障害物ごと消す
void DestroyTile(Obstacle& o, int i) {
	if (!o.tileAlive[i]) return;
	o.tileAlive[i] = false;
	o.aliveTiles--;
	if (o.aliveTiles <= 0) {
		o.isActive = false;
	}
}

// キーが「押された瞬間」か
bool Triggered(const char* keys, const char* preKeys, int key) { return preKeys[key] == 0 && keys[key] != 0; }

// ステージデータの中に、指定した種類の配置がいくつあるか数える
int CountSpawnKind(int stage, SpawnKind kind) {
	int n = 0;
	for (int i = 0; i < kStages[stage].count; i++) {
		if (kStages[stage].data[i].kind == kind) n++;
	}
	return n;
}

// HPバーを描く
void DrawHpBar(int x, int y, int w, int h, int hp, int maxHp, unsigned int color) {
	if (maxHp <= 0) return;
	if (hp < 0) hp = 0;
	Novice::DrawBox(x, y, w, h, 0.0f, kColorDarkGray, kFillModeSolid);
	Novice::DrawBox(x, y, w * hp / maxHp, h, 0.0f, color, kFillModeSolid);
	Novice::DrawBox(x, y, w, h, 0.0f, WHITE, kFillModeWireFrame);
}

// デバッグ表示:当たり判定(四角)を半透明で描く
void DebugBox(float x, float y, float w, float h, unsigned int color) {
	if (!debug) return;
	Novice::DrawBox(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), 0.0f, color, kFillModeSolid);
}

// デバッグ表示:当たり判定(円)を半透明で描く
void DebugCircle(float cx, float cy, float r, unsigned int color) {
	if (!debug) return;
	Novice::DrawEllipse(static_cast<int>(cx), static_cast<int>(cy), static_cast<int>(r), static_cast<int>(r), 0.0f, color, kFillModeSolid);
}

// プレイヤーにダメージ(無敵中は無効)
void DamagePlayer(int damage) {
	if (player.invincible > 0) return;
	player.hp -= damage;
	player.invincible = kInvincibleFrames;
}

// 空いている弾スロットを探す(なければ -1)
int FindFreeBullet(Bullet* bullets) {
	for (int i = 0; i < kMaxBullets; i++) {
		if (!bullets[i].isActive) return i;
	}
	return -1;
}

// ============================================================
// 初期化・リセット(指定したステージを最初からやり直す)
// ============================================================
void ResetGame(int stage) {
	currentStage = stage;

	player.x = 300.0f;
	player.y = 300.0f;
	player.size = 40.0f;
	player.vy = 0.0f;
	player.hp = kPlayerMaxHp;
	player.invincible = 0;
	player.charge = 0;
	player.score = 0;
	player.coins = 0;
	player.bigCoins = 0;

	for (int i = 0; i < kMaxBullets; i++) {
		playerBullets[i].isActive = false;
		enemyBullets[i].isActive = false;
	}
	for (int i = 0; i < kMaxObstacles; i++) obstacles[i].isActive = false;
	for (int i = 0; i < kMaxEnemies; i++) enemies[i].isActive = false;
	for (int i = 0; i < kMaxCoins; i++) coins[i].isActive = false;
	goal.x = 0.0f;
	goal.isActive = false;
	for (int i = 0; i < kMaxBigCoins; i++) bigCoins[i].isActive = false;
	for (int i = 0; i < kMaxHeals; i++) heals[i].isActive = false;
	boss.isActive = false;

	stageFrame = 0;
	spawnIndex = 0;
	isGameOver = false;
	isStageClear = false;
	clearBonus = 0;
}

// ============================================================
// スポーン処理(ステージデータから生成)
// ============================================================
void SpawnObstacle(const SpawnData& d) {
	for (int i = 0; i < kMaxObstacles; i++) {
		if (obstacles[i].isActive) continue;
		obstacles[i].x = static_cast<float>(kScreenW);
		obstacles[i].y = d.y;
		obstacles[i].w = d.w;
		obstacles[i].h = d.h;
		obstacles[i].type = d.type;
		obstacles[i].isActive = true;

		// 縦に並べるタイルの数(高さ ÷ 幅 を四捨五入)。最初は全部残っている
		int n = static_cast<int>(d.h / d.w + 0.5f);
		if (n < 1) n = 1;
		if (n > kMaxTilesPerObstacle) n = kMaxTilesPerObstacle;
		obstacles[i].tileCount = n;
		obstacles[i].aliveTiles = n;
		for (int t = 0; t < kMaxTilesPerObstacle; t++) {
			obstacles[i].tileAlive[t] = (t < n);
		}
		return;
	}
}

void SpawnEnemy(const SpawnData& d) {
	for (int i = 0; i < kMaxEnemies; i++) {
		if (enemies[i].isActive) continue;
		enemies[i].x = static_cast<float>(kScreenW + 40);
		enemies[i].baseY = d.y;
		enemies[i].y = d.y;
		enemies[i].size = 40.0f;
		enemies[i].hp = d.hp;
		enemies[i].maxHp = d.hp;
		enemies[i].shotTimer = 60;
		enemies[i].frame = 0;
		enemies[i].type = d.type;
		enemies[i].isActive = true;
		return;
	}
}

// コインを横一列に並べて出す
void SpawnCoins(const SpawnData& d) {
	for (int n = 0; n < d.count; n++) {
		for (int i = 0; i < kMaxCoins; i++) {
			if (coins[i].isActive) continue;
			coins[i].x = static_cast<float>(kScreenW + n * 40);
			coins[i].y = d.y;
			coins[i].radius = 12.0f;
			coins[i].isActive = true;
			break;
		}
	}
}

void SpawnGoal() {
	goal.x = static_cast<float>(kScreenW);
	goal.isActive = true;
}

// 大コイン
void SpawnBigCoin(const SpawnData& d) {
	for (int i = 0; i < kMaxBigCoins; i++) {
		if (bigCoins[i].isActive) continue;
		bigCoins[i].x = static_cast<float>(kScreenW);
		bigCoins[i].y = d.y;
		bigCoins[i].radius = 24.0f;
		bigCoins[i].isActive = true;
		return;
	}
}

// 回復アイテム
void SpawnHeal(const SpawnData& d) {
	for (int i = 0; i < kMaxHeals; i++) {
		if (heals[i].isActive) continue;
		heals[i].x = static_cast<float>(kScreenW);
		heals[i].y = d.y;
		heals[i].radius = 14.0f;
		heals[i].isActive = true;
		return;
	}
}

// ボス
void SpawnBoss(const SpawnData& d) {
	boss.x = static_cast<float>(kScreenW);
	boss.baseY = 300.0f;
	boss.y = boss.baseY;
	boss.wave = 0.0f;
	boss.hp = d.hp;
	boss.maxHp = d.hp;
	boss.frame = 0;
	boss.aimTimer = 60;
	boss.fanTimer = 100;
	boss.hitFlash = 0;
	boss.isArrived = false;
	boss.isActive = true;
}

void SpawnFromData(const SpawnData& d) {
	switch (d.kind) {
	case kSpawnObstacle:
		SpawnObstacle(d);
		break;
	case kSpawnEnemy:
		SpawnEnemy(d);
		break;
	case kSpawnCoin:
		SpawnCoins(d);
		break;
	case kSpawnGoal:
		SpawnGoal();
		break;
	case kSpawnBigCoin:
		SpawnBigCoin(d);
		break;
	case kSpawnHeal:
		SpawnHeal(d);
		break;
	case kSpawnBoss:
		SpawnBoss(d);
		break;
	}
}

// ============================================================
// 弾の発射
// ============================================================
void FirePlayerBullet(bool isCharged) {
	int idx = FindFreeBullet(playerBullets);
	if (idx < 0) return;
	Bullet& b = playerBullets[idx];
	b.x = player.x + player.size;
	b.y = player.y + player.size / 2.0f;
	b.vy = 0.0f;
	b.isCharged = isCharged;
	b.isActive = true;
	if (isCharged) {
		b.vx = 10.0f;
		b.radius = 20.0f;
	}
	else {
		b.vx = 12.0f;
		b.radius = 6.0f;
	}
}

// 敵から自機へ向けて発射(自機狙い)
void FireEnemyBullet(const Enemy& e) {
	int idx = FindFreeBullet(enemyBullets);
	if (idx < 0) return;

	float ex = e.x + e.size / 2.0f;
	float ey = e.y + e.size / 2.0f;
	float px = player.x + player.size / 2.0f;
	float py = player.y + player.size / 2.0f;

	float dx = px - ex;
	float dy = py - ey;
	float len = std::sqrt(dx * dx + dy * dy);
	if (len < 0.001f) return;

	const float speed = 5.0f;
	Bullet& b = enemyBullets[idx];
	b.x = ex;
	b.y = ey;
	b.vx = dx / len * speed;
	b.vy = dy / len * speed;
	b.radius = 8.0f;
	b.isCharged = false;
	b.isActive = true;
}

// ボスの攻撃:自機の方向を中心に、count発を扇状に撃つ(count=1なら狙い撃ち)
void FireBossFan(int count, float spread, float speed, float radius) {
	float bx = boss.x + kBossW / 2.0f;
	float by = boss.y + kBossH / 2.0f;
	float px = player.x + player.size / 2.0f;
	float py = player.y + player.size / 2.0f;
	float baseAngle = std::atan2(py - by, px - bx);

	for (int n = 0; n < count; n++) {
		int idx = FindFreeBullet(enemyBullets);
		if (idx < 0) return;
		float angle = baseAngle + (n - (count - 1) / 2.0f) * spread;
		Bullet& b = enemyBullets[idx];
		b.x = bx;
		b.y = by;
		b.vx = std::cos(angle) * speed;
		b.vy = std::sin(angle) * speed;
		b.radius = radius;
		b.isCharged = false;
		b.isActive = true;
	}
}

// ============================================================
// 更新処理:タイトル画面
// ============================================================
void UpdateTitle(const char* keys, const char* preKeys) {
	titleFrame++;
	bgOffset += kBgScrollSpeed;

	// ENTERでステージ選択へ
	if (Triggered(keys, preKeys, DIK_RETURN)) {
		scene = kSceneStageSelect;
	}
}

// ============================================================
// 更新処理:ステージ選択
// ============================================================
void UpdateStageSelect(const char* keys, const char* preKeys) {
	bgOffset += kBgScrollSpeed;

	// 左右キー(またはA/D)でカーソル移動
	if (Triggered(keys, preKeys, DIK_LEFT) || Triggered(keys, preKeys, DIK_A)) {
		selectedStage = (selectedStage + kStageCount - 1) % kStageCount;
	}
	if (Triggered(keys, preKeys, DIK_RIGHT) || Triggered(keys, preKeys, DIK_D)) {
		selectedStage = (selectedStage + 1) % kStageCount;
	}

	// ENTERで決定してプレイ開始
	if (Triggered(keys, preKeys, DIK_RETURN)) {
		ResetGame(selectedStage);
		scene = kScenePlay;
	}

	// BACKSPACEでタイトルへ戻る
	if (Triggered(keys, preKeys, DIK_BACK)) {
		scene = kSceneTitle;
	}
}

// ============================================================
// 更新処理:プレイ中
// ============================================================
void UpdatePlay(const char* keys, const char* preKeys) {

	// ---------- ゲームオーバー・クリア後の操作 ----------
	if (isGameOver || isStageClear) {
		if (Triggered(keys, preKeys, DIK_R)) {
			ResetGame(currentStage); // 同じステージをやり直す
		}
		else if (Triggered(keys, preKeys, DIK_RETURN)) {
			selectedStage = currentStage;
			scene = kSceneStageSelect;
		}
		return;
	}

	// ---------- 背景のスクロール ----------
	bgOffset += kBgScrollSpeed;

	// ---------- プレイヤー:重力・浮力 ----------
	player.vy += kGravity;

	if (keys[DIK_SPACE] != 0) {
		// スペース押下中は浮力 + チャージ
		player.vy += kBuoyancy;
		if (player.charge < kChargeNeed) {
			player.charge++;
		}
	}

	if (player.vy > kMaxFallSpeed) player.vy = kMaxFallSpeed;
	if (player.vy < kMaxRiseSpeed) player.vy = kMaxRiseSpeed;
	player.y += player.vy;

	// 画面の上端で止める
	if (player.y < 0.0f) {
		player.y = 0.0f;
		player.vy = 0.0f;
	}

	// ---------- プレイヤー:弾の発射(スペースを離した瞬間) ----------
	if (preKeys[DIK_SPACE] != 0 && keys[DIK_SPACE] == 0) {
		FirePlayerBullet(player.charge >= kChargeNeed);
		player.charge = 0;
	}

	// 無敵時間の減少
	if (player.invincible > 0) player.invincible--;

	// ---------- スポーン(ステージデータに従って出現) ----------
	const StageInfo& stage = kStages[currentStage];
	while (spawnIndex < stage.count && stage.data[spawnIndex].frame <= stageFrame) {
		// ボスが生きている間は、ゴールを出さずに待つ
		if (stage.data[spawnIndex].kind == kSpawnGoal && boss.isActive) break;
		SpawnFromData(stage.data[spawnIndex]);
		spawnIndex++;
	}
	stageFrame++;

	// ---------- 障害物の移動 ----------
	for (int i = 0; i < kMaxObstacles; i++) {
		if (!obstacles[i].isActive) continue;
		obstacles[i].x -= kScrollSpeed;
		if (obstacles[i].x + obstacles[i].w < 0.0f) {
			obstacles[i].isActive = false;
		}
	}

	// ---------- コインの移動 ----------
	for (int i = 0; i < kMaxCoins; i++) {
		if (!coins[i].isActive) continue;
		coins[i].x -= kScrollSpeed;
		if (coins[i].x + coins[i].radius < 0.0f) {
			coins[i].isActive = false;
		}
	}

	// ---------- 大コイン・回復アイテムの移動 ----------
	for (int i = 0; i < kMaxBigCoins; i++) {
		if (!bigCoins[i].isActive) continue;
		bigCoins[i].x -= kScrollSpeed;
		if (bigCoins[i].x + bigCoins[i].radius < 0.0f) bigCoins[i].isActive = false;
	}
	for (int i = 0; i < kMaxHeals; i++) {
		if (!heals[i].isActive) continue;
		heals[i].x -= kScrollSpeed;
		if (heals[i].x + heals[i].radius < 0.0f) heals[i].isActive = false;
	}

	// ---------- ゴールの移動 ----------
	if (goal.isActive) {
		goal.x -= kScrollSpeed;
	}

	// ---------- 敵の移動・射撃 ----------
	for (int i = 0; i < kMaxEnemies; i++) {
		Enemy& e = enemies[i];
		if (!e.isActive) continue;

		e.frame++;
		e.x -= 2.0f;

		if (e.type == kEnemyShooter) {
			// 上下にゆらゆら揺れる
			e.y = e.baseY + std::sin(e.frame * 0.05f) * 40.0f;

			// 画面内にいるときだけ、自機を狙って撃つ
			e.shotTimer--;
			if (e.shotTimer <= 0) {
				if (e.x < kScreenW - 40 && e.x + e.size > 0) {
					FireEnemyBullet(e);
				}
				e.shotTimer = 90;
			}
		}
		// kEnemyStatic は揺れず、撃たない(Y座標は出現時のまま)

		if (e.x + e.size < 0.0f) {
			e.isActive = false;
		}
	}

	// ---------- ボスの移動・攻撃 ----------
	if (boss.isActive) {
		boss.frame++;
		if (boss.hitFlash > 0) boss.hitFlash--;

		if (!boss.isArrived) {
			// 右から登場する
			boss.x -= 3.0f;
			if (boss.x <= kBossStopX) {
				boss.x = kBossStopX;
				boss.isArrived = true;
			}
		}
		else {
			// HPが半分以下になると、動きも攻撃も激しくなる
			bool isEnraged = boss.hp * 2 <= boss.maxHp;

			// 上下にゆっくり動く
			boss.wave += isEnraged ? 0.045f : 0.03f;
			boss.y = boss.baseY + std::sin(boss.wave) * 200.0f;

			// 自機を狙う単発弾
			boss.aimTimer--;
			if (boss.aimTimer <= 0) {
				FireBossFan(1, 0.0f, 6.0f, 9.0f);
				boss.aimTimer = isEnraged ? 40 : 65;
			}

			// 扇状の弾(通常3発、激しいときは5発)
			boss.fanTimer--;
			if (boss.fanTimer <= 0) {
				FireBossFan(isEnraged ? 5 : 3, 0.3f, 4.5f, 8.0f);
				boss.fanTimer = isEnraged ? 100 : 150;
			}
		}
	}

	// ---------- プレイヤーの弾の移動 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		Bullet& b = playerBullets[i];
		if (!b.isActive) continue;
		b.x += b.vx;
		b.y += b.vy;
		if (b.x - b.radius > kScreenW) b.isActive = false;
	}

	// ---------- 敵の弾の移動 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		Bullet& b = enemyBullets[i];
		if (!b.isActive) continue;
		b.x += b.vx;
		b.y += b.vy;
		if (b.x < -50 || b.x > kScreenW + 50 || b.y < -50 || b.y > kScreenH + 50) {
			b.isActive = false;
		}
	}

	// ---------- 当たり判定:プレイヤーの弾 × 障害物(タイル1つずつ) ----------
	for (int i = 0; i < kMaxBullets; i++) {
		Bullet& b = playerBullets[i];
		if (!b.isActive) continue;

		for (int j = 0; j < kMaxObstacles; j++) {
			Obstacle& o = obstacles[j];
			if (!o.isActive) continue;

			bool blocked = false;    // この障害物で弾が止められたか
			int nearestTile = -1;    // 通常弾が壊すタイル(当たったうち弾に一番近いもの)
			float nearestDist = 0.0f;

			for (int t = 0; t < o.tileCount; t++) {
				if (!o.tileAlive[t]) continue;

				float top = TileEdgeY(o, t);
				float height = TileEdgeY(o, t + 1) - top;
				if (!CircleRectHit(b.x, b.y, b.radius, o.x, top, o.w, height)) continue;

				// 壊せないブロック:どんな弾も防がれて消える
				if (o.type == kObstacleSolid) {
					blocked = true;
					break;
				}

				if (b.isCharged) {
					// チャージ弾:触れたタイルをすべて壊し、弾は貫通する
					DestroyTile(o, t);
					player.score += kTileScore;
				}
				else {
					// 通常弾:ここで止まる。壊せるのは通常ブロックのタイル1つだけ
					blocked = true;
					if (o.type == kObstacleNormal) {
						float dist = std::fabs(top + height / 2.0f - b.y);
						if (nearestTile < 0 || dist < nearestDist) {
							nearestTile = t;
							nearestDist = dist;
						}
					}
				}
			}

			if (nearestTile >= 0) {
				DestroyTile(o, nearestTile);
				player.score += kTileScore;
			}
			if (blocked) {
				b.isActive = false;
				break;
			}
		}
	}

	// ---------- 当たり判定:プレイヤーの弾 × 敵 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		Bullet& b = playerBullets[i];
		if (!b.isActive) continue;

		for (int j = 0; j < kMaxEnemies; j++) {
			Enemy& e = enemies[j];
			if (!e.isActive) continue;
			if (!CircleRectHit(b.x, b.y, b.radius, e.x, e.y, e.size, e.size)) continue;

			e.hp -= b.isCharged ? 3 : 1; // チャージ弾は3ダメージ
			b.isActive = false;
			if (e.hp <= 0) {
				e.isActive = false;
				player.score += 100;
			}
			break;
		}
	}

	// ---------- 当たり判定:プレイヤーの弾 × ボス ----------
	if (boss.isActive) {
		for (int i = 0; i < kMaxBullets; i++) {
			Bullet& b = playerBullets[i];
			if (!b.isActive) continue;
			if (!CircleRectHit(b.x, b.y, b.radius, boss.x, boss.y, kBossW, kBossH)) continue;

			boss.hp -= b.isCharged ? 3 : 1; // チャージ弾は3ダメージ
			boss.hitFlash = 6;
			b.isActive = false;

			if (boss.hp <= 0) {
				boss.hp = 0;
				boss.isActive = false;
				player.score += 1000;
				// ボスを倒したら、画面上の敵の弾を消す
				for (int j = 0; j < kMaxBullets; j++) enemyBullets[j].isActive = false;
				break;
			}
		}
	}

	// ---------- 当たり判定:プレイヤー × ボス(体当たり) ----------
	if (boss.isActive && RectRectHit(player.x, player.y, player.size, player.size, boss.x, boss.y, kBossW, kBossH)) {
		DamagePlayer(1);
	}

	// ---------- 当たり判定:プレイヤー × 障害物(タイル1つずつ) ----------
	if (player.invincible == 0) {
		bool isHit = false;
		for (int i = 0; i < kMaxObstacles; i++) {
			Obstacle& o = obstacles[i];
			if (!o.isActive) continue;

			for (int t = 0; t < o.tileCount; t++) {
				if (!o.tileAlive[t]) continue;

				float top = TileEdgeY(o, t);
				float height = TileEdgeY(o, t + 1) - top;
				if (!RectRectHit(player.x, player.y, player.size, player.size, o.x, top, o.w, height)) continue;

				isHit = true;
				// ぶつかったタイルは壊れる(壊せないブロックだけは残る)
				if (o.type != kObstacleSolid) {
					DestroyTile(o, t);
				}
			}
		}
		if (isHit) {
			DamagePlayer(1); // 何枚のタイルにぶつかっても、ダメージは1回
		}
	}

	// ---------- 当たり判定:プレイヤー × 敵の弾 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		Bullet& b = enemyBullets[i];
		if (!b.isActive) continue;
		if (CircleRectHit(b.x, b.y, b.radius, player.x, player.y, player.size, player.size)) {
			b.isActive = false;
			DamagePlayer(1);
		}
	}

	// ---------- 当たり判定:プレイヤー × コイン ----------
	for (int i = 0; i < kMaxCoins; i++) {
		Coin& c = coins[i];
		if (!c.isActive) continue;
		if (CircleRectHit(c.x, c.y, c.radius, player.x, player.y, player.size, player.size)) {
			c.isActive = false;
			player.coins++;
			player.score += 50;
		}
	}

	// ---------- 当たり判定:プレイヤー × 大コイン ----------
	for (int i = 0; i < kMaxBigCoins; i++) {
		Coin& c = bigCoins[i];
		if (!c.isActive) continue;
		if (CircleRectHit(c.x, c.y, c.radius, player.x, player.y, player.size, player.size)) {
			c.isActive = false;
			player.bigCoins++;
			player.score += 300;
		}
	}

	// ---------- 当たり判定:プレイヤー × 回復アイテム ----------
	for (int i = 0; i < kMaxHeals; i++) {
		Coin& c = heals[i];
		if (!c.isActive) continue;
		if (CircleRectHit(c.x, c.y, c.radius, player.x, player.y, player.size, player.size)) {
			c.isActive = false;
			if (player.hp < kPlayerMaxHp) {
				player.hp++; // HPが1回復(最大HPまで)
			}
		}
	}

	// ---------- ゲームオーバー判定 ----------
	// 画面外(下)に落下したら即死
	if (player.y > kScreenH) {
		player.hp = 0;
	}
	if (player.hp <= 0) {
		player.hp = 0;
		isGameOver = true;
		return;
	}

	// ---------- ゴール判定 ----------
	if (goal.isActive && RectRectHit(player.x, player.y, player.size, player.size, goal.x, 0.0f, static_cast<float>(kGoalWidth), static_cast<float>(kScreenH))) {
		isStageClear = true;
		clearBonus = player.hp * kClearHpBonus; // 残りHPが多いほどボーナス
		player.score += clearBonus;
		stageCleared[currentStage] = true;
		if (player.bigCoins > bigCoinRecord[currentStage]) {
			bigCoinRecord[currentStage] = player.bigCoins;
		}
		if (player.score > bestScore[currentStage]) {
			bestScore[currentStage] = player.score;
		}
	}
}

// ============================================================
// 描画処理:タイトル画面
// ============================================================
void DrawTitle() {
	// 背景の飾り(ふわふわ浮かぶ自機とコインと敵)
	float bob = std::sin(titleFrame * 0.05f) * 20.0f;
	DrawSpriteFit(sprPlayer, 300, static_cast<int>(430.0f + bob), 40, 40, GREEN);
	for (int i = 0; i < 3; i++) {
		DrawSpriteCircle(sprCoin, static_cast<float>(420 + i * 40), 450.0f + bob, 12.0f, kColorYellow);
	}
	DrawSpriteCircle(sprEnemyShot, 780.0f, 450.0f - bob, 8.0f, kColorOrange);
	DrawSpriteFit(sprEnemyA, 900, static_cast<int>(430.0f - bob), 40, 40, RED);

	// タイトル
	Novice::DrawBox(440, 180, 400, 100, 0.0f, kColorBlue, kFillModeSolid);
	Novice::DrawBox(440, 180, 400, 100, 0.0f, WHITE, kFillModeWireFrame);
	GetFont().Printf(580, 225, "SKY FLOAT SHOOTER");

	// 点滅する案内
	if ((titleFrame / 30) % 2 == 0) {
		GetFont().Printf(570, 330, "PRESS ENTER TO START");
	}

	// 操作説明
	GetFont().Printf(470, 560, "SPACE : Float up / Release = Shot / Hold = Charge Shot");
	GetFont().Printf(470, 585, "Get Coins, Reach the GOAL!  Falling off screen = Dead");
	GetFont().Printf(470, 610, "ESC : Quit    F1 : Debug (hitbox)");
}

// ============================================================
// 描画処理:ステージ選択
// ============================================================
void DrawStageSelect() {
	GetFont().Printf(565, 110, "- STAGE SELECT -");

	const int kBoxH = 220;
	const int kBoxY = 240;
	int spacing = kScreenW / kStageCount;
	int boxW = spacing - 20; // ステージ数が多いほど、カードを細くする
	if (boxW > 300) boxW = 300;

	for (int i = 0; i < kStageCount; i++) {
		int cx = spacing * i + spacing / 2;
		int x = cx - boxW / 2;
		bool isSelected = (i == selectedStage);

		Novice::DrawBox(x, kBoxY, boxW, kBoxH, 0.0f, isSelected ? kColorBlue : kColorDarkGray, kFillModeSolid);
		Novice::DrawBox(x, kBoxY, boxW, kBoxH, 0.0f, isSelected ? WHITE : kColorGray, kFillModeWireFrame);

		GetFont().Printf(x + 20, kBoxY + 25, "STAGE %d", i + 1);
		GetFont().Printf(x + 20, kBoxY + 55, "%s", kStages[i].name);

		if (stageCleared[i]) {
			GetFont().Printf(x + 20, kBoxY + 140, "CLEARED!");
			GetFont().Printf(x + 20, kBoxY + 165, "BEST SCORE: %d", bestScore[i]);
		}
		else {
			GetFont().Printf(x + 20, kBoxY + 140, "NOT CLEARED");
		}

		if (CountSpawnKind(i, kSpawnBoss) > 0) {
			GetFont().Printf(x + 20, kBoxY + 85, "BOSS STAGE!");
		}
		GetFont().Printf(x + 20, kBoxY + 190, "BIG COIN: %d / %d", bigCoinRecord[i], CountSpawnKind(i, kSpawnBigCoin));

		// カーソル
		if (isSelected) {
			GetFont().Printf(cx - 4, kBoxY - 30, "V");
		}
	}

	GetFont().Printf(430, 560, "LEFT / RIGHT : Select    ENTER : Start    BACKSPACE : Title");
}

// ============================================================
// 描画処理:デバッグ用の当たり判定(debug が true のときだけ表示)
// ============================================================
void DrawHitboxes() {
	if (!debug) return;

	// 障害物(種類ごとに色を変える)
	for (int i = 0; i < kMaxObstacles; i++) {
		const Obstacle& o = obstacles[i];
		if (!o.isActive) continue;
		unsigned int color = kDebugBlockNormal;
		if (o.type == kObstacleChargeOnly) color = kDebugBlockCharge;
		if (o.type == kObstacleSolid) color = kDebugBlockSolid;
		// タイルごとに表示(境目が見えるよう、高さを1px小さく描く)
		for (int t = 0; t < o.tileCount; t++) {
			if (!o.tileAlive[t]) continue;
			float top = TileEdgeY(o, t);
			float height = TileEdgeY(o, t + 1) - top;
			DebugBox(o.x, top, o.w, height - 1.0f, color);
		}
	}

	// ゴール
	if (goal.isActive) {
		DebugBox(goal.x, 0.0f, static_cast<float>(kGoalWidth), static_cast<float>(kScreenH), kDebugGoal);
	}

	// コイン・大コイン・回復アイテム
	for (int i = 0; i < kMaxCoins; i++) {
		if (coins[i].isActive) DebugCircle(coins[i].x, coins[i].y, coins[i].radius, kDebugItem);
	}
	for (int i = 0; i < kMaxBigCoins; i++) {
		if (bigCoins[i].isActive) DebugCircle(bigCoins[i].x, bigCoins[i].y, bigCoins[i].radius, kDebugItem);
	}
	for (int i = 0; i < kMaxHeals; i++) {
		if (heals[i].isActive) DebugCircle(heals[i].x, heals[i].y, heals[i].radius, kDebugItem);
	}

	// 敵・ボス
	for (int i = 0; i < kMaxEnemies; i++) {
		if (enemies[i].isActive) DebugBox(enemies[i].x, enemies[i].y, enemies[i].size, enemies[i].size, kDebugEnemy);
	}
	if (boss.isActive) {
		DebugBox(boss.x, boss.y, kBossW, kBossH, kDebugBoss);
	}

	// 弾
	for (int i = 0; i < kMaxBullets; i++) {
		if (enemyBullets[i].isActive) DebugCircle(enemyBullets[i].x, enemyBullets[i].y, enemyBullets[i].radius, kDebugEnemyBullet);
		if (playerBullets[i].isActive) DebugCircle(playerBullets[i].x, playerBullets[i].y, playerBullets[i].radius, kDebugPlayerBullet);
	}

	// 自機
	if (!isGameOver) {
		DebugBox(player.x, player.y, player.size, player.size, kDebugPlayer);
	}
}

// ============================================================
// 描画処理:プレイ中(ゲームオーバー・クリア表示も含む)
// ============================================================
void DrawPlay() {

	// ---------- 障害物(残っているタイルを1つずつ描く) ----------
	for (int i = 0; i < kMaxObstacles; i++) {
		const Obstacle& o = obstacles[i];
		if (!o.isActive) continue;

		const Sprite* spr = &sprBlockB; // 通常弾で壊せる
		unsigned int fallback = kColorBrown;
		if (o.type == kObstacleChargeOnly) {
			spr = &sprBlockC; // チャージ弾でのみ壊せる
			fallback = kColorPurple;
		}
		else if (o.type == kObstacleSolid) {
			spr = &sprBlockA; // 壊せない
			fallback = kColorGray;
		}

		for (int t = 0; t < o.tileCount; t++) {
			if (!o.tileAlive[t]) continue; // 壊れたタイルは描かない
			// 隣のタイルと同じ式で境目を求めるので、すき間は空かない
			int top = static_cast<int>(TileEdgeY(o, t));
			int bottom = static_cast<int>(TileEdgeY(o, t + 1));
			DrawSpriteFit(*spr, static_cast<int>(o.x), top, static_cast<int>(o.w), bottom - top, fallback);
		}
	}

	// ---------- ゴール(画像を縦にならべたゲート) ----------
	if (goal.isActive) {
		DrawSpriteColumn(sprGoalTile, static_cast<int>(goal.x), 0, kGoalWidth, kScreenH, kScreenH / kGoalTileH, kColorGray);
		GetFont().Printf(static_cast<int>(goal.x) + 10, 340, "GOAL");
	}

	// ---------- コイン ----------
	for (int i = 0; i < kMaxCoins; i++) {
		const Coin& c = coins[i];
		if (!c.isActive) continue;
		DrawSpriteCircle(sprCoin, c.x, c.y, c.radius, kColorYellow);
	}

	// ---------- 大コイン ----------
	for (int i = 0; i < kMaxBigCoins; i++) {
		const Coin& c = bigCoins[i];
		if (!c.isActive) continue;
		DrawSpriteCircle(sprBigCoin, c.x, c.y, c.radius, kColorGold);
	}

	// ---------- 回復アイテム ----------
	for (int i = 0; i < kMaxHeals; i++) {
		const Coin& c = heals[i];
		if (!c.isActive) continue;
		DrawSpriteCircle(sprHeal, c.x, c.y, c.radius, kColorHeal);
	}

	// ---------- 敵 ----------
	for (int i = 0; i < kMaxEnemies; i++) {
		const Enemy& e = enemies[i];
		if (!e.isActive) continue;
		const Sprite& spr = (e.type == kEnemyShooter) ? sprEnemyA : sprEnemyB;
		unsigned int fallback = (e.type == kEnemyShooter) ? RED : kColorPink;
		DrawSpriteFit(spr, static_cast<int>(e.x), static_cast<int>(e.y), static_cast<int>(e.size), static_cast<int>(e.size), fallback);
		// 雑魚敵のHPバー
		DrawHpBar(static_cast<int>(e.x), static_cast<int>(e.y) - 10, static_cast<int>(e.size), 5, e.hp, e.maxHp, kColorHpBar);
	}

	// ---------- ボス(ダメージを受けた直後は専用の画像) ----------
	if (boss.isActive) {
		const Sprite& spr = (boss.hitFlash > 0) ? sprBoss2 : sprBoss1;
		unsigned int fallback = (boss.hitFlash > 0) ? WHITE : kColorDarkRed;
		DrawSpriteFit(spr, static_cast<int>(boss.x), static_cast<int>(boss.y), static_cast<int>(kBossW), static_cast<int>(kBossH), fallback);
	}

	// ---------- 敵の弾 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		const Bullet& b = enemyBullets[i];
		if (!b.isActive) continue;
		DrawSpriteCircle(sprEnemyShot, b.x, b.y, b.radius, kColorOrange);
	}

	// ---------- プレイヤーの弾 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		const Bullet& b = playerBullets[i];
		if (!b.isActive) continue;
		const Sprite& spr = b.isCharged ? sprChargeShot : sprNormalShot;
		DrawSpriteCircle(spr, b.x, b.y, b.radius, b.isCharged ? kColorCyan : WHITE);
	}

	// ---------- プレイヤー(無敵中は点滅) ----------
	if (!isGameOver && player.y <= kScreenH) {
		bool visible = (player.invincible == 0) || ((player.invincible / 5) % 2 == 0);
		if (visible) {
			DrawSpriteFit(sprPlayer, static_cast<int>(player.x), static_cast<int>(player.y), static_cast<int>(player.size), static_cast<int>(player.size), GREEN);
		}

		// チャージ中のエフェクト(チャージ弾の画像が大きくなる)とゲージ
		if (player.charge > 5) {
			float rate = static_cast<float>(player.charge) / kChargeNeed;
			bool full = player.charge >= kChargeNeed;
			float r = 4.0f + 16.0f * rate;
			// 溜まりきるまでは半透明、溜まったら不透明
			DrawSpriteCircle(sprChargeShot, player.x + player.size + 8.0f, player.y + player.size / 2.0f, r, full ? kColorCyan : kColorGray, full ? WHITE : 0xFFFFFF99);

			int gaugeW = static_cast<int>(player.size * rate);
			Novice::DrawBox(static_cast<int>(player.x), static_cast<int>(player.y) - 10, static_cast<int>(player.size), 5, 0.0f, kColorGray, kFillModeWireFrame);
			Novice::DrawBox(static_cast<int>(player.x), static_cast<int>(player.y) - 10, gaugeW, 5, 0.0f, full ? kColorCyan : WHITE, kFillModeSolid);
		}
	}

	// ---------- デバッグ:当たり判定(半透明) ----------
	DrawHitboxes();

	// ---------- UI ----------
	// HP(ハートの画像。残っているものと、なくなったもの)
	for (int i = 0; i < kPlayerMaxHp; i++) {
		if (i < player.hp) {
			DrawSpriteFit(sprHeart1, 20 + i * 30, 20, 24, 24, RED);
		}
		else {
			DrawSpriteFit(sprHeart2, 20 + i * 30, 20, 24, 24, kColorGray);
		}
	}
	GetFont().Printf(20, 55, "SCORE: %d", player.score);
	GetFont().Printf(20, 75, "COIN : %d", player.coins);
	GetFont().Printf(150, 75, "BIG COIN: %d / %d", player.bigCoins, CountSpawnKind(currentStage, kSpawnBigCoin));
	GetFont().Printf(20, 100, "SPACE: Float / Release = Shot / Hold = Charge Shot");
	GetFont().Printf(20, 120, "Block: Normal = Shot OK / Hard = Charge Only / Solid = Unbreakable");
	GetFont().Printf(20, 140, "Big Coin = Bonus / Heal Item = +1 HP");
	if (debug) {
		GetFont().Printf(20, 160, "DEBUG ON (F1): hitbox");
	}
	GetFont().Printf(1000, 20, "STAGE %d : %s", currentStage + 1, kStages[currentStage].name);

	// ボスのHPバー(画面上部)
	if (boss.isActive) {
		GetFont().Printf(340, 12, "BOSS   HP: %d / %d", boss.hp, boss.maxHp);
		DrawHpBar(340, 32, 600, 18, boss.hp, boss.maxHp, RED);
	}

	// ---------- ゲームオーバー表示 ----------
	if (isGameOver) {
		Novice::DrawBox(400, 240, 480, 230, 0.0f, kColorPanel, kFillModeSolid);
		Novice::DrawBox(400, 240, 480, 230, 0.0f, WHITE, kFillModeWireFrame);
		GetFont().Printf(580, 275, "GAME OVER");
		GetFont().Printf(520, 320, "SCORE: %d    COIN: %d", player.score, player.coins);
		GetFont().Printf(540, 390, "R     : Retry");
		GetFont().Printf(540, 415, "ENTER : Stage Select");
	}

	// ---------- ステージクリア表示 ----------
	if (isStageClear) {
		Novice::DrawBox(400, 240, 480, 260, 0.0f, kColorPanel, kFillModeSolid);
		Novice::DrawBox(400, 240, 480, 260, 0.0f, kColorYellow, kFillModeWireFrame);
		GetFont().Printf(570, 275, "STAGE CLEAR!");
		GetFont().Printf(520, 320, "COIN: %d    HP BONUS: +%d", player.coins, clearBonus);
		GetFont().Printf(520, 345, "SCORE: %d    BEST: %d", player.score, bestScore[currentStage]);
		GetFont().Printf(520, 370, "BIG COIN: %d / %d", player.bigCoins, CountSpawnKind(currentStage, kSpawnBigCoin));
		GetFont().Printf(540, 420, "R     : Retry");
		GetFont().Printf(540, 445, "ENTER : Stage Select");
	}
}

// ============================================================
// Windowsアプリでのエントリーポイント(main関数)
// ============================================================
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, kScreenW, kScreenH);

	GetFont().Create(L"PixelMplus12-Bold.ttf", L"PixelMplus12", 16);

	// 画像の読み込み
	LoadSprites();

	// キー入力結果を受け取る箱
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	// ステージごとの記録を初期化
	for (int i = 0; i < kStageCount; i++) {
		stageCleared[i] = false;
		bestScore[i] = 0;
		bigCoinRecord[i] = 0;
	}
	ResetGame(0);

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///

		// F1キーでデバッグ表示(当たり判定)の ON / OFF
		if (Triggered(keys, preKeys, DIK_F1)) {
			debug = !debug;
		}

		switch (scene) {
		case kSceneTitle:
			UpdateTitle(keys, preKeys);
			break;
		case kSceneStageSelect:
			UpdateStageSelect(keys, preKeys);
			break;
		case kScenePlay:
			UpdatePlay(keys, preKeys);
			break;
		}

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		// 背景(どの画面でも最背面に描く)
		DrawBackground();

		switch (scene) {
		case kSceneTitle:
			DrawTitle();
			break;
		case kSceneStageSelect:
			DrawStageSelect();
			break;
		case kScenePlay:
			DrawPlay();
			break;
		}

		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();
	return 0;
}