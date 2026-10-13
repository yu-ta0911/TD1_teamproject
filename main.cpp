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

// グリッド設定
const float kGridSize = 40.0f;           // 1タイルの大きさ（40px）
const int kGridRows = 18;                // 縦の行数 (720 / 40 = 18)

// 物理・移動仕様
const float kNormalSpeed = 4.0f;     // 通常時の上下往復速度
const float kDashSpeed = 12.0f;      // スペース押下中のダッシュ速度

// プレイヤー
const int kPlayerMaxHp = 5;
const int kInvincibleFrames = 90; // 被弾後の無敵時間

// 弾
const int kMaxBullets = 64;
const float kScrollSpeedBase = 4.0f; // 基本のスクロール速度

// ゴール
const int kGoalWidth = 60;       // ゴールの幅
const int kGoalTileH = 30;       // ゴール画像1枚ぶんの高さ
const int kClearHpBonus = 100;   // クリア時、残りHP1つにつくボーナス

// ボス
const float kBossW = 120.0f;
const float kBossH = 120.0f;
const float kBossStopX = 900.0f; // ボスが止まって戦うX座標

// 背景
const float kBgScrollSpeed = 1.0f; // 背景のスクロール速度

// 配列サイズ（長いステージや多くのブロック・アイテムに余裕で対応）
const int kMaxObstacles = 8192;
const int kMaxEnemies = 128;
const int kMaxCoins = 512;
const int kMaxBigCoins = 32;
const int kMaxHeals = 32;
const int kTileScore = 5;

// 画像フォルダ
const char kImageDir[] = "./images/";

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
const unsigned int kColorPanel = 0x000000BBu;
const unsigned int kColorGold = 0xFFD700FF;
const unsigned int kColorHeal = 0x44DD66FF;
const unsigned int kColorDarkRed = 0xAA2222FF;
const unsigned int kColorHpBar = 0x44DD44FF;

// デバッグ表示の色
const unsigned int kDebugPlayer = 0x00FF0066;
const unsigned int kDebugEnemy = 0xFF000066;
const unsigned int kDebugBoss = 0xFF000066;
const unsigned int kDebugBlockNormal = 0xFFAA0066;
const unsigned int kDebugBlockSolid = 0xFFFFFF66;
const unsigned int kDebugItem = 0xFFFF0066;
const unsigned int kDebugEnemyBullet = 0xFF880066;
const unsigned int kDebugGoal = 0x00FF0044;

// 画面(シーン)
enum Scene {
	kSceneTitle,
	kSceneStageSelect,
	kScenePlay,
};

// ============================================================
// 構造体
// ============================================================
struct Player {
	float x, y, size, vy;
	int hp;
	int invincible;
	int score;
	int coins;
	int bigCoins;
};

struct Bullet {
	float x, y;
	float vx, vy;
	float radius;
	bool isActive;
};

struct Obstacle {
	float x, y, w, h;
	int type; // 0: ダッシュで壊せる / 1: 壊せない
	bool isActive;
};

struct Enemy {
	float x, baseY, y;
	float size;
	int hp;
	int maxHp;
	int shotTimer;
	int frame;
	int type;
	bool isActive;
};

struct Coin {
	float x, y, radius;
	bool isActive;
};

struct Goal {
	float x;
	bool isActive;
};

struct Boss {
	float x, y;
	float baseY;
	float wave;
	int hp, maxHp;
	int frame;
	int aimTimer;
	int fanTimer;
	int hitFlash;
	bool isArrived;
	bool isActive;
};

struct Sprite {
	int handle = -1;
	int w = 0;
	int h = 0;
	bool IsValid() const { return handle >= 0 && w > 0 && h > 0; }
};

// ============================================================
// ステージデータ(文字によるアスキーアートマップエディター方式)
// ============================================================
const char* kStage1MapLines[] = {
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	".......................................................X............................................................................................................................................................................................................................................................................................................",
	".......................................................X....................................................................................................................................CCCCCCCCCC..............................................................................................................................................................",
	".......................................................X................................................................................................................C..................C..........C.............................................................................................................................................................",
	".......................................................X..............................................CCCCC..............................................................C................C............C............................................................................................................................................................",
	".......................................................X..................................................................................................................C..............C..............C...........................................................................................................................................................",
	".......................................................X..........................................................CCCCC....................................................C............C................C..........................................................................................................................................................",
	".......................................................X....................................................................................................................C..........C..................C.........................................................................................................................................................",
	".......................................................X.......................................................................CCCCC.........................................CCCCCCCCCC....................C........................................................................................................................................................",
	".......................................................X....................................................................................................................................................C.......................................................................................................................................................",
	".......................................................X.....................................................................................................................................................C...........C..........................................................................................................................................",
	".......................................................X......................................................................................................................................................C.........C...........................................................................................................................................",
	".......................................................X.......................................................................................................................................................C.......C............................................................................................................................................",
	".......................................................X........................................................................................................................................................C.....C.............................................................................................................................................",
	".......................................................X.........................................................................................................................................................CCCCC..............................................................................................................................................",
	".......................................................X............................................................................................................................................................................................................................................................................................................",
	".......................................................X............................................................................................................................................................................................................................................................................................................",
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXG"
};

