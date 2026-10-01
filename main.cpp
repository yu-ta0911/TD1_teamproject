#include <Novice.h>
#include <cmath>
#include <cstring>

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
const int kChargeNeed = 45;       // チャージ弾に必要なフレーム数

// 弾
const int kMaxBullets = 64;
const float kScrollSpeed = 4.0f; // 障害物・コイン・ゴールが左へ流れる速度

// ゴール
const int kGoalWidth = 60;       // ゴールの幅
const int kClearHpBonus = 100;   // クリア時、残りHP1つにつくボーナス

// ボス
const float kBossW = 120.0f;
const float kBossH = 120.0f;
const float kBossStopX = 900.0f; // ボスが止まって戦うX座標

// 配列サイズ
const int kMaxObstacles = 32;
const int kMaxEnemies = 12;
const int kMaxCoins = 32;
const int kMaxBigCoins = 8;
const int kMaxHeals = 8;

// 色
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
	int type; // 0: 通常弾で壊せる / 1: チャージ弾でのみ壊せる
	bool isActive;
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
	int hitFlash;    // 被弾したときの白点滅フレーム
	bool isArrived;  // 戦う位置に到着したか
	bool isActive;
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
const int kObstacleNormal = 0;     // 通常弾で壊せる
const int kObstacleChargeOnly = 1; // チャージ弾でしか壊せない

// 敵の種類
const int kEnemyShooter = 0; // 上下に揺れながら、自機を狙って弾を撃つ
const int kEnemyStatic = 1;  // 揺れない・弾も撃たない

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

	MakeEnemy(1040, 200.0f, kEnemyStatic, 3),
	MakeEnemy(1040, 500.0f, kEnemyShooter, 3),
	MakeObstacle(1100, 250.0f, 40.0f, 220.0f, kObstacleNormal),
	MakeCoin(1180, 450.0f, 3),

	MakeEnemy(1260, 120.0f, kEnemyShooter, 3),
	MakeEnemy(1260, 360.0f, kEnemyShooter, 3),
	MakeEnemy(1260, 600.0f, kEnemyShooter, 3),

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

	MakeEnemy(1260, 360.0f, kEnemyStatic, 5),
	MakeObstacle(1340, 0.0f, 40.0f, 340.0f, kObstacleChargeOnly),
	MakeObstacle(1340, 420.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	MakeCoin(1420, 380.0f, 3),

	MakeHeal(1480, 360.0f), // ボス前の回復アイテム

	MakeBoss(1600, 40), // ボス登場(HP40)。倒すとゴールが出現します
	MakeGoal(1700),     // ボスが生きている間は、ここまで来ても出現を待ちます
};

// ---------- ステージ一覧(ステージ選択画面に並ぶ順) ----------
// ステージを増やすときは、上に kStage4Data[] を作って、ここに1行足すだけです。
struct StageInfo {
	const char* name;
	const SpawnData* data;
	int count;
};

const StageInfo kStages[] = {
	{"Sky Walk", kStage1Data, static_cast<int>(sizeof(kStage1Data) / sizeof(kStage1Data[0]))},
	{"Cannon Valley", kStage2Data, static_cast<int>(sizeof(kStage2Data) / sizeof(kStage2Data[0]))},
	{"Wall Rush", kStage3Data, static_cast<int>(sizeof(kStage3Data) / sizeof(kStage3Data[0]))},
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

	// ENTERでステージ選択へ
	if (Triggered(keys, preKeys, DIK_RETURN)) {
		scene = kSceneStageSelect;
	}
}

