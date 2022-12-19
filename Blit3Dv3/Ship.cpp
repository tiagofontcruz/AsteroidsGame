#include "Ship.h"
#include<random>

std::mt19937 rng;

void Shot::Draw(){
	sprite->Blit(position.x, position.y); //draw the shots
}

bool Shot::Update(float seconds)
{	
	//determine if we are past the end of our life time, if so return false
	bool shotFlag = true;
	timeToLive -= seconds;
	if (timeToLive <= 0)
	{
		shotFlag = false;
	}	
	//move the shot
	position += velocity * seconds;
	
	//wrap around
	if (position.x < 0) position.x += 1920.f;
	if (position.x > 1920) position.x -= 1920.f;
	if (position.y < 0) position.y += 1080.f;
	if (position.y > 1080) position.y -= 1080.f;
	
	return shotFlag;
}

void Ship::Draw()
{
	//change ship angle because graphics face "up"
	spriteList[frameTime]->angle = angle - 90;
	
	//draw main ship sprite
	spriteList[frameTime]->Blit(position.x, position.y);

	//wrap-around
	for (float xoffset = -1920; xoffset <= 1920; xoffset += 1920)
	{
		for (float yoffset = -1080; yoffset <= 1080; yoffset += 1080)
		{
			spriteList[frameTime]->Blit(position.x + xoffset, position.y + yoffset);
			//draw shield if timer > 0
			if (shieldTimer > 0)
			{
				if (blink) shieldSprite->Blit(position.x + xoffset, position.y + yoffset, 1.5f, 1.5f, 0.6f); //shields blinker draw
			}
		}
	}

	//redraw if too close to an edge	
	if (position.x < radius + 10.f) { //left
		spriteList[frameTime]->Blit(position.x + 1920.f, position.y);
	}	
	if (position.x > 1920.f - (radius + 10.f)) { //right
		spriteList[frameTime]->Blit(position.x - 1920.f, position.y);
	}	
	if (position.y < radius + 10.f) { //down
		spriteList[frameTime]->Blit(position.x, position.y + 1080.f);
	}	
	if (position.y > 1080.f - (radius + 10.f)) { //up
		spriteList[frameTime]->Blit(position.x, position.y - 1080.f);
	}

	//copies for 4 diagonal corners
	spriteList[frameTime]->Blit(position.x + 1920.f, position.y + 1080.f);
	spriteList[frameTime]->Blit(position.x - 1920.f, position.y - 1080.f);
	spriteList[frameTime]->Blit(position.x - 1920.f, position.y + 1080.f);
	spriteList[frameTime]->Blit(position.x + 1920.f, position.y - 1080.f);
}
void Ship::Update(float seconds)
{
	//handle turning
	if (turningLeft){
		angle += 180.f * seconds;
	}
	if (turningRight){
		angle -= 180.f * seconds;
	}

	if (thrusting)
	{
		//calculate facing vector
		float radians = angle * (M_PI / 180);
		glm::vec2 facing;
		facing.x = std::cos(radians);
		facing.y = std::sin(radians);
		facing *= seconds * 400.f;
		velocity += facing;

		//check if over max speed
		if (velocity.length() > 600.f){
			velocity = glm::normalize(velocity) * 600.f;
		}
		thrustTimer += seconds;
		
		//animation timing
		if (thrustTimer >= 1.f / 20.f)
		{
			//change frames
			frameTime++;
			if (frameTime > 3) {
				frameTime = 1;
			}
			thrustTimer -= 1.f / 20.f;
		}
	}
	else {
		frameTime = 0;
	}

	//update position
	position += velocity * seconds;

	//bounds check position
	if (position.x < 0) {
		position.x += 1920.f;
	}
	if (position.x > 1920) {
		position.x -= 1920.f;
	}
	if (position.y < 0) {
		position.y += 1080.f;
	}
	if (position.y > 1080) {
		position.y -= 1080.f;
	}

	//reduce velocity due to "space friction"
	float scale = 1.f - seconds * 0.5f;
	velocity *= scale;

	//handle velocity
	float length = glm::length(velocity);
	if (std::isnan(length) || length < 0.00001f) {
		velocity = glm::vec2(0, 0);
	}

	//handle shot
	if (shotTimer > 0) {
		shotTimer -= seconds;
	}

	//handle shield timer
	if (shieldTimer > 0)
	{
		shieldTimer -= seconds;
		blinkTimer -= seconds;
		if (blinkTimer <= 0){			
			blink = !blink;
			blinkTimer = 0.15f;
		}
	}
}