const char* kStage2MapLines[] = {
	".........................................................................................",
	".........................................................................................",
	"......B..................................................................................",
	"....#####...................E.......................E....................................",
	"....#...#................................................................................",
	"....#####................................................................................",
	".........................................................................................",
	".........................................................................................",
	"..................................................HHHH...................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".....................XXXXXXXXX...........................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".......................................................................................G."
};

const char* kStage3MapLines[] = {
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".......##########........................................................................",
	".......#........#........................................................................",
	".......#........#........................................................................",
	".......##########........................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".....................................................................................M.G."
};

const char* kStage4MapLines[] = {
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".......................................................................................G."
};

const char* kStage5MapLines[] = {
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".........................................................................................",
	".....................................................................................M.G."
};

struct StageInfo {
	const char* name;
	const char** map;
	int cols;
};

const StageInfo kStages[] = {
	{"Sky Walk", kStage1MapLines, static_cast<int>(strlen(kStage1MapLines[0]))},
	{"Cannon Valley", kStage2MapLines, static_cast<int>(strlen(kStage2MapLines[0]))},
	{"Wall Rush", kStage3MapLines, static_cast<int>(strlen(kStage3MapLines[0]))},
	{"Fortress Run", kStage4MapLines, static_cast<int>(strlen(kStage4MapLines[0]))},
	{"Final Skies", kStage5MapLines, static_cast<int>(strlen(kStage5MapLines[0]))},
};
const int kStageCount = static_cast<int>(sizeof(kStages) / sizeof(kStages[0]));

// ============================================================
// ゲームデータ
// ============================================================
Player player;
float playerDirY = 1.0f;
bool isDashing = false;
bool prevSpace = false;
Bullet enemyBullets[kMaxBullets];
Obstacle obstacles[kMaxObstacles];
Enemy enemies[kMaxEnemies];
Coin coins[kMaxCoins];
Goal goal;
Boss boss;
Coin bigCoins[kMaxBigCoins];
Coin heals[kMaxHeals];

int stageFrame;

Scene scene = kSceneTitle;
int titleFrame = 0;
int selectedStage = 0;
int currentStage = 0;
bool isGameOver = false;
bool isStageClear = false;
int clearBonus = 0;

bool stageCleared[kStageCount];
int bestScore[kStageCount];
int bigCoinRecord[kStageCount];

bool debug = true;
float bgOffset = 0.0f;

// ============================================================
// 画像(スプライト)
// ============================================================
Sprite sprBg;
Sprite sprPlayer;
Sprite sprEnemyA;
Sprite sprEnemyB;
Sprite sprBoss1;
Sprite sprBoss2;
Sprite sprEnemyShot;
Sprite sprBlockA;
Sprite sprBlockB;
Sprite sprGoalTile;
Sprite sprCoin;
Sprite sprBigCoin;
Sprite sprHeal;
Sprite sprHeart1;
Sprite sprHeart2;

bool ReadPngSize(const char* path, int* outW, int* outH) {
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) return false;

	unsigned char header[24] = {};
	ifs.read(reinterpret_cast<char*>(header), sizeof(header));
	if (ifs.gcount() < 24) return false;

	if (header[0] != 0x89 || header[1] != 'P' || header[2] != 'N' || header[3] != 'G') return false;

	*outW = (header[16] << 24) | (header[17] << 16) | (header[18] << 8) | header[19];
	*outH = (header[20] << 24) | (header[21] << 16) | (header[22] << 8) | header[23];
	return true;
}

Sprite LoadSprite(const char* fileName) {
	Sprite s;
	std::string path = std::string(kImageDir) + fileName;
	if (!ReadPngSize(path.c_str(), &s.w, &s.h)) {
		return s;
	}
	s.handle = Novice::LoadTexture(path.c_str());
	return s;
}

