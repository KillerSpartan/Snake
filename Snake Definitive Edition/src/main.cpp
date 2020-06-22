
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define EEPROM_SIZE 50
#define BUTTON_PIN_BITMASK 0x200000000

RTC_DATA_ATTR int bootCount = 0;



char incomingByte;
int scoreX = 14;
int aux=0;
int sel;
char P1Name[5];
char P2Name[5];
char P3Name[5];

byte mySnake[8][8] =
{
{ B00000,
  B00000,
  B00011,
  B00110,
  B01100,
  B11000,
  B00000,
},
{ B00000,
  B11000,
  B11110,
  B00011,
  B00001,
  B00000,
  B00000,
},
{ B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B01110,
},
{ B00000,
  B00000,
  B00011,
  B01111,
  B11000,
  B00000,
  B00000,
},
{ B00000,
  B11100,
  B11111,
  B00001,
  B00000,
  B00000,
  B00000,
},
{ B00000,
  B00000,
  B00000,
  B11000,
  B01101,
  B00111,
  B00000,
},
{ B00000,
  B00000,
  B01110,
  B11011,
  B11111,
  B01110,
  B00000,
},
{ B00000,
  B00000,
  B00000,
  B01000,
  B10000,
  B01000,
  B00000,
}
};


//Pausa el juego función
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

//~ boolean levelz[5][2][9] = {
//~ {{false,false,false,false,false,false,false,false,true},
//~ {false,false,false,false,false,false,false,false,true}},

//~ {{false,false,true,false,false,false,true,false,false},
//~ {true,false,false,false,true,false,false,false,false}},

//~ {{true,false,false,false,false,false,false,false,true},
//~ {true,false,false,false,false,false,false,false,true}},

//~ {{true,false,true,false,false,false,false,false,false},
//~ {false,false,false,false,true,false,false,true,false}}
//~ };

int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);

