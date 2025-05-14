#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>


static float colorShiftTime = 0.0f;
static const float COLOR_CYCLE_SPEED = 0.1f;

struct Explosion {
    Vector2 position;
    float time = 0.3f;
    float timer = 0.0f;

    bool Update(float dt) {
        timer += dt;
        return timer > time;
    }

    void Draw() const {
        float alpha = 1.0f - (timer / time);
        Color c = ColorAlpha(ORANGE, alpha);
        DrawCircleV(position, 20 * alpha, c);
    }
};


void DrawSpaceGradient()
{
    float screenWidth = static_cast<float>(GetScreenWidth());
    float screenHeight = static_cast<float>(GetScreenHeight());
    
    colorShiftTime += GetFrameTime() * COLOR_CYCLE_SPEED;
 
    Color topColor = {
        static_cast<unsigned char>(100 + 50 * sinf(colorShiftTime * 0.7f)),       // Red
        static_cast<unsigned char>(80 + 40 * sinf(colorShiftTime * 1.2f + 2.1f)), // Green
        static_cast<unsigned char>(120 + 60 * sinf(colorShiftTime * 0.9f + 1.5f)),// Blue
        255
    };
    
    Color bottomColor = {
        static_cast<unsigned char>(30 + 20 * sinf(colorShiftTime * 0.5f + 1.0f)), 
        static_cast<unsigned char>(20 + 15 * sinf(colorShiftTime * 0.8f + 3.1f)),
        static_cast<unsigned char>(40 + 30 * sinf(colorShiftTime * 0.6f + 2.3f)),
        255
    };
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, topColor, bottomColor);
}

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

class Starfield {
public:
    struct Star {
        Vector2 position;
        float speed;
        float speedX;
        float size;
    };

    Starfield(int count, int screenWidth, int screenHeight) {
        stars.resize(count);
        for (auto& star : stars) {
            star.position = {
                Utils::RandomFloat(-screenWidth, screenWidth*2),
                Utils::RandomFloat(-screenHeight, screenHeight*2)
            };
            star.speed = Utils::RandomFloat(10.f, 50.f);
            star.speedX = Utils::RandomFloat(-10.f, 10.f);
            star.size = Utils::RandomFloat(0.5f, 2.f);
        }
    }

    void Update(float dt) {
        float screenW = Renderer::Instance().Width();
        float screenH = Renderer::Instance().Height();
        
        for (auto& star : stars) {
            star.position.y += star.speed * dt;
            star.position.x += star.speedX * dt;
            
            if (star.position.y > screenH || star.position.x < 0 || star.position.x > screenW) {
                star.position = {
                    Utils::RandomFloat(0, screenW),
                    Utils::RandomFloat(-screenH, 0)
                };
                star.speed = Utils::RandomFloat(10.f, 50.f);
            }
        }
    }

    void Draw() const {
        for (const auto& star : stars) {
            DrawCircleV(star.position, star.size, WHITE);
        }
    }

private:
    std::vector<Star> stars;
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

protected:
Color color;
	void init(int screenW, int screenH) {
		// Choose size
		render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));
		
		 color = {
            static_cast<unsigned char>(50 + GetRandomValue(0, 205)),
            static_cast<unsigned char>(50 + GetRandomValue(0, 205)),
            static_cast<unsigned char>(50 + GetRandomValue(0, 205)),
            255
        };

		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);
	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};


class TriangleAsteroid : public Asteroid {
public:
    TriangleAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
    void Draw() const override {
        DrawPoly(transform.position, 3, GetRadius(), transform.rotation, color);
    }
};

class SquareAsteroid : public Asteroid {
public:
    SquareAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 10; }
    void Draw() const override {
        DrawRectanglePro(
            {transform.position.x, transform.position.y, GetRadius()*2, GetRadius()*2},
            {GetRadius(), GetRadius()}, transform.rotation, color
        );
    }
};

class HexagonAsteroid : public Asteroid {
public:
    HexagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 20; }
    void Draw() const override {
        DrawPoly(transform.position, 6, GetRadius(), transform.rotation, color);
    }
};

class CircleAsteroid : public Asteroid {
public:
    CircleAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 25; }
    void Draw() const override {
        DrawCircleV(transform.position, GetRadius(), color);
    }
};

class PentagonAsteroid : public Asteroid {
public:
    PentagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 15; }
    void Draw() const override {
        DrawPoly(transform.position, 5, GetRadius(), transform.rotation, color);
    }
};


// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h) {
    int shape = GetRandomValue(0, 4);
    
    switch (shape) {
        case 0: return std::make_unique<TriangleAsteroid>(w, h);
        case 1: return std::make_unique<SquareAsteroid>(w, h);
        case 2: return std::make_unique<PentagonAsteroid>(w, h);
        case 3: return std::make_unique<HexagonAsteroid>(w, h);
        case 4: return std::make_unique<CircleAsteroid>(w, h);
        default: return std::make_unique<TriangleAsteroid>(w, h);
    }
}

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, COUNT };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}
	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}
	void Draw() const {
    if (this->type == WeaponType::BULLET) {
        DrawCircleGradient(
            transform.position.x, transform.position.y, 
            5.f, 
            YELLOW, 
            ColorAlpha(RED, 0.5f)
        );
    }
    else {
        const float laserLength = 50.f;
        const float coreWidth = 5.f;
        const float glowWidth = 10.f;
        float pulse = 0.8f + 0.2f * sinf(GetTime() * 15.f);
        
        Vector2 start = transform.position;
        Vector2 end = { start.x, start.y - laserLength };
        
        // Color - neon
        Color outerGlow = { 0, 200, 255, 150 };
        Color middleLayer = { 50, 255, 150, 200 };
        Color core = { 200, 255, 255, 255 };
        Color sparkColor = { 100, 255, 200, 255 };

        DrawLineEx(start, end, glowWidth, outerGlow);
        DrawLineEx(start, end, coreWidth*1.7f, middleLayer);
        DrawLineEx(start, end, coreWidth*pulse, core);

		}
	}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		return (type == WeaponType::BULLET) ? 5.f : 2.f;
	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
};

inline static Projectile MakeProjectile(WeaponType wt,
    const Vector2 pos,
    float speed)
{
    Vector2 vel{ 0, -speed };
    if (wt == WeaponType::LASER) {
        return Projectile(pos, vel, 20, wt);
    }
    else {
        return Projectile(pos, vel, 10, wt);
    }
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
												 screenW * 0.5f,
												 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 18.f; // shots/sec
		fireRateBullet = 22.f;
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 20.f;
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) alive = false;
	}

	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? fireRateLaser : fireRateBullet;
	}

	float GetSpacing(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? spacingLaser : spacingBullet;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      spacingLaser;
	float      spacingBullet;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("spaceship1.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		scale = 0.25f;
	}
	~PlayerShip() {
		UnloadTexture(texture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);
	}

	float GetRadius() const override {
		return (texture.width * scale) * 0.5f;
	}

private:
	Texture2D texture;
	float     scale;
};

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

			starfield.Update(dt);
    
    		spawnTimer += dt;

			// Update player
			player->Update(dt);

			// Restart logic
			if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
				player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
				asteroids.clear();
				projectiles.clear();
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::RANDOM;
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();
						projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed));
						shotTimer -= interval;
					}
				}
				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}
			}
			
			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;
			
				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());
					if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {
						explosions.push_back({ (*ait)->GetPosition() });
						ait = asteroids.erase(ait);
						pit = projectiles.erase(pit);
						removed = true;
						break;
					}
				}
				if (!removed) {
					++pit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision =
					[&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
			}

			// Render everything
			{
				Renderer::Instance().Begin();
				DrawSpaceGradient();
				starfield.Draw();


				auto it = std::remove_if(explosions.begin(), explosions.end(), [dt](Explosion& e) {
						return e.Update(dt);
					});
					explosions.erase(it, explosions.end());

					for (const auto& e : explosions) {
						e.Draw();
					}

					
				// Nowy pasek HP
				float hpPercent = player->GetHP() / 100.0f;
				Color hpColor = hpPercent > 0.6f ? GREEN : 
							hpPercent > 0.3f ? YELLOW : RED;
				
				// Tło paska HP
				DrawRectangle(10, 10, 200, 20, Fade(BLACK, 0.5f));
				// Wypełnienie paska HP
				DrawRectangle(10, 10, 200 * hpPercent, 20, hpColor);
				// Obramowanie
				DrawRectangleLines(10, 10, 200, 20, WHITE);
				// Tekst HP
				DrawText(TextFormat("HP: %d", player->GetHP()), 220, 10, 20, WHITE);


				const char* weaponName = (currentWeapon == WeaponType::LASER) ? "LASER" : "BULLET";
				DrawText(TextFormat("Weapon: %s", weaponName),
					10, 40, 20, BLUE);

				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}

				player->Draw();

				Renderer::Instance().End();
			}
		}
	}

private:
	Application()
    : starfield(200, C_WIDTH, C_HEIGHT)
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
	};
	Starfield starfield;
	std::vector<Explosion> explosions;
	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;

	AsteroidShape currentShape = AsteroidShape::TRIANGLE;

	static constexpr int C_WIDTH = 1600;
	static constexpr int C_HEIGHT = 800;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;

	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
};

int main() {
	Application::Instance().Run();
	return 0;
}