void LoadSprites() {
	sprBg = LoadSprite("td1-1_bg_1.png");
	sprPlayer = LoadSprite("td1-1_player1.png");
	sprEnemyA = LoadSprite("td1-1_enemyA.png");
	sprEnemyB = LoadSprite("td1-1_enemyB.png");
	sprBoss1 = LoadSprite("td1-1_boss1.png");
	sprBoss2 = LoadSprite("td1-1_boss2.png");
	sprEnemyShot = LoadSprite("td1-1_enemy_shot.png");
	sprBlockA = LoadSprite("td1-1_blockA.png");
	sprBlockB = LoadSprite("td1-1_blockB.png");
	sprGoalTile = LoadSprite("td1-1_goal_tile.png");
	sprCoin = LoadSprite("td1-1_coin.png");
	sprBigCoin = LoadSprite("td1-1_big_coin.png");
	sprHeal = LoadSprite("td1-1_heal.png");
	sprHeart1 = LoadSprite("td1-1_ui_heart1.png");
	sprHeart2 = LoadSprite("td1-1_ui_heart2.png");
}

void DrawSpriteFit(const Sprite& s, int x, int y, int w, int h, unsigned int fallbackColor, unsigned int color = WHITE) {
	if (!s.IsValid()) {
		Novice::DrawBox(x, y, w, h, 0.0f, fallbackColor, kFillModeSolid);
		return;
	}
	Novice::DrawSprite(x, y, s.handle, static_cast<float>(w) / s.w, static_cast<float>(h) / s.h, 0.0f, color);
}

void DrawSpriteCircle(const Sprite& s, float cx, float cy, float r, unsigned int fallbackColor, unsigned int color = WHITE) {
	if (!s.IsValid()) {
		Novice::DrawEllipse(static_cast<int>(cx), static_cast<int>(cy), static_cast<int>(r), static_cast<int>(r), 0.0f, fallbackColor, kFillModeSolid);
		return;
	}
	int d = static_cast<int>(r * 2.0f);
	DrawSpriteFit(s, static_cast<int>(cx - r), static_cast<int>(cy - r), d, d, fallbackColor, color);
}

void DrawSpriteColumn(const Sprite& s, int x, int y, int w, int h, int count, unsigned int fallbackColor) {
	if (count < 1) count = 1;
	for (int i = 0; i < count; i++) {
		int top = y + h * i / count;
		int bottom = y + h * (i + 1) / count;
		DrawSpriteFit(s, x, top, w, bottom - top, fallbackColor);
	}
}