boolean levelz[5][4][20] = {
{{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false}},

{{true,true,true,true,true,true,true,true,true,true,true,true,true,true,false,false,false,false,false,false},
{true,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{true,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{true,true,true,true,true,true,true,true,true,true,true,true,true,true,false,false,false,false,false,false}},

{{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false}},

{{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false}},
};

unsigned long time_p, timeNow;
int gameSpeed;
boolean skip, gameOver, gameStarted, gamePause;
int olddir;
int selectedLevel,levels;

int key=-1;
int oldkey=-1;

boolean x[32][100];
byte myChar[8];
byte nullChar[8] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 };
boolean special;

struct partdef
{
  int row,column,dir; //0 - up, 1 - down, 2 - right, 3 - left
  struct partdef *next;
};
typedef partdef part;

part *head, *tail;
int i,j,collected;
long pc,pr;

void drawMatrix()
{

  int cc=0;
  if (!gameOver)
  {
  x[pr][pc] = true;
  //for (i=0;i<8;i++) lcd.createChar(i, nullChar);
  for(int r=0;r<4;r++)
  {
    for(int c=0;c<14;c++)
    {
      special = false;
      for(int i=0;i<8;i++)
      {
        byte b=B00000;
        if (x[r*8+i][c*5+0]) {b+=B10000; special = true;}
        if (x[r*8+i][c*5+1]) {b+=B01000; special = true;}
        if (x[r*8+i][c*5+2]) {b+=B00100; special = true;}
        if (x[r*8+i][c*5+3]) {b+=B00010; special = true;}
        if (x[r*8+i][c*5+4]) {b+=B00001; special = true;}
        myChar[i] = b;
      }
      if (special)
      {
        lcd.createChar(cc, myChar);
        lcd.setCursor(c,r);
        lcd.write(byte(cc));
  cc++;
      }
      else
      {
        lcd.setCursor(c,r);
        if (levelz[selectedLevel][r][c]) lcd.write(255);
        else lcd.write(254);
      }
    }
  }
  }
}

void freeList()
{
  part *p,*q;
  p = tail;
  while (p!=NULL)
  {
    q = p;
    p = p->next;
    free(q);
  }
  head = tail = NULL;
}

void gameOverFunction()
{
  delay(1000);
  lcd.clear();
  freeList();
  lcd.setCursor(5,1);
  lcd.print("Game Over");
  lcd.setCursor(5,2);
  lcd.print("Score: ");
  lcd.print(collected);
  delay(1000);
}

void pauseGame(){
  delay(300);
  lcd.clear();
  lcd.setCursor(6,3);
  lcd.print("Pause");

  lcd.setCursor(0, 1);
  lcd.print(EEPROM.read(0));
  lcd.setCursor(0, 0);
  lcd.print(EEPROM.readString(1));

  lcd.setCursor(12, 0);
  lcd.print(EEPROM.readString(15));
  lcd.setCursor(12, 1);
  lcd.print(EEPROM.read(14));

  delay(300);
}
void growSnake()
{
  part *p;
  p = (part*)malloc(sizeof(part));
  p->row = tail->row;
  p->column = tail->column;
  p->dir = tail->dir;
  p->next = tail;
  tail = p;
}

void newPoint()
{

  part *p;
  p = tail;
  boolean newp = true;
  while (newp)
  {
    pr = random(32);
    pc = random(70);
    newp = false;
    if (levelz[selectedLevel][pr / 8][pc / 5]) newp=true;
    while (p->next != NULL && !newp)
    {
      if (p->row == pr && p->column == pc) newp = true;
      p = p->next;
    }
  }

  if (collected < 99 && gameStarted) growSnake();
}

void moveHead()
{
  switch(head->dir) // 1 step in direction
  {
    case 0: head->row--; break;
    case 1: head->row++; break;
    case 2: head->column++; break;
    case 3: head->column--; break;
    default : break;
  }
  if (head->column >= 65) head->column = 0;
  if (head->column < 0) head->column = 64;
  if (head->row >= 32) head->row = 0;
  if (head->row < 0) head->row = 31;

  if (levelz[selectedLevel][head->row / 8][head->column / 5]) gameOver = true; // wall collision check

  part *p;
  p = tail;
  while (p != head && !gameOver) // self collision
  {
    if (p->row == head->row && p->column == head->column) gameOver = true;
    p = p->next;
  }
  if (gameOver)
    gameOverFunction();
  else
  {
  x[head->row][head->column] = true;

  if (head->row == pr && head->column == pc) // point pickup check
  {
    collected++;
    if (gameSpeed < 50) gameSpeed+=3;
    newPoint();
  }
  }
}

void moveAll()
{
  part *p;
  p = tail;
  x[p->row][p->column] = false;
  while (p->next != NULL)
  {
    p->row = p->next->row;
    p->column = p->next->column;
    p->dir = p->next->dir;
    p = p->next;
  }
  moveHead();
}

void createSnake(int n) // n = size of snake
{
  for (i=0;i<32;i++)
    for (j=0;j<100;j++)
      x[i][j] = false;

  part *p, *q;
  tail = (part*)malloc(sizeof(part));
  tail->row = 10;
  tail->column = 39 + n/2;
  tail->dir = 3;
  q = tail;
  x[tail->row][tail->column] = true;
  for (i = 0; i < n-1; i++) // build snake from tail to head
  {
    p = (part*)malloc(sizeof(part));
    p->row = q->row;
    p->column = q->column - 1; //initial snake id placed horizoltally
    x[p->row][p->column] = true;
    p->dir = q->dir;
    q->next = p;
    q = p;
  }
  if (n>1)
  {
    p->next = NULL;
    head  = p;
  }
  else
  {
    tail->next = NULL;
    head = tail;
  }
}


void bestScores(int gameScore, char name[5]){
  //Saitama
if((gameScore > EEPROM.read(0))){
    EEPROM.write(0, gameScore);
    EEPROM.writeString(1, name);
    EEPROM.commit();

  }



//~ //Goku
  //~ if((gameScore < EEPROM.read(0))&&(gameScore > EEPROM.read(2))&&(gameScore > EEPROM.read(4))){
      //~ EEPROM.write(2, gameScore);
      //~ EEPROM.commit();
      //~ EEPROM.writeString(3, name);
      //~ EEPROM.commit();
    //~ }

//~ //Naruto
    //~ if((gameScore < EEPROM.read(0))&&(gameScore < EEPROM.read(2))&&(gameScore > EEPROM.read(4))){
        //~ EEPROM.write(4, gameScore);
        //~ EEPROM.commit();
        //~ EEPROM.writeString(5, name);
        //~ EEPROM.commit();
      //~ }

}


void startF()
{
  gameOver = false;
  gameStarted = false;
  gamePause = false;
  selectedLevel = 1;
  EEPROM.writeString(15, "Pepe");
  EEPROM.write(14, 30);
  EEPROM.commit();


  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("The Snake");
  lcd.setCursor(1, 1);
  lcd.print("Definitive Edition");

  lcd.setCursor(4,3);
  lcd.print("Level: 1");

  collected = 60;
  gameSpeed = 8;
  createSnake(7);  //Esta función crea el tamaño inicial de Snake
  time_p = 0;
  sel = 1;
}

int playerNames(char asci, int n){

  switch(n){
    case 1:
      lcd.setCursor(aux,1);
      lcd.write(asci);
      P1Name[aux] = asci;
      aux++;
      if(asci==13){P1Name[aux-1]=' ';aux=0; Serial.print(P1Name); sel=2;}
    break;
  }
      return 0;
}

int get_key(char input)
{
    int k;
    switch (input) {
      case 'l':
        k = 4;
      break;

      case 'w':
        k = 1;
      break;

      case 's':
        k = 2;
      break;

      case 'a':
        k = 3;
      break;

      case 'd':
        k = 0;
      break;

      default:
      k = -1;  // No valid key pressed
      break;
    }

    return k;
}

void setup()
{
  levels = 5; //number of lvls
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  startF();


}

void loop()
{


  if (!gameOver && !gameStarted)
  {
    if(Serial.available() > 0) {
        incomingByte = Serial.read();
        Serial.print("I received: "); Serial.println(incomingByte);
         delay(10);
      }
   key = get_key(incomingByte);  // convert into key press
   if (key != oldkey)   // if keypress is detected
   {
     delay(50);  // wait for debounce time
     key = get_key(incomingByte);    // convert into key press
     if (key != oldkey)
     {
       oldkey = key;
       if (key >=0)
       {
         olddir = head->dir;
         if (key==1 && selectedLevel<levels) selectedLevel++;
         if (key==2 && selectedLevel>1) selectedLevel--;
         if (key==4)
         {
           lcd.clear();
           selectedLevel--;
           newPoint();
           gameStarted = true;
         }
         else
         {
           lcd.setCursor(11,3);
           lcd.print(selectedLevel);
         }
       }
     }
   }
  }

  if (!gameOver && gameStarted && !gamePause)
  {

    if(sel==1){
      lcd.setCursor(0,0);
      lcd.print("Player");
      if(Serial.available() > 0) {incomingByte = Serial.read();  playerNames(incomingByte,1); delay(10);}
      lcd.setCursor(18, 1);
      lcd.print(EEPROM.read(0));
      lcd.setCursor(15, 0);
      lcd.print(EEPROM.readString(1));
      lcd.setCursor(16,2);
      lcd.print("Best");

    }


    if(sel==2){
      lcd.clear();
      delay(300);
      sel =3;
    }

    if(sel == 3){



      if(Serial.available() > 0) {

          incomingByte = Serial.read();
          Serial.print("I received: "); Serial.println(incomingByte);
          delay(10);
          if(incomingByte == 'l') gamePause = true;
        }

        lcd.setCursor(14,0);
        lcd.print("Player");
        lcd.setCursor(14,1);
        lcd.print(P1Name);
        lcd.setCursor(14, 2);
        lcd.print("Score");
        lcd.setCursor(14, 3);
        lcd.print(collected);

     skip = false; //skip the second moveAll() function call if the first was made
     key = get_key(incomingByte);  // convert into key press
     if (key != oldkey)   // if keypress is detected
     {
       delay(50);  // wait for debounce time
       key = get_key(incomingByte);    // convert into key press
       if (key != oldkey)
       {
         oldkey = key;
         if (key >=0)
         {
           olddir = head->dir;
           if (key==0 && head->dir!=3) head->dir = 2;
           if (key==1 && head->dir!=1) head->dir = 0;
           if (key==2 && head->dir!=0) head->dir = 1;
           if (key==3 && head->dir!=2) head->dir = 3;

           if (olddir != head->dir)
           {
             skip = true;
             delay(1000/gameSpeed);
             moveAll();
             drawMatrix();
           }
         }
       }
     }

 if (!skip)
 {
   timeNow = millis();
   if ((timeNow - time_p)>(1000/gameSpeed))
   {
     moveAll();
     drawMatrix();
     time_p = millis();
   }
 }
}




}

if(!gameOver && gameStarted && gamePause){
  if(Serial.available() > 0) {
      incomingByte = Serial.read();
      Serial.print("I received: "); Serial.println(incomingByte);
      if(incomingByte == 'l') {gamePause = false; sel=3;}
       delay(10);
    }
  pauseGame();}

  if(gameOver)
  {
    if(Serial.available() > 0) {
        incomingByte = Serial.read();
        Serial.print("I received: "); Serial.println(incomingByte);
         delay(10);
      }

  bestScores(collected, P1Name);
   key = get_key(incomingByte);  // convert into key press
   if (key != oldkey)   // if keypress is detected
   {
     delay(50);  // wait for debounce time
     key = get_key(incomingByte);    // convert into key press
     if (key != oldkey)
     {
       oldkey = key;
       if (key >=0)
       {
          startF();
       }
     }
   }

 }



}



/*
void printChar()
{
  Serial.println();
  Serial.println();
  Serial.println();
  for (i=0;i<8;i++)
  {
    Serial.println(myChar[i]);
  }
}
void printMatrix()
{
  Serial.println();
  Serial.println();
  Serial.println();
  for (i=0;i<16;i++)
  {
    for (j=0;j<80;j++)
    {
      Serial.print(x[i][j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}
*/
