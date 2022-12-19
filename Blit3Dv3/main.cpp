//Asteroid's game by Tiago
//memory leak detection
#define CRTDBG_MAP_ALLOC
#ifdef _DEBUG
	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
		#define new DBG_NEW
	#endif
#endif  // _DEBUG

#include <stdlib.h>
#include <crtdbg.h>
#include <random>
#include "Blit3D.h"
#include "Ship.h"

extern std::mt19937 rng;


//GLOBAL DATA
double elapsedTime = 0;
float timeSlice = 1.f / 60.f;
bool shoot = false;
int score = 0;
int level = 0;

//objects
Blit3D *blit3D = NULL;
Ship* ship = NULL;
AngelcodeFont* neon80s = NULL;

//sprites
Sprite* backgroundSprite = NULL;    
Sprite* largeAsteroid = NULL;		
Sprite* mediumAsteroid = NULL;		
Sprite* smallAsteroid = NULL;		
Sprite* lifeSprite = NULL;

//vectors
std::vector<Shot> shotList;
std::vector<Asteroid> asteroidList;
std::vector<Sprite*> explosionSpriteList;
std::vector<Explosion> explosionList;

//constants
enum class GameState { PLAYING, PAUSE, GAMEOVER, START };
GameState gameState = GameState::START;

//random distributions
std::uniform_real_distribution<float> spinDist2(-45.f, 45.f);
std::uniform_real_distribution<float> mediumSpeedDist2(200.f, 300.f);
std::uniform_real_distribution<float> smallSpeedDist2(350.f, 450.f);
std::uniform_real_distribution<float> angleDist2(glm::radians(5.f), glm::radians(30.f));
std::uniform_real_distribution<float> angleDist3(glm::radians(5.f), glm::radians(60.f));
std::uniform_real_distribution<float> directionDist2(0.f, 360.f);

void MakeLevel() {
	level++;

	//turn on invulnerability - activate the player's shield
	ship->shieldTimer = 3.f;

	//move the ship back to the center of the screen
	ship->position = glm::vec2(1920.f / 2, 1080.f / 2);
	ship->angle = 90;
	ship->velocity = glm::vec2(0.f, 0.f);

	//cleanup old shots, asteroids
	shotList.clear();
	asteroidList.clear();
	explosionList.clear();

	for (int i = 0; i < level; ++i){
		asteroidList.push_back(AsteroidFactory(AsteroidSize::LARGE));
	}
}

void Init()
{
	InitializeRNG(); //initializing seed/rng

	//turn cursor off
	blit3D->ShowCursor(false);

	//load our background image: the arguments are upper-left corner x, y, width to copy, height to copy, and file name.
	backgroundSprite = blit3D->MakeSprite(0, 0, 1920, 1080, "Media\\back.png");

	//load an Angelcode binary32 font file.
	neon80s = blit3D->MakeAngelcodeFontFromBinary32("Media\\neon80s.bin");

	//create a ship
	ship = new Ship;
	
	//load a sprite off of a spritesheet
	for (int i = 0; i < 4; ++i)
		ship->spriteList.push_back(blit3D->MakeSprite(i * 72, 0, 72, 88, "Media\\Player_Ship2c.png"));

	ship->position = glm::vec2(1920.f / 2, 1080.f / 2);

	//load our small ship life counter
	lifeSprite = blit3D->MakeSprite(0, 0, 88, 88, "Media\\Player_Ship_small.png");

	//load the shield graphic
	ship->shieldSprite = blit3D->MakeSprite(0, 0, 60, 60, "Media\\shield.png");

	//load the shot graphic
	ship->shotSprite = blit3D->MakeSprite(0, 0, 8, 8, "Media\\shot.png");
	
	//set the clear colour
	glClearColor(1.0f, 0.0f, 1.0f, 0.0f);	//clear colour: r,g,b,a 		

	//load a sprite off of a spritesheet
	largeAsteroid = blit3D->MakeSprite(0, 0, 240, 240, "Media\\L10.png");
	mediumAsteroid = blit3D->MakeSprite(0, 0, 120, 120, "Media\\M10.png");
	smallAsteroid = blit3D->MakeSprite(0, 0, 64, 64, "Media\\S10.png");

	for (int i = 0; i < 10; ++i)
	{
		//it generates random asteroids between 0 to 10 for each level except the first
		asteroidList.push_back(AsteroidFactory(MakeRandomAsteroidSize()));
	}

	//load explosions
	for (int i = 0; i < 8; ++i)
	{
		explosionSpriteList.push_back(blit3D->MakeSprite(1 + i * 63, 0, 63, 63, "Media\\explosion.png"));
	}
}