void DrawBackground() {
	if (!sprBg.IsValid()) return;

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

bool RectRectHit(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
	return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

bool Triggered(const char* keys, const char* preKeys, int key) { return preKeys[key] == 0 && keys[key] != 0; }

int CountMapSymbol(int stage, char symbol) {
	int n = 0;
	const StageInfo& s = kStages[stage];
	for (int r = 0; r < kGridRows; r++) {
		for (int c = 0; c < s.cols; c++) {
			if (s.map[r][c] == symbol) n++;
		}
	}
	return n;
}

void DrawHpBar(int x, int y, int w, int h, int hp, int maxHp, unsigned int color) {
	if (maxHp <= 0) return;
	if (hp < 0) hp = 0;
	Novice::DrawBox(x, y, w, h, 0.0f, kColorDarkGray, kFillModeSolid);
	Novice::DrawBox(x, y, w * hp / maxHp, h, 0.0f, color, kFillModeSolid);
	Novice::DrawBox(x, y, w, h, 0.0f, WHITE, kFillModeWireFrame);
}

void DebugBox(float x, float y, float w, float h, unsigned int color) {
	if (!debug) return;
	Novice::DrawBox(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), 0.0f, color, kFillModeSolid);
}

void DebugCircle(float cx, float cy, float r, unsigned int color) {
	if (!debug) return;
	Novice::DrawEllipse(static_cast<int>(cx), static_cast<int>(cy), static_cast<int>(r), static_cast<int>(r), 0.0f, color, kFillModeSolid);
}

void DamagePlayer(int damage) {
	if (player.invincible > 0) return;
	player.hp -= damage;
	player.invincible = kInvincibleFrames;
}

int FindFreeBullet(Bullet* bullets) {
	for (int i = 0; i < kMaxBullets; i++) {
		if (!bullets[i].isActive) return i;
	}
	return -1;
}

bool IsOnScreenX(float x, float w) {
	return x + w > 0.0f && x < static_cast<float>(kScreenW);
}

// ============================================================
// ステージの全マップを一括で解析して1マスずつ配置する初期化関数
// ============================================================
void LoadMapAll(int stageIdx) {
	for (int i = 0; i < kMaxObstacles; i++) obstacles[i].isActive = false;
	for (int i = 0; i < kMaxEnemies; i++) enemies[i].isActive = false;
	for (int i = 0; i < kMaxCoins; i++) coins[i].isActive = false;
	for (int i = 0; i < kMaxBigCoins; i++) bigCoins[i].isActive = false;
	for (int i = 0; i < kMaxHeals; i++) heals[i].isActive = false;
	goal.isActive = false;
	boss.isActive = false;

	const StageInfo& stage = kStages[stageIdx];

	// マップの各文字をスキャンしてオブジェクトを登録
	for (int r = 0; r < kGridRows; r++) {
		for (int c = 0; c < stage.cols; c++) {
			char symbol = stage.map[r][c];
			float x = static_cast<float>(c) * kGridSize;
			float y = static_cast<float>(r) * kGridSize;

			if (symbol == '#' || symbol == 'X') {
				int type = (symbol == '#') ? 0 : 1;
				for (int i = 0; i < kMaxObstacles; i++) {
					if (!obstacles[i].isActive) {
						obstacles[i].x = x;
						obstacles[i].y = y;
						obstacles[i].w = kGridSize;
						obstacles[i].h = kGridSize;
						obstacles[i].type = type;
						obstacles[i].isActive = true;
						break;
					}
				}
			}
			else if (symbol == 'E') {
				for (int i = 0; i < kMaxEnemies; i++) {
					if (!enemies[i].isActive) {
						enemies[i].x = x;
						enemies[i].baseY = y;
						enemies[i].y = y;
						enemies[i].size = 40.0f;
						enemies[i].hp = 3;
						enemies[i].maxHp = 3;
						enemies[i].shotTimer = 60;
						enemies[i].frame = 0;
						enemies[i].type = 0;
						enemies[i].isActive = true;
						break;
					}
				}
			}
			else if (symbol == 'C') {
				for (int i = 0; i < kMaxCoins; i++) {
					if (!coins[i].isActive) {
						coins[i].x = x + kGridSize * 0.5f;
						coins[i].y = y + kGridSize * 0.5f;
						coins[i].radius = 12.0f;
						coins[i].isActive = true;
						break;
					}
				}
			}
			else if (symbol == 'B') {
				for (int i = 0; i < kMaxBigCoins; i++) {
					if (!bigCoins[i].isActive) {
						bigCoins[i].x = x + kGridSize * 0.5f;
						bigCoins[i].y = y + kGridSize * 0.5f;
						bigCoins[i].radius = 24.0f;
						bigCoins[i].isActive = true;
						break;
					}
				}
			}
			else if (symbol == 'H') {
				for (int i = 0; i < kMaxHeals; i++) {
					if (!heals[i].isActive) {
						heals[i].x = x + kGridSize * 0.5f;
						heals[i].y = y + kGridSize * 0.5f;
						heals[i].radius = 14.0f;
						heals[i].isActive = true;
						break;
					}
				}
			}
			else if (symbol == 'G') {
				if (!goal.isActive) {
					goal.x = x;
					goal.isActive = true;
				}
			}
			else if (symbol == 'M') {
				if (!boss.isActive) {
					boss.x = x;
					boss.baseY = 300.0f;
					boss.y = boss.baseY;
					boss.wave = 0.0f;
					boss.hp = 40;
					boss.maxHp = 40;
					boss.frame = 0;
					boss.aimTimer = 60;
					boss.fanTimer = 100;
					boss.hitFlash = 0;
					boss.isArrived = false;
					boss.isActive = true;
				}
			}
		}
	}
}

// ============================================================
// 初期化・リセット
// ============================================================
void ResetGame(int stage) {
	currentStage = stage;

	player.x = 300.0f;
	player.y = 300.0f;
	player.size = 40.0f;
	player.vy = 0.0f;
	player.hp = kPlayerMaxHp;
	player.invincible = 0;
	player.score = 0;
	player.coins = 0;
	player.bigCoins = 0;
	playerDirY = 1.0f;
	isDashing = false;
	prevSpace = false;

	for (int i = 0; i < kMaxBullets; i++) {
		enemyBullets[i].isActive = false;
	}

	stageFrame = 0;
	isGameOver = false;
	isStageClear = false;
	clearBonus = 0;

	LoadMapAll(currentStage);
}

// ============================================================
// 弾の発射 (敵・ボス用)
// ============================================================
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
	b.isActive = true;
}

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
		b.isActive = true;
	}
}

