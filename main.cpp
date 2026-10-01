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
const int kMaxBullets = 32;
const float kScrollSpeed = 4.0f; // 障害物・コインが左へ流れる速度

// 配列サイズ
const int kMaxObstacles = 16;
const int kMaxEnemies = 8;
const int kMaxCoins = 32;

// 色
const unsigned int kColorYellow = 0xFFFF00FF;
const unsigned int kColorOrange = 0xFFA500FF;
const unsigned int kColorCyan = 0x00FFFFFF;
const unsigned int kColorBrown = 0xCC8844FF;
const unsigned int kColorPurple = 0x9944DDFF;
const unsigned int kColorGray = 0x888888FF;
const unsigned int kColorPink = 0xFF66CCFF;

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
	int shotTimer;
	int frame;
	int type; // 0: 揺れて撃つ / 1: 揺れず撃たない
	bool isActive;
};

struct Coin {
	float x, y, radius;
	bool isActive;
};

// ============================================================
// ステージデータ(ここを書き換えて配置をカスタマイズ!)
// ============================================================
// ・frame : 出現するフレーム(60フレーム = 1秒)。画面の右端から出てきます。
//           表の上から下へ、小さい順に並べてください。
// ・y     : 出現するY座標(画面の高さは720。上が0)
// ・横方向の位置は frame で調整します。
//   障害物とコインは 4px/フレーム、敵は 2px/フレームで左へ流れます。
//   (右端から自機のX=300付近まで、障害物は約245フレームで届きます)

