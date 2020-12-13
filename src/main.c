#include <genesis.h>
#include <resources.h>

#define hard_reset() __asm__("move   #0x2700,%sr\n\t" \
                             "move.l (0),%a7\n\t"     \
                             "jmp    _hard_reset")

#define enable_ints __asm__("move #0x2500,%sr")
#define disable_ints __asm__("move #0x2700,%sr")

#define CELL(I,J) (field[size*(I)+(J)])
#define ALIVE(I,J) t[size*(I)+(J)] = 1
#define DEAD(I,J)  t[size*(I)+(J)] = 0


int count_alive(const char *field, int i, int j, int size)
{
   int x, y, a = 0;
   for(x=i-1; x <= (i+1) ; x++)
   {
      for(y=j-1; y <= (j+1) ; y++)
      {
         if ( (x==i) && (y==j) ) continue;
         if ( (y<size) && (x<size) &&
              (x>=0)   && (y>=0) )
         {
              a += CELL(x,y);
         }
      }
   }
   return a;
}

void evolve(const char *field, char *t, int size)
{
   int i, j, alive, cs;
   for(i=0; i < size; i++)
   {
      for(j=0; j < size; j++)
      {
         alive = count_alive(field, i, j, size);
         cs = CELL(i,j);
         if ( cs )
         {
            if ( (alive > 3) || ( alive < 2 ) )
                DEAD(i,j);
            else
                ALIVE(i,j);
         } else {
            if ( alive == 3 )
                ALIVE(i,j);
            else
                DEAD(i,j);
         }
      }
   }
}

#define FIELD_SIZE 24
bool running = 0;
bool drawing = 0;

typedef struct
{
    u16 x;
    u16 y;
    char label[10];
} Option;

#define NUM_OPTIONS 3
Option options[NUM_OPTIONS] = {
    {3, 0, "DRAW"},
    {12, 0, "START"},
    {20, 0, "RESET"},
};

u8 currentIndex = 0;
u16 xIndex = 0;
u16 yIndex = 1;

Sprite *cursor;
Sprite *cross;

void updateCursorPosition(){
    SPR_setPosition(cursor, options[currentIndex].x*8-12, 0);
}
void updateCrossPosition(){
    SPR_setPosition(cross, xIndex * 8, yIndex * 8);
}
void moveUP(){
  if (drawing){
      if(yIndex > 1){
          yIndex -= 1;
          updateCrossPosition();
      }
  }
}

void moveDown(){
  if (drawing){
    if(yIndex < FIELD_SIZE){
        yIndex += 1;
        updateCrossPosition();
    }
  }
}
void moveLeft(){
  if (drawing){
    if(xIndex > 0){
        xIndex -= 1;
        updateCrossPosition();
    }
  }
  else{
    if(currentIndex > 0){
        currentIndex--;
        updateCursorPosition();
    }
  }
}

void moveRight(){
  if (drawing){
    if(xIndex < FIELD_SIZE-1){
        xIndex += 1;
        updateCrossPosition();
    }
  }
  else{
    if(currentIndex < NUM_OPTIONS-1){
        currentIndex++;
        updateCursorPosition();
    }
  }
}

void pickDraw(){
  SPR_setPosition(cross, 0, 8);
  running = 0;
  drawing = 1;
}

void pickStart(){
  SPR_setPosition(cross, -12, -12);
  running = 1;
  drawing = 0;
}

void pickReset(){
  running = 0;
  drawing = 0;
  hard_reset();
}

void select(u16 Option){
    switch (Option)
    {
    case 0:{
        pickDraw();
        break;
    }
    case 1:{
        pickStart();
        break;
    }
    case 2:{
        pickReset();
        break;
    }

    default:
        break;
    }
}

#define FIELD_GEN 160
char field[FIELD_SIZE * FIELD_SIZE];
char temp_field[FIELD_SIZE*FIELD_SIZE];

/* set the cell i,j as alive */
#define ACELL(I,J) field[FIELD_SIZE*(I)+(J)] = 1
#define KCELL(I,J) field[FIELD_SIZE*(I)+(J)] = 0


void joyEventHandler(u16 joy, u16 changed, u16 state){

  int size = FIELD_SIZE;

  if (changed & state & BUTTON_RIGHT) {
      moveRight();
  }
  else if(changed & state & BUTTON_UP) {
      moveUP();
  }
  else if(changed & state & BUTTON_DOWN) {
      moveDown();
  }
  else if(changed & state & BUTTON_LEFT) {
      moveLeft();
  }
  else if(changed & state & BUTTON_A) {
    if (drawing) {
      if (CELL(yIndex-1, xIndex)){
        KCELL(yIndex-1, xIndex);
        VDP_setTileMapXY(VDP_PLAN_A, 2, xIndex, yIndex);
      }
      else{
        ACELL(yIndex-1, xIndex);
        VDP_setTileMapXY(VDP_PLAN_A, 1, xIndex, yIndex);
      }
    }
    else {
      select(currentIndex);
    }
  }
  else if(changed & state & BUTTON_B) {
    if (drawing) {
      SPR_setPosition(cross, -12, -12);
      drawing = 0;
    }
    else if (running) {
      running = 0;
    }
  }
}


int main(u16 hard) {

    JOY_init();
    JOY_setEventHandler(&joyEventHandler);

    const u32 tile[8] = {
        0x11111111,
        0x11111111,
        0x11111111,
        0x11111111,
        0x11111111,
        0x11111111,
        0x11111111,
        0x11111111
    };


    SPR_init(0, 0, 0);
    cursor = SPR_addSprite(&gfx_cursor, 12, 0, 0);
    cross = SPR_addSprite(&gfx_cross, -12, -12, 0);


    VDP_setScreenWidth256();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);
    VDP_setPaletteColor(0, RGB24_TO_VDPCOLOR(0x000000));

    VDP_loadTileData((const u32 *)tile, 1, 1, 0);

    char *fa, *fb, *tt;

    /* fast and dirty selection option */
    for(int i=0; i < (FIELD_SIZE*FIELD_SIZE) ; i++) field[i]=0;
    /* prepare the glider */

    //Draw options
    for(int i = 0; i < NUM_OPTIONS; i++){
        Option o = options[i];
        VDP_drawText(o.label,o.x,o.y);
    }


    int x = 0;
    int y = 0;
    int z = 0;

    for (z=0; z < (FIELD_SIZE*FIELD_SIZE); z++) {
      x += 1;
      if ((z % FIELD_SIZE) == 0 ){
          y += 1;
          x = 0;
      }
      VDP_setTileMapXY(VDP_PLAN_A, 2, x, y);
    }

    VDP_drawText("- Game of life - TurBoss 2020 -", 0, 26);

    while (1){

      SPR_update();

      /* evolve */
      fa = field;
      fb = temp_field;

      if (running){
        for (int i=0; i < FIELD_GEN; i++)
        {
            int x = 0;
            int y = 0;
            int z = 0;

            for (z=0; z < (FIELD_SIZE*FIELD_SIZE); z++) {
              x += 1;
              if ((z % FIELD_SIZE) == 0 ){
                  y += 1;
                  x = 0;
              }
              if (fa[z] == 1){
                VDP_setTileMapXY(VDP_PLAN_A, 1, x, y);
              }
              else{
                VDP_setTileMapXY(VDP_PLAN_A, 2, x, y);
              }
            }
           evolve(fa, fb, FIELD_SIZE);
           tt = fb; fb = fa; fa = tt;
        }
      }
      VDP_waitVSync();
    }
    return 0;
}