// ============================================================
// 更新処理:タイトル画面
// ============================================================
void UpdateTitle(const char* keys, const char* preKeys) {
	titleFrame++;
	bgOffset += kBgScrollSpeed;

	if (Triggered(keys, preKeys, DIK_RETURN)) {
		scene = kSceneStageSelect;
	}
}

// ============================================================
// 更新処理:ステージ選択
// ============================================================
void UpdateStageSelect(const char* keys, const char* preKeys) {
	bgOffset += kBgScrollSpeed;

	if (Triggered(keys, preKeys, DIK_LEFT) || Triggered(keys, preKeys, DIK_A)) {
		selectedStage = (selectedStage + kStageCount - 1) % kStageCount;
	}
	if (Triggered(keys, preKeys, DIK_RIGHT) || Triggered(keys, preKeys, DIK_D)) {
		selectedStage = (selectedStage + 1) % kStageCount;
	}

	if (Triggered(keys, preKeys, DIK_RETURN)) {
		ResetGame(selectedStage);
		scene = kScenePlay;
	}

	if (Triggered(keys, preKeys, DIK_BACK)) {
		scene = kSceneTitle;
	}
}

// ============================================================
// 更新処理:プレイ中
// ============================================================
void UpdatePlay(const char* keys, const char* preKeys) {

	if (isGameOver || isStageClear) {
		if (Triggered(keys, preKeys, DIK_R)) {
			ResetGame(currentStage);
		}
		else if (Triggered(keys, preKeys, DIK_RETURN)) {
			selectedStage = currentStage;
			scene = kSceneStageSelect;
		}
		return;
	}

	bool isSpacePressed = (keys[DIK_SPACE] != 0);

	if (!prevSpace && isSpacePressed) {
		isDashing = true;
	}

	float currentScrollSpeed = isDashing ? (kScrollSpeedBase * 2.5f) : kScrollSpeedBase;
	bgOffset += kBgScrollSpeed * (isDashing ? 2.0f : 1.0f);

	float currentSpeed = isDashing ? kDashSpeed : kNormalSpeed;

	if (isDashing) {
		// ダッシュ中は上下移動なし
	}
	else {
		player.y += playerDirY * currentSpeed;

		if (player.y <= 0.0f) {
			player.y = 0.0f;
			playerDirY = 1.0f;
		}
		if (player.y + player.size >= kScreenH) {
			player.y = static_cast<float>(kScreenH) - player.size;
			playerDirY = -1.0f;
		}
	}

	if (prevSpace && !isSpacePressed && isDashing) {
		isDashing = false;
		playerDirY = -playerDirY;
	}

	prevSpace = isSpacePressed;

	if (player.invincible > 0) player.invincible--;
	stageFrame++;

	// ---------- 各種オブジェクトの移動 ----------
	for (int i = 0; i < kMaxObstacles; i++) {
		if (!obstacles[i].isActive) continue;
		obstacles[i].x -= currentScrollSpeed;
	}

	for (int i = 0; i < kMaxCoins; i++) {
		if (!coins[i].isActive) continue;
		coins[i].x -= currentScrollSpeed;
	}

	for (int i = 0; i < kMaxBigCoins; i++) {
		if (!bigCoins[i].isActive) continue;
		bigCoins[i].x -= currentScrollSpeed;
	}
	for (int i = 0; i < kMaxHeals; i++) {
		if (!heals[i].isActive) continue;
		heals[i].x -= currentScrollSpeed;
	}

	if (goal.isActive) {
		goal.x -= currentScrollSpeed;
	}

	for (int i = 0; i < kMaxEnemies; i++) {
		Enemy& e = enemies[i];
		if (!e.isActive) continue;

		e.frame++;
		e.x -= currentScrollSpeed;

		e.y = e.baseY + std::sin(e.frame * 0.05f) * 40.0f;

		e.shotTimer--;
		if (e.shotTimer <= 0) {
			if (e.x < kScreenW - 40 && e.x + e.size > 0) {
				FireEnemyBullet(e);
			}
			e.shotTimer = 90;
		}
	}

	if (boss.isActive) {
		boss.frame++;
		if (boss.hitFlash > 0) boss.hitFlash--;

		if (!boss.isArrived) {
			boss.x -= currentScrollSpeed;
			if (boss.x <= kBossStopX) {
				boss.x = kBossStopX;
				boss.isArrived = true;
			}
		}
		else {
			bool isEnraged = boss.hp * 2 <= boss.maxHp;

			boss.wave += isEnraged ? 0.045f : 0.03f;
			boss.y = boss.baseY + std::sin(boss.wave) * 200.0f;

			boss.aimTimer--;
			if (boss.aimTimer <= 0) {
				FireBossFan(1, 0.0f, 6.0f, 9.0f);
				boss.aimTimer = isEnraged ? 40 : 65;
			}

			boss.fanTimer--;
			if (boss.fanTimer <= 0) {
				FireBossFan(isEnraged ? 5 : 3, 0.3f, 4.5f, 8.0f);
				boss.fanTimer = isEnraged ? 100 : 150;
			}
		}
	}

	for (int i = 0; i < kMaxBullets; i++) {
		Bullet& b = enemyBullets[i];
		if (!b.isActive) continue;
		b.x += b.vx - currentScrollSpeed;
		b.y += b.vy;
		if (b.x < -50 || b.x > kScreenW + 50 || b.y < -50 || b.y > kScreenH + 50) {
			b.isActive = false;
		}
	}

	// ---------- 当たり判定:プレイヤー × 障害物 ----------
	if (player.invincible == 0) {
		bool isHit = false;
		for (int i = 0; i < kMaxObstacles; i++) {
			Obstacle& o = obstacles[i];
			if (!o.isActive) continue;

			if (!RectRectHit(player.x, player.y, player.size, player.size, o.x, o.y, o.w, o.h)) continue;

			if (o.type == 0 && isDashing) {
				o.isActive = false;
				player.score += kTileScore;
			}
			else {
				isHit = true;
			}
		}
		if (isHit) {
			DamagePlayer(1);
		}
	}

	// ---------- 当たり判定:プレイヤー × 敵 ----------
	for (int i = 0; i < kMaxEnemies; i++) {
		Enemy& e = enemies[i];
		if (!e.isActive) continue;
		if (!RectRectHit(player.x, player.y, player.size, player.size, e.x, e.y, e.size, e.size)) continue;

		if (isDashing) {
			e.hp = 0;
			e.isActive = false;
			player.score += 100;
		}
		else {
			DamagePlayer(1);
		}
	}

	// ---------- 当たり判定:プレイヤー × ボス ----------
	if (boss.isActive && RectRectHit(player.x, player.y, player.size, player.size, boss.x, boss.y, kBossW, kBossH)) {
		if (isDashing) {
			boss.hp -= 2;
			boss.hitFlash = 6;
			if (boss.hp <= 0) {
				boss.hp = 0;
				boss.isActive = false;
				player.score += 1000;
				for (int j = 0; j < kMaxBullets; j++) enemyBullets[j].isActive = false;
			}
		}
		else {
			DamagePlayer(1);
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
				player.hp++;
			}
		}
	}

	if (player.hp <= 0) {
		player.hp = 0;
		isGameOver = true;
		return;
	}

	// ---------- ゴール判定 ----------
	if (goal.isActive && RectRectHit(player.x, player.y, player.size, player.size, goal.x, 0.0f, static_cast<float>(kGoalWidth), static_cast<float>(kScreenH))) {
		isStageClear = true;
		clearBonus = player.hp * kClearHpBonus;
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
	float bob = std::sin(titleFrame * 0.05f) * 20.0f;
	DrawSpriteFit(sprPlayer, 300, static_cast<int>(430.0f + bob), 40, 40, GREEN);
	for (int i = 0; i < 3; i++) {
		DrawSpriteCircle(sprCoin, static_cast<float>(420 + i * 40), 450.0f + bob, 12.0f, kColorYellow);
	}
	DrawSpriteCircle(sprEnemyShot, 780.0f, 450.0f - bob, 8.0f, kColorOrange);
	DrawSpriteFit(sprEnemyA, 900, static_cast<int>(430.0f - bob), 40, 40, RED);

	Novice::DrawBox(440, 180, 400, 100, 0.0f, kColorBlue, kFillModeSolid);
	Novice::DrawBox(440, 180, 400, 100, 0.0f, WHITE, kFillModeWireFrame);
	GetFont().Printf(580, 225, "SKY DASH ACTION");

	if ((titleFrame / 30) % 2 == 0) {
		GetFont().Printf(570, 330, "PRESS ENTER TO START");
	}

	GetFont().Printf(470, 560, "SPACE : Dash & Destroy Blocks / Release : Reverse Direction");
	GetFont().Printf(470, 585, "Get Coins, Reach the GOAL!");
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
	int boxW = spacing - 20;
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

		if (CountMapSymbol(i, 'M') > 0) {
			GetFont().Printf(x + 20, kBoxY + 85, "BOSS STAGE!");
		}
		GetFont().Printf(x + 20, kBoxY + 190, "BIG COIN: %d / %d", bigCoinRecord[i], CountMapSymbol(i, 'B'));

		if (isSelected) {
			GetFont().Printf(cx - 4, kBoxY - 30, "V");
		}
	}

	GetFont().Printf(430, 560, "LEFT / RIGHT : Select    ENTER : Start    BACKSPACE : Title");
}