void DeInit(void)
{
	//delete the object to deallocate memory
	if(ship != NULL) delete ship;
}

void Update(double seconds)
{
	//only update time to a maximun amount
	if (seconds < 0.15){
		elapsedTime += seconds;
	}
	else {
		elapsedTime += 0.15;
	}

	switch (gameState) //alters the game's status
	{
		case GameState::PLAYING:
		{
			//update by a full timeslice when it's time
			while (elapsedTime >= timeSlice)
			{
				elapsedTime -= timeSlice;
				ship->Update(timeSlice);

				if (shoot) {
					ship->Shoot(shotList);
				}

				//iterate backwards through the shotlist for the next loop
				for (int i = shotList.size() - 1; i >= 0; --i)
				{
					//shot Update() returns false when the bullet should be killed off
					if (!shotList[i].Update(timeSlice)) {
						shotList.erase(shotList.begin() + i);
					}
				}

				//explosion update
				for (int i = explosionList.size() - 1; i >= 0; --i)
				{
					if (!explosionList[i].Update(timeSlice)) {
						explosionList.erase(explosionList.begin() + i);
					}
				}

				for (auto& A : asteroidList) A.Update(timeSlice);

				//verifies the collision between the shots and asteroids
				for (int aIndex = asteroidList.size() - 1; aIndex >= 0; --aIndex)
				{
					for (int sIndex = shotList.size() - 1; sIndex >= 0; --sIndex)
					{
						//verifies the collision
						if (CollideAsteroidWithShot(asteroidList[aIndex], shotList[sIndex]))
						{
							//remove the shot
							shotList.erase(shotList.begin() + sIndex);

							//erase/split the asteroids
							switch (asteroidList[aIndex].size)
							{
							case AsteroidSize::LARGE:
							{
								//make an explosion
								Explosion E(asteroidList[aIndex].position, 5.f);
								explosionList.push_back(E);
								score++;
								//calculate the original asteroid's angle of motion
								float angle = atan2(asteroidList[aIndex].velocity.y, asteroidList[aIndex].velocity.x);

								for (int i = 0; i < 5; ++i)
								{
									float angleTheAsteroid = angle;
									//generates medium asteroids
									Asteroid A;
									A.size = AsteroidSize::MEDIUM;
									A.position = asteroidList[aIndex].position;
									A.spin = spinDist2(rng);
									A.radius2 = (120.f / 2) * (120.f / 2);
									A.sprite = mediumAsteroid;
									A.angle = directionDist2(rng);

									if (i == 0) angleTheAsteroid -= angleDist2(rng);
									else if (i == 1) angleTheAsteroid += angleDist2(rng);
									else if (i == 2) angleTheAsteroid -= angleDist3(rng);
									else angleTheAsteroid + angleDist3(rng);

									//velocity from the new angle
									A.velocity.x = cos(angleTheAsteroid);
									A.velocity.y = sin(angleTheAsteroid);
									A.velocity *= mediumSpeedDist2(rng);
									asteroidList.push_back(A);
								}
							}
							break;
							case AsteroidSize::MEDIUM:
							{
								//make an explosion
								Explosion E(asteroidList[aIndex].position, 2.f);
								explosionList.push_back(E);
								score++;
								float angle = atan2(asteroidList[aIndex].velocity.y, asteroidList[aIndex].velocity.x);

								for (int i = 0; i < 5; ++i)
								{
									float angleTheAsteroid = angle;
									//generates medium asteroids
									Asteroid A;
									A.size = AsteroidSize::SMALL;
									A.position = asteroidList[aIndex].position;
									A.spin = spinDist2(rng);
									A.radius2 = (120.f / 2) * (120.f / 2);
									A.sprite = smallAsteroid;
									A.angle = directionDist2(rng);

									if (i == 0) angleTheAsteroid -= angleDist2(rng);
									else if (i == 1) angleTheAsteroid += angleDist2(rng);
									else if (i == 2) angleTheAsteroid -= angleDist3(rng);
									else angleTheAsteroid + angleDist3(rng);

									//velocity from the new angle
									A.velocity.x = cos(angleTheAsteroid);
									A.velocity.y = sin(angleTheAsteroid);
									A.velocity *= smallSpeedDist2(rng);
									asteroidList.push_back(A);
								}
							}
							break;
							case AsteroidSize::SMALL: //delete small asteroids
							{
								//make an explosion
								Explosion E(asteroidList[aIndex].position, 1.f);
								explosionList.push_back(E);
								score++;
							}
							break;
							}//end switch
							asteroidList.erase(asteroidList.begin() + aIndex);
							break; //go to the next asteroid
						}//end the collisions IF
					}//end of collisions inner loop
				}//end of collision loop

				if (asteroidList.empty())
				{
					MakeLevel();
					gameState = GameState::PAUSE;
					break;
				}

				for (auto A : asteroidList)
				{
					//check for collision with the ship
					if (CollideWithAsteroids(A, ship))
					{
						//take away a life
						ship->lives--;
						//check for game over
						if (ship->lives < 1)
						{
							gameState = GameState::GAMEOVER;
							break;
						}

						//turn on invulnerability - activate the player's shield
						ship->shieldTimer = 5.f;

						//make an explosion
						Explosion e(ship->position, 3.f);
						explosionList.push_back(e);

						//move the ship to the center of the screen
						ship->position = glm::vec2(1920.f / 2, 1080.f / 2);
						ship->angle = 90;
						ship->velocity = glm::vec2(0.f, 0.f);
					}
				}
			}
		}//end PLAYING case
		break;
		case GameState::START:
		{
			level = 0;
			ship->lives = 3;
			MakeLevel();
			gameState = GameState::PAUSE;
		}
		break;
		case GameState::PAUSE:
		case GameState::GAMEOVER:
		default:
			break;
	
	}//end gameState switch
}//end update