// ============================================================
// 更新処理:ステージ選択
// ============================================================
void UpdateStageSelect(const char* keys, const char* preKeys) {
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

	// ---------- 当たり判定:プレイヤーの弾 × 障害物 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		Bullet& b = playerBullets[i];
		if (!b.isActive) continue;

		for (int j = 0; j < kMaxObstacles; j++) {
			Obstacle& o = obstacles[j];
			if (!o.isActive) continue;
			if (!CircleRectHit(b.x, b.y, b.radius, o.x, o.y, o.w, o.h)) continue;

			if (b.isCharged) {
				// チャージ弾:どちらの障害物も破壊し、弾は貫通する
				o.isActive = false;
				player.score += 10;
			}
			else {
				// 通常弾:type 0 は破壊、type 1 は弾かれて消える
				if (o.type == kObstacleNormal) {
					o.isActive = false;
					player.score += 10;
				}
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

	// ---------- 当たり判定:プレイヤー × 障害物 ----------
	for (int i = 0; i < kMaxObstacles; i++) {
		Obstacle& o = obstacles[i];
		if (!o.isActive) continue;
		if (RectRectHit(player.x, player.y, player.size, player.size, o.x, o.y, o.w, o.h)) {
			if (player.invincible == 0) {
				DamagePlayer(1);
				o.isActive = false; // ぶつかった障害物は壊れる
			}
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
	Novice::DrawBox(300, static_cast<int>(430.0f + bob), 40, 40, 0.0f, GREEN, kFillModeSolid);
	for (int i = 0; i < 3; i++) {
		Novice::DrawEllipse(420 + i * 40, static_cast<int>(450.0f + bob), static_cast<int>(12.0f), static_cast<int>(12.0f), 0.0f, kColorYellow, kFillModeSolid);
	}
	Novice::DrawEllipse(780, static_cast<int>(450.0f - bob), static_cast<int>(8.0f), static_cast<int>(8.0f), 0.0f, kColorOrange, kFillModeSolid);
	Novice::DrawBox(900, static_cast<int>(430.0f - bob), 40, 40, 0.0f, RED, kFillModeSolid);

	// タイトル
	Novice::DrawBox(440, 180, 400, 100, 0.0f, kColorBlue, kFillModeSolid);
	Novice::DrawBox(440, 180, 400, 100, 0.0f, WHITE, kFillModeWireFrame);
	Novice::ScreenPrintf(580, 225, "SKY FLOAT SHOOTER");

	// 点滅する案内
	if ((titleFrame / 30) % 2 == 0) {
		Novice::ScreenPrintf(570, 330, "PRESS ENTER TO START");
	}

	// 操作説明
	Novice::ScreenPrintf(470, 560, "SPACE : Float up / Release = Shot / Hold = Charge Shot");
	Novice::ScreenPrintf(470, 585, "Get Coins, Reach the GOAL!  Falling off screen = Dead");
	Novice::ScreenPrintf(470, 610, "ESC : Quit");
}

// ============================================================
// 描画処理:ステージ選択
// ============================================================
void DrawStageSelect() {
	Novice::ScreenPrintf(565, 110, "- STAGE SELECT -");

	const int kBoxW = 300;
	const int kBoxH = 220;
	const int kBoxY = 240;
	int spacing = kScreenW / kStageCount;

	for (int i = 0; i < kStageCount; i++) {
		int cx = spacing * i + spacing / 2;
		int x = cx - kBoxW / 2;
		bool isSelected = (i == selectedStage);

		Novice::DrawBox(x, kBoxY, kBoxW, kBoxH, 0.0f, isSelected ? kColorBlue : kColorDarkGray, kFillModeSolid);
		Novice::DrawBox(x, kBoxY, kBoxW, kBoxH, 0.0f, isSelected ? WHITE : kColorGray, kFillModeWireFrame);

		Novice::ScreenPrintf(x + 20, kBoxY + 25, "STAGE %d", i + 1);
		Novice::ScreenPrintf(x + 20, kBoxY + 55, "%s", kStages[i].name);

		if (stageCleared[i]) {
			Novice::ScreenPrintf(x + 20, kBoxY + 140, "CLEARED!");
			Novice::ScreenPrintf(x + 20, kBoxY + 165, "BEST SCORE: %d", bestScore[i]);
		}
		else {
			Novice::ScreenPrintf(x + 20, kBoxY + 140, "NOT CLEARED");
		}

		if (CountSpawnKind(i, kSpawnBoss) > 0) {
			Novice::ScreenPrintf(x + 20, kBoxY + 85, "BOSS STAGE!");
		}
		Novice::ScreenPrintf(x + 20, kBoxY + 190, "BIG COIN: %d / %d", bigCoinRecord[i], CountSpawnKind(i, kSpawnBigCoin));

		// カーソル
		if (isSelected) {
			Novice::ScreenPrintf(cx - 4, kBoxY - 30, "V");
		}
	}

	Novice::ScreenPrintf(430, 560, "LEFT / RIGHT : Select    ENTER : Start    BACKSPACE : Title");
}

// ============================================================
// 描画処理:プレイ中(ゲームオーバー・クリア表示も含む)
// ============================================================
void DrawPlay() {

	// ---------- 障害物 ----------
	for (int i = 0; i < kMaxObstacles; i++) {
		const Obstacle& o = obstacles[i];
		if (!o.isActive) continue;
		unsigned int color = (o.type == kObstacleNormal) ? kColorBrown : kColorPurple;
		Novice::DrawBox(static_cast<int>(o.x), static_cast<int>(o.y), static_cast<int>(o.w), static_cast<int>(o.h), 0.0f, color, kFillModeSolid);
		// チャージ専用は枠線で目立たせる
		if (o.type == kObstacleChargeOnly) {
			Novice::DrawBox(static_cast<int>(o.x), static_cast<int>(o.y), static_cast<int>(o.w), static_cast<int>(o.h), 0.0f, WHITE, kFillModeWireFrame);
		}
	}

	// ---------- ゴール(チェッカー模様のゲート) ----------
	if (goal.isActive) {
		const int cell = 30;
		for (int row = 0; row * cell < kScreenH; row++) {
			for (int col = 0; col < kGoalWidth / cell; col++) {
				unsigned int color = ((row + col) % 2 == 0) ? WHITE : BLACK;
				Novice::DrawBox(static_cast<int>(goal.x) + col * cell, row * cell, cell, cell, 0.0f, color, kFillModeSolid);
			}
		}
		Novice::ScreenPrintf(static_cast<int>(goal.x) + 10, 340, "GOAL");
	}

	// ---------- コイン ----------
	for (int i = 0; i < kMaxCoins; i++) {
		const Coin& c = coins[i];
		if (!c.isActive) continue;
		Novice::DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y), static_cast<int>(c.radius), static_cast<int>(c.radius), 0.0f, kColorYellow, kFillModeSolid);
	}

	// ---------- 大コイン ----------
	for (int i = 0; i < kMaxBigCoins; i++) {
		const Coin& c = bigCoins[i];
		if (!c.isActive) continue;
		int cx = static_cast<int>(c.x);
		int cy = static_cast<int>(c.y);
		Novice::DrawEllipse(cx, cy, static_cast<int>(c.radius), static_cast<int>(c.radius), 0.0f, kColorOrange, kFillModeSolid);
		Novice::DrawEllipse(cx, cy, static_cast<int>(c.radius - 6.0f), static_cast<int>(c.radius - 6.0f), 0.0f, kColorGold, kFillModeSolid);
		Novice::DrawEllipse(cx - 6, cy - 6, static_cast<int>(4.0f), static_cast<int>(4.0f), 0.0f, WHITE, kFillModeSolid); // つや
	}

	// ---------- 回復アイテム(緑の丸に白い十字) ----------
	for (int i = 0; i < kMaxHeals; i++) {
		const Coin& c = heals[i];
		if (!c.isActive) continue;
		int cx = static_cast<int>(c.x);
		int cy = static_cast<int>(c.y);
		Novice::DrawEllipse(cx, cy, static_cast<int>(c.radius), static_cast<int>(c.radius), 0.0f, kColorHeal, kFillModeSolid);
		Novice::DrawBox(cx - 8, cy - 2, 16, 4, 0.0f, WHITE, kFillModeSolid);
		Novice::DrawBox(cx - 2, cy - 8, 4, 16, 0.0f, WHITE, kFillModeSolid);
	}

	// ---------- 敵 ----------
	for (int i = 0; i < kMaxEnemies; i++) {
		const Enemy& e = enemies[i];
		if (!e.isActive) continue;
		unsigned int color = (e.type == kEnemyShooter) ? RED : kColorPink;
		Novice::DrawBox(static_cast<int>(e.x), static_cast<int>(e.y), static_cast<int>(e.size), static_cast<int>(e.size), 0.0f, color, kFillModeSolid);
		// 雑魚敵のHPバー
		DrawHpBar(static_cast<int>(e.x), static_cast<int>(e.y) - 10, static_cast<int>(e.size), 5, e.hp, e.maxHp, kColorHpBar);
	}

	// ---------- ボス ----------
	if (boss.isActive) {
		int bx = static_cast<int>(boss.x);
		int by = static_cast<int>(boss.y);
		int bw = static_cast<int>(kBossW);
		int bh = static_cast<int>(kBossH);
		unsigned int color = (boss.hitFlash > 0) ? WHITE : kColorDarkRed; // 被弾すると白く光る
		Novice::DrawBox(bx, by, bw, bh, 0.0f, color, kFillModeSolid);
		Novice::DrawBox(bx, by, bw, bh, 0.0f, WHITE, kFillModeWireFrame);
		// 目(自機のほうを向いている)
		Novice::DrawBox(bx + 20, by + 30, 24, 24, 0.0f, kColorYellow, kFillModeSolid);
		Novice::DrawBox(bx + 20, by + 70, 24, 24, 0.0f, kColorYellow, kFillModeSolid);
	}

	// ---------- 敵の弾 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		const Bullet& b = enemyBullets[i];
		if (!b.isActive) continue;
		Novice::DrawEllipse(static_cast<int>(b.x), static_cast<int>(b.y), static_cast<int>(b.radius), static_cast<int>(b.radius), 0.0f, kColorOrange, kFillModeSolid);
	}

	// ---------- プレイヤーの弾 ----------
	for (int i = 0; i < kMaxBullets; i++) {
		const Bullet& b = playerBullets[i];
		if (!b.isActive) continue;
		unsigned int color = b.isCharged ? kColorCyan : WHITE;
		Novice::DrawEllipse(static_cast<int>(b.x), static_cast<int>(b.y), static_cast<int>(b.radius), static_cast<int>(b.radius), 0.0f, color, kFillModeSolid);
	}

	// ---------- プレイヤー(無敵中は点滅) ----------
	if (!isGameOver && player.y <= kScreenH) {
		bool visible = (player.invincible == 0) || ((player.invincible / 5) % 2 == 0);
		if (visible) {
			Novice::DrawBox(static_cast<int>(player.x), static_cast<int>(player.y), static_cast<int>(player.size), static_cast<int>(player.size), 0.0f, GREEN, kFillModeSolid);
		}

		// チャージ中のエフェクトとゲージ
		if (player.charge > 5) {
			float rate = static_cast<float>(player.charge) / kChargeNeed;
			bool full = player.charge >= kChargeNeed;
			float r = 4.0f + 16.0f * rate;
			Novice::DrawEllipse(static_cast<int>(player.x + player.size + 8.0f), static_cast<int>(player.y + player.size / 2.0f), static_cast<int>(r), static_cast<int>(r), 0.0f, full ? kColorCyan : kColorGray, kFillModeSolid);

			int gaugeW = static_cast<int>(player.size * rate);
			Novice::DrawBox(static_cast<int>(player.x), static_cast<int>(player.y) - 10, static_cast<int>(player.size), 5, 0.0f, kColorGray, kFillModeWireFrame);
			Novice::DrawBox(static_cast<int>(player.x), static_cast<int>(player.y) - 10, gaugeW, 5, 0.0f, full ? kColorCyan : WHITE, kFillModeSolid);
		}
	}

	// ---------- UI ----------
	// HPバー
	for (int i = 0; i < kPlayerMaxHp; i++) {
		Novice::DrawBox(20 + i * 30, 20, 24, 24, 0.0f, i < player.hp ? RED : kColorGray, i < player.hp ? kFillModeSolid : kFillModeWireFrame);
	}
	Novice::ScreenPrintf(20, 55, "SCORE: %d", player.score);
	Novice::ScreenPrintf(20, 75, "COIN : %d", player.coins);
	Novice::ScreenPrintf(150, 75, "BIG COIN: %d / %d", player.bigCoins, CountSpawnKind(currentStage, kSpawnBigCoin));
	Novice::ScreenPrintf(20, 100, "SPACE: Float / Release = Shot / Hold = Charge Shot");
	Novice::ScreenPrintf(20, 120, "Brown = Normal Shot OK / Purple = Charge Shot Only");
	Novice::ScreenPrintf(20, 140, "Red = Shooter / Pink = Static (no shot)");
	Novice::ScreenPrintf(20, 160, "Gold(big) = Bonus Coin / Green + = Heal");
	Novice::ScreenPrintf(1000, 20, "STAGE %d : %s", currentStage + 1, kStages[currentStage].name);

	// ボスのHPバー(画面上部)
	if (boss.isActive) {
		Novice::ScreenPrintf(340, 12, "BOSS   HP: %d / %d", boss.hp, boss.maxHp);
		DrawHpBar(340, 32, 600, 18, boss.hp, boss.maxHp, RED);
	}

	// ---------- ゲームオーバー表示 ----------
	if (isGameOver) {
		Novice::DrawBox(400, 240, 480, 230, 0.0f, kColorPanel, kFillModeSolid);
		Novice::DrawBox(400, 240, 480, 230, 0.0f, WHITE, kFillModeWireFrame);
		Novice::ScreenPrintf(580, 275, "GAME OVER");
		Novice::ScreenPrintf(520, 320, "SCORE: %d    COIN: %d", player.score, player.coins);
		Novice::ScreenPrintf(540, 390, "R     : Retry");
		Novice::ScreenPrintf(540, 415, "ENTER : Stage Select");
	}

	// ---------- ステージクリア表示 ----------
	if (isStageClear) {
		Novice::DrawBox(400, 240, 480, 260, 0.0f, kColorPanel, kFillModeSolid);
		Novice::DrawBox(400, 240, 480, 260, 0.0f, kColorYellow, kFillModeWireFrame);
		Novice::ScreenPrintf(570, 275, "STAGE CLEAR!");
		Novice::ScreenPrintf(520, 320, "COIN: %d    HP BONUS: +%d", player.coins, clearBonus);
		Novice::ScreenPrintf(520, 345, "SCORE: %d    BEST: %d", player.score, bestScore[currentStage]);
		Novice::ScreenPrintf(520, 370, "BIG COIN: %d / %d", player.bigCoins, CountSpawnKind(currentStage, kSpawnBigCoin));
		Novice::ScreenPrintf(540, 420, "R     : Retry");
		Novice::ScreenPrintf(540, 445, "ENTER : Stage Select");
	}
}

// ============================================================
// Windowsアプリでのエントリーポイント(main関数)
// ============================================================
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, kScreenW, kScreenH);

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