// ============================================================
// 描画処理:デバッグ用の当たり判定
// ============================================================
void DrawHitboxes() {
	if (!debug) return;

	for (int i = 0; i < kMaxObstacles; i++) {
		const Obstacle& o = obstacles[i];
		if (!o.isActive) continue;
		unsigned int color = (o.type == 0) ? kDebugBlockNormal : kDebugBlockSolid;
		DebugBox(o.x, o.y, o.w - 1.0f, o.h - 1.0f, color);
	}

	if (goal.isActive) {
		DebugBox(goal.x, 0.0f, static_cast<float>(kGoalWidth), static_cast<float>(kScreenH), kDebugGoal);
	}

	for (int i = 0; i < kMaxCoins; i++) {
		if (coins[i].isActive) DebugCircle(coins[i].x, coins[i].y, coins[i].radius, kDebugItem);
	}
	for (int i = 0; i < kMaxBigCoins; i++) {
		if (bigCoins[i].isActive) DebugCircle(bigCoins[i].x, bigCoins[i].y, bigCoins[i].radius, kDebugItem);
	}
	for (int i = 0; i < kMaxHeals; i++) {
		if (heals[i].isActive) DebugCircle(heals[i].x, heals[i].y, heals[i].radius, kDebugItem);
	}

	for (int i = 0; i < kMaxEnemies; i++) {
		if (enemies[i].isActive) DebugBox(enemies[i].x, enemies[i].y, enemies[i].size, enemies[i].size, kDebugEnemy);
	}
	if (boss.isActive) {
		DebugBox(boss.x, boss.y, kBossW, kBossH, kDebugBoss);
	}

	for (int i = 0; i < kMaxBullets; i++) {
		if (enemyBullets[i].isActive) DebugCircle(enemyBullets[i].x, enemyBullets[i].y, enemyBullets[i].radius, kDebugEnemyBullet);
	}

	if (!isGameOver) {
		DebugBox(player.x, player.y, player.size, player.size, kDebugPlayer);
	}
}