bool Ship::Shoot(std::vector<Shot> &shotList)
{
	bool timerFlag = false;
	//shotTimer is set for 0.1f AND shotTimer -= seconds IF shotTimer < 0 THEN you can shoot
	if (shotTimer < 0)
	{
		timerFlag = true;	
		
		//reset shot timer (bigger number = bigger interval between shots)
		shotTimer = 0.2f;

		//make a new shot
		Shot s;

		//set the shot's sprite and position using the ship's variables
		s.sprite = shotSprite;
		s.position = position;

		//build a vector from the ship angle
		float radians = angle * (M_PI / 180);
		glm::vec2 shipAngle;
		shipAngle.x = std::cos(radians);
		shipAngle.y = std::sin(radians);
		s.velocity = shipAngle;	

		//scale up the shot velocity
		s.velocity *= 600.f;

		//add the ship velocity	
		s.velocity += velocity;

		//add the shot to the shotList
		shotList.push_back(s);

		//make a new shot
		Shot s2;

		//set the shot's sprite and position using the ship's variables
		s2.sprite = shotSprite;		
		
		//add the ship velocity	
		s2.velocity = s.velocity;

		//build a vector from the ship angle
		float radians2 = angle + 117.f;
		glm::vec2 shipAngle2;
		radians2 = glm::radians(radians2);
		shipAngle2.x = std::cos(radians2);
		shipAngle2.y = std::sin(radians2);		

		//scale up the shot velocity		
		shipAngle2 *= 30.f;
		s2.position = position + shipAngle2;		

		//add the shot to the shotList
		shotList.push_back(s2);

		//make a new shot
		Shot s3;

		//set the shot's sprite and position using the ship's variables
		s3.sprite = shotSprite;

		//add the ship velocity	
		s3.velocity = s.velocity;

		//build a vector from the ship angle
		float radians3 = angle - 117.f;
		glm::vec2 shipAngle3;
		radians3 = glm::radians(radians3);
		shipAngle3.x = std::cos(radians3);
		shipAngle3.y = std::sin(radians3);

		//scale up the shot velocity		
		shipAngle3 *= 30.f;
		s3.position = position + shipAngle3;

		//add the shot to the shotList
		shotList.push_back(s3);
	}	
	return timerFlag;
}

void Asteroid::Draw()
{
	sprite->angle = angle;
	for (float xoffset = -1920.f; xoffset <= 1920; xoffset += 1920.f)
	{
		for (float yoffset = -1080.f; yoffset <= 1080; yoffset += 1080.f)
		{			
			sprite->Blit(position.x + xoffset, position.y + yoffset, 1.f, 1.f);
		}
	}
}

void Asteroid::Update(float seconds)
{
	position += velocity * seconds;
	//wrap around and check position
	if (position.x < 0) {
		position.x += 1920.f;
	}
	if (position.x > 1920) {
		position.x -= 1920.f;
	}
	if (position.y < 0) {
		position.y += 1080.f;
	}
	if (position.y > 1000) {
		position.y -= 1080.f;
	}
	angle += spin * seconds;
}

extern Sprite* largeAsteroid;
extern Sprite* mediumAsteroid;
extern Sprite* smallAsteroid;

std::uniform_real_distribution<float> posDistX(0.f, 1920.f);
std::uniform_real_distribution<float> posDistY(0.f, 1080.f);
std::uniform_real_distribution<float> spinDist(-45.f, 45.f);
std::uniform_real_distribution<float> directionDist(0.f, 360.f);
std::uniform_real_distribution<float> largeSpeedDist(20.f, 150.f);
std::uniform_real_distribution<float> mediumSpeedDist(200.f, 300.f);
std::uniform_real_distribution<float> smallSpeedDist(350.f, 450.f);

Asteroid AsteroidFactory(AsteroidSize type)
{
	Asteroid A;
	A.angle = directionDist(rng);//angle to each asteroid
	float dirAngle = directionDist(rng); //angle direction
	A.velocity.x = cos(glm::radians(dirAngle));
	A.velocity.y = sin(glm::radians(dirAngle));
	switch (type)
	{
	case AsteroidSize::SMALL:
		A.sprite = smallAsteroid;
		A.velocity *= smallSpeedDist(rng);
		A.radius2 = (64.f / 2) * (64.f / 2);
		break;
	case AsteroidSize::MEDIUM:
		A.sprite = mediumAsteroid;
		A.velocity *= mediumSpeedDist(rng);
		A.radius2 = (120.f / 2) * (120.f / 2);
		break;
	case AsteroidSize::LARGE:
		A.sprite = largeAsteroid;
		A.velocity *= largeSpeedDist(rng);
		A.radius2 = (240.f / 2) * (240.f / 2);
		break;
	}
	A.size = type;
	A.position.x = posDistX(rng);
	A.position.y = posDistY(rng);
	A.spin = spinDist(rng);
	return A;
}

void InitializeRNG()
{
	std::random_device rd;
	rng.seed(rd());
}

std::uniform_int_distribution<int> asteroidSizeDist(0, 2);

AsteroidSize MakeRandomAsteroidSize()
{
	int size = asteroidSizeDist(rng);
	switch (size)
	{
	case 0:
		return AsteroidSize::SMALL;
		break;
	case 1:
		return AsteroidSize::MEDIUM;
		break;
	default:
		return AsteroidSize::LARGE;
		break;
	}
}

float DistanceSquared(glm::vec2 position1, glm::vec2 position2)
{
	return (position1.x - position2.x) * (position1.x  - position2.x) +
		   (position1.y - position2.y) * (position1.y - position2.y);
}

bool CollideAsteroidWithShot(Asteroid& a, Shot& s)
{
	//verifies the point of collision
	if (DistanceSquared(s.position, a.position) > a.radius2) return false;
	return true;
}

bool CollideWithAsteroids(Asteroid& a, Ship* s)
{
	//verifies the if the shield's up
	if (s->shieldTimer > 0) return false;
	if (DistanceSquared(s->position, a.position) > a.radius2 + s->radius2) return false;
	return true;
}

void Explosion::Draw()
{
	explosionSpriteList[frameNum]->Blit(position.x, position.y, scale, scale);
}

bool Explosion::Update(float seconds)
{
	//update frame time
	frameTimer += seconds;
	//see if advance the frame counter
	if (frameTimer >= frameSpeed)
	{
		if (frameNum > explosionSpriteList.size() - 2) return false;
		frameNum++; //advance to next frame
		frameTimer -= frameSpeed;
	}
	return true;
}

Explosion::Explosion(glm::vec2 location, float size)
{
	position = location;
	scale = size;
}