enum SpawnKind {
	kSpawnObstacle,
	kSpawnEnemy,
	kSpawnCoin,
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

// ---------- ステージの配置表 ----------
const SpawnData kStageData[] = {
	MakeCoin(60, 360.0f, 3),

	MakeObstacle(150, 0.0f, 40.0f, 250.0f, kObstacleNormal),   // 上から
	MakeObstacle(210, 470.0f, 40.0f, 250.0f, kObstacleNormal), // 下から
	MakeCoin(240, 300.0f, 3),

	MakeEnemy(360, 250.0f, kEnemyShooter, 3),
	MakeCoin(400, 450.0f, 2),

	MakeObstacle(480, 250.0f, 40.0f, 200.0f, kObstacleChargeOnly),
	MakeEnemy(540, 500.0f, kEnemyStatic, 1),
	MakeEnemy(580, 500.0f, kEnemyStatic, 1),
	MakeEnemy(620, 500.0f, kEnemyStatic, 1),
	MakeEnemy(660, 500.0f, kEnemyStatic, 1),
	MakeEnemy(700, 500.0f, kEnemyStatic, 1),
	MakeEnemy(740, 500.0f, kEnemyStatic, 1),

	// 同じフレームに2つ出すと、すき間のある壁になります
	//MakeObstacle(600, 0.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	//MakeObstacle(600, 420.0f, 40.0f, 300.0f, kObstacleChargeOnly),
	//MakeCoin(660, 360.0f, 3),

	//MakeEnemy(780, 150.0f, kEnemyShooter, 3),
	//MakeEnemy(780, 550.0f, kEnemyShooter, 3),
	//MakeCoin(900, 200.0f, 3),

	MakeObstacle(960, 300.0f, 40.0f, 120.0f, kObstacleNormal),
	MakeEnemy(1020, 360.0f, kEnemyStatic, 5),
	MakeCoin(1100, 500.0f, 3),

	MakeObstacle(1200, 0.0f, 40.0f, 330.0f, kObstacleNormal),
	MakeObstacle(1200, 410.0f, 40.0f, 310.0f, kObstacleChargeOnly),
	MakeEnemy(1320, 200.0f, kEnemyShooter, 3),
	MakeCoin(1400, 350.0f, 3),

	MakeEnemy(1500, 450.0f, kEnemyStatic, 3),
	MakeObstacle(1560, 100.0f, 40.0f, 150.0f, kObstacleChargeOnly),
	MakeCoin(1650, 300.0f, 3),
};
const int kStageDataCount = static_cast<int>(sizeof(kStageData) / sizeof(kStageData[0]));

const int kStageLength = 1800;  // ステージの長さ(フレーム)
const bool kLoopStage = false;   // true: 最後までいったら最初から繰り返す

// ============================================================
// ゲームデータ
// ============================================================
Player player;
Bullet playerBullets[kMaxBullets];
Bullet enemyBullets[kMaxBullets];
Obstacle obstacles[kMaxObstacles];
Enemy enemies[kMaxEnemies];
Coin coins[kMaxCoins];

int stageFrame; // ステージ開始からの経過フレーム
int spawnIndex; // 次に出現させるステージデータの番号
bool isGameOver;

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

// 円と円の当たり判定
bool CircleCircleHit(float ax, float ay, float ar, float bx, float by, float br) {
	float dx = ax - bx;
	float dy = ay - by;
	float rr = ar + br;
	return dx * dx + dy * dy <= rr * rr;
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
// 初期化・リセット
// ============================================================
void ResetGame() {
	player.x = 300.0f;
	player.y = 300.0f;
	player.size = 40.0f;
	player.vy = 0.0f;
	player.hp = kPlayerMaxHp;
	player.invincible = 0;
	player.charge = 0;
	player.score = 0;
	player.coins = 0;

	for (int i = 0; i < kMaxBullets; i++) {
		playerBullets[i].isActive = false;
		enemyBullets[i].isActive = false;
	}
	for (int i = 0; i < kMaxObstacles; i++) obstacles[i].isActive = false;
	for (int i = 0; i < kMaxEnemies; i++) enemies[i].isActive = false;
	for (int i = 0; i < kMaxCoins; i++) coins[i].isActive = false;

	stageFrame = 0;
	spawnIndex = 0;
	isGameOver = false;
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

// ============================================================
// Windowsアプリでのエントリーポイント(main関数)
// ============================================================
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, kScreenW, kScreenH);

	// キー入力結果を受け取る箱
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	ResetGame();

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

		if (isGameOver) {
			// Rキーでリスタート
			if (preKeys[DIK_R] == 0 && keys[DIK_R] != 0) {
				ResetGame();
			}
		}
		else {

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
			while (spawnIndex < kStageDataCount && kStageData[spawnIndex].frame <= stageFrame) {
				SpawnFromData(kStageData[spawnIndex]);
				spawnIndex++;
			}
			stageFrame++;
			if (kLoopStage && stageFrame >= kStageLength) {
				stageFrame = 0;
				spawnIndex = 0;
			}

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
						if (o.type == 0) {
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

			// ---------- ゲームオーバー判定 ----------
			// 画面外(下)に落下したら即死
			if (player.y > kScreenH) {
				player.hp = 0;
			}
			if (player.hp <= 0) {
				player.hp = 0;
				isGameOver = true;
			}
		}

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		// ---------- 障害物 ----------
		for (int i = 0; i < kMaxObstacles; i++) {
			const Obstacle& o = obstacles[i];
			if (!o.isActive) continue;
			unsigned int color = (o.type == 0) ? kColorBrown : kColorPurple;
			Novice::DrawBox(static_cast<int>(o.x), static_cast<int>(o.y), static_cast<int>(o.w), static_cast<int>(o.h), 0.0f, color, kFillModeSolid);
			// チャージ専用は枠線で目立たせる
			if (o.type == 1) {
				Novice::DrawBox(static_cast<int>(o.x), static_cast<int>(o.y), static_cast<int>(o.w), static_cast<int>(o.h), 0.0f, WHITE, kFillModeWireFrame);
			}
		}

		// ---------- コイン ----------
		for (int i = 0; i < kMaxCoins; i++) {
			const Coin& c = coins[i];
			if (!c.isActive) continue;
			Novice::DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y), static_cast<int>(c.radius), static_cast<int>(c.radius), 0.0f, kColorYellow, kFillModeSolid);
		}

		// ---------- 敵 ----------
		for (int i = 0; i < kMaxEnemies; i++) {
			const Enemy& e = enemies[i];
			if (!e.isActive) continue;
			unsigned int color = (e.type == kEnemyShooter) ? RED : kColorPink;
			Novice::DrawBox(static_cast<int>(e.x), static_cast<int>(e.y), static_cast<int>(e.size), static_cast<int>(e.size), 0.0f, color, kFillModeSolid);
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
		Novice::ScreenPrintf(20, 100, "SPACE: Float / Release = Shot / Hold = Charge Shot");
		Novice::ScreenPrintf(20, 120, "Brown = Normal Shot OK / Purple = Charge Shot Only");
		Novice::ScreenPrintf(20, 140, "Red = Shooter / Pink = Static (no shot)");

		if (isGameOver) {
			Novice::ScreenPrintf(560, 320, "GAME OVER");
			Novice::ScreenPrintf(540, 345, "SCORE: %d  COIN: %d", player.score, player.coins);
			Novice::ScreenPrintf(540, 370, "Press R to Restart");
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