// ============================================================
// 描画処理:プレイ中
// ============================================================
void DrawPlay() {

	for (int i = 0; i < kMaxObstacles; i++) {
		const Obstacle& o = obstacles[i];
		if (!o.isActive) continue;
		if (!IsOnScreenX(o.x, o.w)) continue;

		const Sprite* spr = (o.type == 0) ? &sprBlockB : &sprBlockA;
		unsigned int fallback = (o.type == 0) ? kColorBrown : kColorGray;
		DrawSpriteFit(*spr, static_cast<int>(o.x), static_cast<int>(o.y), static_cast<int>(o.w), static_cast<int>(o.h), fallback);
	}

	if (goal.isActive) {
		DrawSpriteColumn(sprGoalTile, static_cast<int>(goal.x), 0, kGoalWidth, kScreenH, kScreenH / kGoalTileH, kColorGray);
		GetFont().Printf(static_cast<int>(goal.x) + 10, 340, "GOAL");
	}

	for (int i = 0; i < kMaxCoins; i++) {
		const Coin& c = coins[i];
		if (!c.isActive) continue;
		DrawSpriteCircle(sprCoin, c.x, c.y, c.radius, kColorYellow);
	}

	for (int i = 0; i < kMaxBigCoins; i++) {
		const Coin& c = bigCoins[i];
		if (!c.isActive) continue;
		DrawSpriteCircle(sprBigCoin, c.x, c.y, c.radius, kColorGold);
	}

	for (int i = 0; i < kMaxHeals; i++) {
		const Coin& c = heals[i];
		if (!c.isActive) continue;
		DrawSpriteCircle(sprHeal, c.x, c.y, c.radius, kColorHeal);
	}

	for (int i = 0; i < kMaxEnemies; i++) {
		const Enemy& e = enemies[i];
		if (!e.isActive) continue;
		const Sprite& spr = sprEnemyA;
		unsigned int fallback = RED;
		DrawSpriteFit(spr, static_cast<int>(e.x), static_cast<int>(e.y), static_cast<int>(e.size), static_cast<int>(e.size), fallback);
		DrawHpBar(static_cast<int>(e.x), static_cast<int>(e.y) - 10, static_cast<int>(e.size), 5, e.hp, e.maxHp, kColorHpBar);
	}

	if (boss.isActive) {
		const Sprite& spr = (boss.hitFlash > 0) ? sprBoss2 : sprBoss1;
		unsigned int fallback = (boss.hitFlash > 0) ? WHITE : kColorDarkRed;
		DrawSpriteFit(spr, static_cast<int>(boss.x), static_cast<int>(boss.y), static_cast<int>(kBossW), static_cast<int>(kBossH), fallback);
	}

	for (int i = 0; i < kMaxBullets; i++) {
		const Bullet& b = enemyBullets[i];
		if (!b.isActive) continue;
		DrawSpriteCircle(sprEnemyShot, b.x, b.y, b.radius, kColorOrange);
	}

	if (!isGameOver) {
		bool visible = (player.invincible == 0) || ((player.invincible / 5) % 2 == 0);
		if (visible) {
			DrawSpriteFit(sprPlayer, static_cast<int>(player.x), static_cast<int>(player.y), static_cast<int>(player.size), static_cast<int>(player.size), GREEN);
		}
	}

	DrawHitboxes();

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
	GetFont().Printf(150, 75, "BIG COIN: %d / %d", player.bigCoins, CountMapSymbol(currentStage, 'B'));
	GetFont().Printf(20, 100, "SPACE: Dash & Destroy Blocks / Release = Reverse Direction");
	GetFont().Printf(20, 120, "Block: # = Breakable on Dash / X = Unbreakable");
	if (debug) {
		GetFont().Printf(20, 140, "DEBUG ON (F1): hitbox");
	}
	GetFont().Printf(1000, 20, "STAGE %d : %s", currentStage + 1, kStages[currentStage].name);

	if (boss.isActive) {
		GetFont().Printf(340, 12, "BOSS   HP: %d / %d", boss.hp, boss.maxHp);
		DrawHpBar(340, 32, 600, 18, boss.hp, boss.maxHp, RED);
	}

	if (isGameOver) {
		Novice::DrawBox(400, 240, 480, 230, 0.0f, kColorPanel, kFillModeSolid);
		Novice::DrawBox(400, 240, 480, 230, 0.0f, WHITE, kFillModeWireFrame);
		GetFont().Printf(580, 275, "GAME OVER");
		GetFont().Printf(520, 320, "SCORE: %d    COIN: %d", player.score, player.coins);
		GetFont().Printf(540, 390, "R     : Retry");
		GetFont().Printf(540, 415, "ENTER : Stage Select");
	}

	if (isStageClear) {
		Novice::DrawBox(400, 240, 480, 260, 0.0f, kColorPanel, kFillModeSolid);
		Novice::DrawBox(400, 240, 480, 260, 0.0f, kColorYellow, kFillModeWireFrame);
		GetFont().Printf(570, 275, "STAGE CLEAR!");
		GetFont().Printf(520, 320, "COIN: %d    HP BONUS: +%d", player.coins, clearBonus);
		GetFont().Printf(520, 345, "SCORE: %d    BEST: %d", player.score, bestScore[currentStage]);
		GetFont().Printf(520, 370, "BIG COIN: %d / %d", player.bigCoins, CountMapSymbol(currentStage, 'B'));
		GetFont().Printf(540, 420, "R     : Retry");
		GetFont().Printf(540, 445, "ENTER : Stage Select");
	}
}

// ============================================================
// メイン関数
// ============================================================
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	Novice::Initialize(kWindowTitle, kScreenW, kScreenH);

	GetFont().Create(L"PixelMplus12-Bold.ttf", L"PixelMplus12", 16);

	LoadSprites();

	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	for (int i = 0; i < kStageCount; i++) {
		stageCleared[i] = false;
		bestScore[i] = 0;
		bigCoinRecord[i] = 0;
	}
	ResetGame(0);

	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();

		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

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

		Novice::EndFrame();

		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	Novice::Finalize();
	return 0;
}