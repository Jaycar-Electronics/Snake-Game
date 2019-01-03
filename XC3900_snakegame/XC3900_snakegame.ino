#define MAX7219DIN 4
#define MAX7219CS 5
#define MAX7219CLK 6
#define JOYX A1
#define JOYY A0
#define BUTTON 2
#define SPEAKER 3

byte p[8]={0,0,0,0,0,0,0,0};      //bitmap to display
byte nums[10][3]={                   //bitmap of numbers
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

byte marquee1[]={0,0,0,0,0,0,0,0,0,127,9,9,6,120,4,4,56,84,84,24,0,72,84,36,0,72,84,36,0,0, 0,  62, 68, 68, 56, 68, 68, 56, 0,  0,  72, 84, 36, 0,  62, 68, 68, 0,  116,  84, 120,  0,  120,  4,  4,  0,  62, 68, 68, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
byte marquee2[]={0,0,0,0,0,0,70,73,49,0,56,68,68,0,56,68,68,56,0,120,4,4,0,60,84,92,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

char sx[64];                        
char sy[64];                      //snake positions, zeroth is front of snake, first slen are valid
char dir=0;                       //1=up, 2=right, 3=down, 4=left
char slen=0;
int score=0;
char gamestate=0;                 //0=idle, 1=play, 2 =game over
unsigned long tc=80; //menu             //time between game cycles (~speed/difficulty)
unsigned long ts=250;//snake
void setup() {
  MAX7219init();
  MAX7219brightness(1);
  pinMode(BUTTON,INPUT_PULLUP);
  pinMode(SPEAKER,OUTPUT);
}

void loop() {
  switch(gamestate){
    case 0: doidle(); break;
    case 1: dogame(); break;
    case 2: dogameover(); break;
    case 3: dogameend(); break;
  }
  
  delay(gamestate ==1 ? ts : tc);
}

void dogameend(){
  gamestate=2;
  tone(SPEAKER,100,500);
}

void dogameover(){
  //tc=80;
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
  if(digitalRead(BUTTON)==LOW){   //change modes,
    gamestate=0;
    MAX7219sendbm(marquee1);        //blank screen
    while(digitalRead(BUTTON)==LOW){}// wait til released
  }

}

void dogame(){
  int i;                //loop variable
  char newx,newy;
  int a;
  if(slen==0){        //game is reset > initialise
    score=0;
    dir=1;
    sx[0]=3;
    sy[0]=3;
    sx[1]=3;
    sy[1]=4;
    sx[2]=3;
    sy[2]=5;
    slen=3;    
  }
  newx=sx[0];
  newy=sy[0];     //new snake head position
  a=analogRead(JOYX);
  if(a<256){dir=2;}
  if(a>768){dir=4;}
  a=analogRead(JOYY);
  if(a<256){dir=1;}
  if(a>768){dir=3;}
  switch(dir){
    case 1: newy=newy-1;break;
    case 2: newx=newx+1;break;
    case 3: newy=newy+1;break;
    case 4: newx=newx-1;break;
  }
  if((newx<0)||(newx>7)||(newy<0)||(newy>7)){     //outside walls > game over
    gamestate=3;
  }
  for(i=0;i<slen;i++){
    if((newx==sx[i])&&(newy==sy[i])){gamestate=3;}  //collided with self > game over
  }
  for(i=63;i>0;i--){                             //move old positions
    sx[i]=sx[i-1];
    sy[i]=sy[i-1];
  }
  sx[0]=newx;
  sy[0]=newy;
  for(i=0;i<8;i++){p[i]=0;}                   //clear display
  for(i=0;i<slen;i++){                        //draw snake
    p[sx[i]]=p[sx[i]]|(1<<(sy[i]));
  }
  
  MAX7219sendbm(p);
  score++;                //increase score
  slen=(score+50)/25;    //increase length
  tone(SPEAKER,440,10);  //chirp for game rhythm
}

void doidle(){
  //tc=80;
  static int n=0;
  MAX7219sendbm(marquee1+n);
  n++;
  if(n>sizeof(marquee1)-8){n=0;}
  if(digitalRead(BUTTON)==LOW){   //change modes,
    gamestate=1;
    MAX7219sendbm(marquee1);        //blank screen
    //tc=200;                         //slow down
    slen=0;                         //reset game
    while(digitalRead(BUTTON)==LOW){}// wait til released
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

void MAX7219brightness(byte b){  //0-15 is range high nybble is ignored
  MAX7219senddata(10,b);        //intensity  
}

void MAX7219init(){
  pinMode(MAX7219DIN,OUTPUT);
  pinMode(MAX7219CS,OUTPUT);
  pinMode(MAX7219CLK,OUTPUT);
  digitalWrite(MAX7219CS,HIGH);   //CS off
  digitalWrite(MAX7219CLK,LOW);   //CLK low
  MAX7219senddata(15,0);        //test mode off
  MAX7219senddata(12,1);        //display on
  MAX7219senddata(9,0);         //no decode
  MAX7219senddata(11,7);        //scan all
  for(int i=1;i<9;i++){
    MAX7219senddata(i,0);       //blank all
  }
}

void MAX7219senddata(byte reg, byte data){
  digitalWrite(MAX7219CS,LOW);   //CS on
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