void Draw(void)
{
	//wipe the drawing surface clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (gameState)
	{
		case GameState::PAUSE:
		{
			backgroundSprite->Blit(1920.f / 2, 1080.f / 2);

			for (auto& A : asteroidList) A.Draw();

			//draw the ship
			ship->Draw();

			//draw the shots
			for (auto& S : shotList) S.Draw();

			//draw explosions
			for (auto& E : explosionList) E.Draw();

			//draw our life HUD
			for (int i = 0; i < ship->lives; ++i)
			{
				lifeSprite->Blit(100 + i * 32, 1080 - 50);
			}

			//message to start the level
			std::string start = "Press P to start";
			float widthText = neon80s->WidthText(start);
			neon80s->BlitText(1920.f / 2.f - widthText / 2.f, 1080.f / 2.f, start);
		}//end PAUSE case
		break;
		case GameState::GAMEOVER:
		{
			backgroundSprite->Blit(1920.f / 2, 1080.f / 2);

			for (auto& A : asteroidList) A.Draw();

			//draw the ship
			ship->Draw();

			//draw the shots
			for (auto& S : shotList) S.Draw();

			//draw explosions
			for (auto& E : explosionList) E.Draw();

			//draw our life HUD
			for (int i = 0; i < ship->lives; ++i)
			{
				lifeSprite->Blit(100 + i * 32, 1080 - 50);
			}

			//messages when game is over
			std::string gameOver = "GAME OVER";
			float textGameOver = neon80s->WidthText(gameOver);
			neon80s->BlitText(1920.f / 2.f - textGameOver / 2.f, 1080.f / 2.f, gameOver);
		
			std::string finalScore = "Final score: " + std::to_string(score);		
			float textFinalScore = neon80s->WidthText(finalScore);
			neon80s->BlitText(1920.f / 2.f - textFinalScore / 2.f, 1080.f / 2.f - 100.f, finalScore);
		
			std::string pressG = "Press G to start a new game";
			float textPressG = neon80s->WidthText(pressG);
			neon80s->BlitText(1920.f / 2.f - textPressG / 2.f, 1080.f / 2.f - 200.f, pressG);
		}//end GAMEOVER case
		break;
		case GameState::START:
		case GameState::PLAYING:
		{
			//draw the background in the middle of the screen
			//the arguments to Blit(0 are the x, y pixel coords to draw the center of the sprite at, 
			//starting as 0,0 in the bottom-left corner.
			backgroundSprite->Blit(1920.f / 2, 1080.f / 2);

			//draw the asteroids
			for (auto& A : asteroidList) A.Draw();

			//draw the ship
			ship->Draw();
			//draw the shots
			for (auto& S : shotList) S.Draw();

			//text for score
			std::string textScore = "Score " + std::to_string(score);
			float scoreWidth = neon80s->WidthText(textScore);
			//Draw the text
			neon80s->BlitText(3320.f / 2 - scoreWidth / 2, 2100.f / 2, textScore);

			//text for lives
			std::string textLevel = "Level " + std::to_string(level);
			float livesWidth = neon80s->WidthText(textLevel);
			//Draw the text
			neon80s->BlitText(3350.f / 2 - livesWidth / 2, 1900.f / 2, textLevel);

			//draw the explosions
			for (auto& E : explosionList) E.Draw();

			//draw our life HUD
			for (int i = 0; i < ship->lives; ++i)
			{
				lifeSprite->Blit(100 + i * 64, 1080 - 50);
			}
		}//end PLAYING case
		break;
	}//end gameState switch
}//end Draw

