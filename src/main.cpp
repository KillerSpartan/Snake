#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

char incomingByte;
int scoreX = 14;
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

byte limitRight[] = {
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001
};

byte limitLeft[] = {
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000
};

byte limitTop[] = {
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte limitButtom[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111
};

boolean levelz[5][2][9] = {
{{false,false,false,false,false,false,false,false,true},
{false,false,false,false,false,false,false,false,true}},

{{false,false,true,false,false,false,true,false,false},
{true,false,false,false,true,false,false,false,false}},

{{true,false,false,false,false,false,false,false,true},
{true,false,false,false,false,false,false,false,true}},

{{true,false,true,false,false,false,false,false,false},
{false,false,false,false,true,false,false,true,false}}
};

/*boolean levelz[5][2][16] = {
{{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}},

{{true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true},
{true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true}},

{{true,false,false,false,true,false,false,false,false,false,false,true,false,false,false,true},
{true,false,false,false,false,false,false,false,true,false,false,false,false,false,false,true}},

{{true,false,true,false,false,false,false,false,false,true,false,false,false,true,false,false},
{false,false,false,false,true,false,false,true,false,false,false,true,false,false,false,true}}
};*/

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);

unsigned long timeNow;
unsigned long time_p;
int gameSpeed;
boolean skip, gameOver, gameStarted;
int olddir;
int selectedLevel,levels;

int key=-1;
int oldkey=-1;

boolean x[16][80];
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
  for(int r=0;r<2;r++)
  {
    for(int c=0;c<8;c++)
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
  lcd.setCursor(3,0);
  lcd.print("Game Over");
  lcd.setCursor(2,1);
  lcd.print("Score: ");
  lcd.print(collected);
  delay(1000);
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
    pr = random(16);
    pc = random(40);
    newp = false;
    if (levelz[selectedLevel][pr / 8][pc / 5]) newp=true;
    while (p->next != NULL && !newp)
    {
      if (p->row == pr && p->column == pc) newp = true;
      p = p->next;
    }
  }

  if (collected < 50 && gameStarted) growSnake();
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
  if (head->column >= 40) gameOver = true;
  if (head->column < 0) gameOver = true;
  if (head->row >= 16) gameOver = true;
  if (head->row < 0) gameOver = true;

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
    if(collected<10) gameSpeed+=1;
    if (collected >=10 && collected <20) gameSpeed+=2;
    if (collected >=20 && collected <30) gameSpeed+=3;
    if (collected >=30 && collected <40) gameSpeed+=4;
    if (gameSpeed > 40) gameSpeed+=5;

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

void createSnake(int n) // n = Tamaño de Snake
{
  for (i=0;i<8;i++)
    for (j=0;j<80;j++)
      x[i][j] = false;

  part *p, *q;
  tail = (part*)malloc(sizeof(part));
  tail->row = 7;
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

void startF()
{
  gameOver = false;
  gameStarted = false;
  selectedLevel = 1;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("The Snake V1.0");

  lcd.setCursor(0,1);
  lcd.print("Level: 1");

  collected = 0;
  gameSpeed = 8;
  createSnake(7);  //Esta función crea el tamaño inicial de Snake
  time_p = 0;
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
  Serial.begin(9600);
  startF();
  lcd.createChar(7, limitRight);
  lcd.createChar(8, limitLeft);
  lcd.createChar(9, limitTop);
  lcd.createChar(10, limitButtom);



}

void loop()
{

  if(Serial.available() > 0) {
      incomingByte = Serial.read();
      Serial.print("I received: "); Serial.println(incomingByte);
       delay(10);
    }

  if (!gameOver && !gameStarted)
  {
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
           lcd.setCursor(7,1);
           lcd.print(selectedLevel);
         }
       }
     }
   }
  }
  if (!gameOver && gameStarted)
  {

    lcd.setCursor(10, 0);
    lcd.print("Score");

    lcd.setCursor(scoreX, 1);
    lcd.print(collected);

    if(collected>=9){
      scoreX = 13;
    }

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
  if(gameOver)
  {

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
