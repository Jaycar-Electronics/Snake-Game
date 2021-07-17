// Snake Game - JayCar Arduino For Beginners Course
// Updates made 10 April 2021 by Tony, Max and Riley Hansen:
// - Startup, Pickup and Game Over tunes added
// - "Fruit" pick-ups.  Adds 5 to your score
// - Fruit relocates every 10 moves, if not picked up
// - Game starts slower, with snake lower on matrix (more kid-friendly)
// - Ignores attempts to reverse direction, less "bounce" issues
// - Game speeds up as you pick up fruit
// - "Tension" tone rises as you pick up fruit
// - Extensive code refactoring

#define MAX7219DIN 4
#define MAX7219CS 5
#define MAX7219CLK 6
#define JOYX A1
#define JOYY A0
#define BUTTON 2
#define SPEAKER 3
#define ELEMENTS(x)   (sizeof(x) / sizeof(x[0]))

byte p[8]={0,0,0,0,0,0,0,0};      // bitmap to display
byte nums[10][3]={                // bitmap of numbers
  {28,34,28},
  {36,62,32},
  {50,42,36},
  {42,42,20},
  {28,18,62},
  {46,42,18},
  {28,42,18},
  {50,10,6},
  {54,42,54},
  {4,42,28}
};

byte marquee1[]={0,0,0,0,0,0,0,0,0,127,9,9,6,120,4,4,56,84,84,24,0,72,84,36,0,72,84,36,0,0,0,62,68,68,56,68,68,56,0,0,72,84,36,0,62,68,68,0,116,84,120,0,120,4,4,0,62,68,68,0,0,0,0,0,0,0,0,0,0};
byte marquee2[]={0,0,0,0,0,0,70,73,49,0,56,68,68,0,56,68,68,56,0,120,4,4,0,60,84,92,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// Tunes are stored as an array of tone, duration and delay 
int tuneStartUp[][3] = {
  {600, 500, 100}, 
  {700, 500, 100}, 
  {800, 500, 100}, 
  {650, 500, 100}, 
  {750, 500, 100}, 
  {850, 500, 100}
};

int tuneFruitPickup[][3] = {
  {600, 500, 50}, 
  {700, 500, 50}, 
  {800, 500, 50}
};

int tuneGameOver[][3] = {
  {850, 500, 100}, 
  {750, 500, 100}, 
  {650, 500, 100}, 
  {800, 500, 100}, 
  {700, 500, 100}, 
  {600, 500, 100}
};

char snakeX[64];                              // snake x positions  
char snakeY[64];                              // snake y positions
char snakeLength=0;                           // snake length

long fruitX = 0;                              // fruit x position
long fruitY = 0;                              // fruit y position
int fruitScore = 5;                           // fruit score
int fruitResetCount = 10;                     // move fruit if not picked up after this many moves
int fruitMoveCount = 0;                       // number of moves since last fruit reset

char dir=0;                                   // direction: 1=up, 2=right, 3=down, 4=left
unsigned score=0;                             // player score
char gamestate=0;                             // 0=idle, 1=play, 2=game over
unsigned long menuCycleSpeed=80;              // time between game cycles: menu speed       
unsigned long initialSpeed = 400;             // time between game cycles: snake initial speed
unsigned long gameCycleSpeed=initialSpeed;    // time between game cycles: snake progressive speed
int speedUp=20;                               // snake speed-up

int moveUp = 1;                               // movement identifiers
int moveLeft = 2;
int moveDown = 3;
int moveRight = 4;

int toleranceLeft=256;                        // movement analog tolerances (256,768)
int toleranceRight=768;
int toleranceUp=256;
int toleranceDown=768;

int tension=0;                                // tension tone : more fruit, more tension
int tensionTone=20;

const char gameStateIdle = 0;
const char gameStatePlaying = 1;
const char gameStateGameOver = 2;
const char gameStateGameEnd = 3;

void setup() {
  Serial.begin(9600);
  MAX7219init();
  MAX7219brightness(1);
  pinMode(BUTTON,INPUT_PULLUP);
  pinMode(SPEAKER,OUTPUT);
  playtune(tuneStartUp, ELEMENTS(tuneStartUp));          // play startup tune
  randomSeed(analogRead(5));      // randomization
}

void loop() {
  switch(gamestate){
    case gameStateIdle: doidle(); break;
    case gameStatePlaying: dogame(); break;
    case gameStateGameOver: dogameover(); break;
    case gameStateGameEnd: dogameend(); break;
  }
  
  delay(gamestate==gameStatePlaying ? gameCycleSpeed : menuCycleSpeed);
}

void dogameend(){
  gamestate=gameStateGameOver;
  playtune(tuneGameOver, ELEMENTS(tuneGameOver));                    // play end tune
}

void dogameover(){
  static int n=0;
  marquee2[30]=nums[(score/100)%10][0];
  marquee2[31]=nums[(score/100)%10][1];
  marquee2[32]=nums[(score/100)%10][2];
  marquee2[34]=nums[(score/10)%10][0];
  marquee2[35]=nums[(score/10)%10][1];
  marquee2[36]=nums[(score/10)%10][2];
  marquee2[38]=nums[(score/1)%10][0];
  marquee2[39]=nums[(score/1)%10][1];
  marquee2[40]=nums[(score/1)%10][2];
  MAX7219sendbm(marquee2+n);
  n++;
  if(n>sizeof(marquee2)-8){n=0;}
  if(digitalRead(BUTTON)==LOW){           // change modes
    gamestate=gameStateIdle;
    MAX7219sendbm(marquee1);              // blank screen
    while(digitalRead(BUTTON)==LOW){}     // wait til released
  }
}

void dogame(){
  int i;                // loop variable
  char newSnakeX,newSnakeY;
  int a;
  bool moveDetected = false;
  
  if(snakeLength==0){
            
    score=0;                        // game is reset > initialise score 
    dir=moveUp;                     // set initial direction
    gameCycleSpeed=initialSpeed;    // set initial speed
    fruitMoveCount=0;               // reset fruit move count
    tension=0;                      // reset tension
        
    snakeX[0]=3;                    // set initial snake positions
    snakeY[0]=6;
    snakeX[1]=3;
    snakeY[1]=7;
    
    snakeLength=2;                  // set initial snake length

    setFruitPosition();
  }

  // Do we need to move the fruit?
  fruitMoveCount++;

  if(fruitMoveCount==fruitResetCount)
  {
    setFruitPosition();
    fruitMoveCount=0;
  }

  moveDetected = false;

  a=analogRead(JOYX);
  if(a<toleranceLeft && dir!=moveRight && !moveDetected){
    dir=moveLeft;
    moveDetected = true;
  }
  
  if(a>toleranceRight && dir!=moveLeft && !moveDetected){
    dir=moveRight;
    moveDetected = true;
  }

  a=analogRead(JOYY);
  if(a<toleranceUp && dir!=moveDown && !moveDetected){
    dir=moveUp;
    moveDetected = true;
  }
  
  if(a>toleranceDown && dir!=moveUp && !moveDetected){
    dir=moveDown;
  }
  
  //new snake head position
  newSnakeX=snakeX[0];
  newSnakeY=snakeY[0];     
  
  switch(dir)
  {
    case 1: 
      newSnakeY=newSnakeY-1;  // move up
      break;
    
    case 2: 
      newSnakeX=newSnakeX+1;  // move right
      break;
    
    case 3: 
      newSnakeY=newSnakeY+1;  // move down
      break;
    
    case 4: 
      newSnakeX=newSnakeX-1;  // move left
      break;
  }

  if((newSnakeX<0)||(newSnakeX>7)||(newSnakeY<0)||(newSnakeY>7))
  {     
    //outside walls > game over
    gamestate=gameStateGameEnd;
  }

  for(i=0;i<snakeLength;i++)
  {
    if((newSnakeX==snakeX[i])&&(newSnakeY==snakeY[i]))
    {
      //collided with self > game over
      gamestate=gameStateGameEnd;
    }  
  }
  
  if((newSnakeX==fruitX)&&(newSnakeY==fruitY))
  {
    // Picking up fruit

    // Play happy tune
    playtune(tuneFruitPickup, ELEMENTS(tuneFruitPickup));

    // Increase score
    score=score+fruitScore;

    // Set new fruit position
    setFruitPosition();

    // Reset fruit move count
    fruitMoveCount=0;

    // Speed up, increase tension
    if(gameCycleSpeed-speedUp > 0)
    {
      gameCycleSpeed=gameCycleSpeed-speedUp;
      tension = tension + tensionTone;
    }
  }

  //move old positions
  for(i=63;i>0;i--)
  {                             
    snakeX[i]=snakeX[i-1];
    snakeY[i]=snakeY[i-1];
  }

  snakeX[0]=newSnakeX;
  snakeY[0]=newSnakeY;

  //clear display
  for(i=0;i<8;i++)
  {
    p[i]=0;
  }

  //draw snake
  for(i=0;i<snakeLength;i++)
  {                        
    p[snakeX[i]]=p[snakeX[i]]|(1<<(snakeY[i]));
  }

  //draw fruit
  p[fruitX] = p[fruitX]|(1<<fruitY);

  MAX7219sendbm(p);
  score++;                          //increase score
  snakeLength=(score+50)/25;        //increase length
  tone(SPEAKER,440 + tension,10);   //chirp for game rhythm
}

void setFruitPosition()
{
  bool fruitOnSnake = false;
    
  do
  {
    int i=0;
    fruitOnSnake = false;

    //place fruit
    fruitX = random(7);
    fruitY = random(7);

    // are we placing this fruit on the snake?
    // if we are, then we need to try placing it again
    for(i=0;i<snakeLength;i++)
    {
      if((fruitX==snakeX[i])&&(fruitY==snakeY[i]))
      {
        fruitOnSnake=true;
      }  
    }
  } while ( fruitOnSnake==true );
}

void playtune(int tune[][3], int count)
{
  int i = 0;
  for(int i = 0; i < count; i++)
  {
    tone(SPEAKER, tune[i][0],tune[i][1]);
    delay(tune[i][2]);
  }
}

void doidle(){
  static int n=0;
  MAX7219sendbm(marquee1+n);
  n++;
  if(n>sizeof(marquee1)-8){n=0;}
  if(digitalRead(BUTTON)==LOW){         //change modes,
    gamestate=gameStatePlaying;
    MAX7219sendbm(marquee1);            //blank screen
    snakeLength=0;                      //reset game
    while(digitalRead(BUTTON)==LOW){}   // wait til released
  }
}

void MAX7219shown(byte n){
  byte s[8];
  s[0]=nums[(n/10)%10][0];
  s[1]=nums[(n/10)%10][1];
  s[2]=nums[(n/10)%10][2];
  s[3]=0;
  s[4]=nums[n%10][0];
  s[5]=nums[n%10][1];
  s[6]=nums[n%10][2];
  s[7]=0;
  MAX7219sendbm(s);
}

void MAX7219sendbm(byte p[]){
  for(int i=0;i<8;i++){
    MAX7219senddata(i+1,p[i]);
  }
}

void MAX7219brightness(byte b){   //0-15 is range high nybble is ignored
  MAX7219senddata(10,b);          //intensity  
}

void MAX7219init(){
  pinMode(MAX7219DIN,OUTPUT);
  pinMode(MAX7219CS,OUTPUT);
  pinMode(MAX7219CLK,OUTPUT);
  digitalWrite(MAX7219CS,HIGH);   //CS off
  digitalWrite(MAX7219CLK,LOW);   //CLK low
  MAX7219senddata(15,0);          //test mode off
  MAX7219senddata(12,1);          //display on
  MAX7219senddata(9,0);           //no decode
  MAX7219senddata(11,7);          //scan all
  for(int i=1;i<9;i++){
    MAX7219senddata(i,0);         //blank all
  }
}

void MAX7219senddata(byte reg, byte data){
  digitalWrite(MAX7219CS,LOW);    //CS on
  for(int i=128;i>0;i=i>>1){
    if(i&reg){
      digitalWrite(MAX7219DIN,HIGH);
    }else{
      digitalWrite(MAX7219DIN,LOW);      
    }
  digitalWrite(MAX7219CLK,HIGH);   
  digitalWrite(MAX7219CLK,LOW);   //CLK toggle    
  }
  for(int i=128;i>0;i=i>>1){
    if(i&data){
      digitalWrite(MAX7219DIN,HIGH);
    }else{
      digitalWrite(MAX7219DIN,LOW);      
    }
  digitalWrite(MAX7219CLK,HIGH);   
  digitalWrite(MAX7219CLK,LOW);   //CLK toggle    
  }
  digitalWrite(MAX7219CS,HIGH);   //CS off
}