//the key codes/actions/mods for DoInput are from GLFW: check its documentation for their values
void DoInput(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		blit3D->Quit(); //start the shutdown sequence

	switch (gameState)
	{	
	case GameState::GAMEOVER:
		if (key == GLFW_KEY_G && action == GLFW_RELEASE){
			score = 0;
			gameState = GameState::START;
		}			
		break;
	case GameState::PAUSE:	
		if (key == GLFW_KEY_P && action == GLFW_RELEASE)
			gameState = GameState::PLAYING;
	case GameState::PLAYING:
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
			ship->turningLeft = true;

		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
			ship->turningLeft = false;

		if (key == GLFW_KEY_D && action == GLFW_PRESS)
			ship->turningRight = true;

		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
			ship->turningRight = false;

		if (key == GLFW_KEY_W && action == GLFW_PRESS)
			ship->thrusting = true;

		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
			ship->thrusting = false;

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
			shoot = true;

		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
			shoot = false;

		if (key == GLFW_KEY_P && action == GLFW_PRESS)
			gameState = GameState::PAUSE;
		break;
	case GameState::START:
		gameState = GameState::PAUSE;
	default:
		break;
	}//end gameState switch
}//end DoInput

int main(int argc, char *argv[])
{
	//memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	//set X to the memory allocation number in order to force a break on the allocation:
	//useful for debugging memory leaks, as long as your memory allocations are deterministic.
	//_crtBreakAlloc = X;

	blit3D = new Blit3D(Blit3DWindowModel::BORDERLESSFULLSCREEN_1080P, 640, 400);

	//set our callback funcs
	blit3D->SetInit(Init);
	blit3D->SetDeInit(DeInit);
	blit3D->SetUpdate(Update);
	blit3D->SetDraw(Draw);
	blit3D->SetDoInput(DoInput);

	//Run() blocks until the window is closed
	blit3D->Run(Blit3DThreadModel::SINGLETHREADED);
	if (blit3D) delete blit3D;
}