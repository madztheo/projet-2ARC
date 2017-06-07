//this example shows how to set up a palette and use 8x8 HW sprites
//also shows how fast (or slow) C code is

#include "neslib.h"
#include "background.h"
#include "lose.h"
#include "win.h"
#include "logo.h"
#include "level1.h"
#include "level2.h"

//general purpose vars

static unsigned int i, j;
static unsigned char spr; //To keep track of our sprite
static unsigned char pad;
static unsigned char key;
//static unsigned char menu;
static unsigned char pause = FALSE;
static unsigned char frame = 0;

static unsigned char x_paddle = 100;
static const unsigned char y_paddle = 212;

static unsigned char x_ball= 116;
static unsigned char y_ball = 200;
static char x_ball_speed = -2;
static char y_ball_speed = -2;

static unsigned int score = 0;
static unsigned char scoreTab[5];

static unsigned int currentPosition = 0;
static unsigned char wallLeftPos = 0;
static unsigned char wallRightPos = 0;
static unsigned char wallTopPos = 0;

static unsigned char update_list[6];

static unsigned char currentLevelNb = 1;
static unsigned char isLevelFinished = FALSE;

static unsigned char canDestroyBrick = TRUE;

static unsigned char level[0x3A0];

const unsigned char paddle[]={
	0,	0,	0x41,	1, 
	8,	0,	0x42,	1,
	16,	0,	0x43,	1,
	24,	0,	0x44,	1,
	128
};

const unsigned char palSprites[16]={
	0x0f,0x17,0x27,0x37,
	0x0f,0x01,0x02,0x12,
	0x0f,0x15,0x25,0x35,
	0x0F,0x05,0x02,0x30
};

const unsigned char paletteBG[16]={ 0x0f,0x00,0x10,0x30,0x0f,0x01,0x02,0x12,0x0f,0x06,0x16,0x26,0x0f,0x09,0x19,0x29 };

const unsigned char palLogo[] = {
    0x0F,0x05,0x02,0x30
};


//put a string into the nametable

void put_str(unsigned int adr,const char *str)
{
	vram_adr(adr);

	while(1)
	{
		if(!*str) break;

        //To be changed once the alhpabet is integrated in our tileset
		vram_put((*str++)-0x21);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

void put_nb(unsigned int adr, const char nb)
{
	vram_adr(adr);
    vram_put(0x0f + nb); //Number starts at 4f in the table
}


void put_brick(unsigned int adr)
{
    vram_adr(adr);
    vram_put(0x46);
    vram_put(0x47);
}


void drawBackground(void)
{
    pal_bg(paletteBG);
    vram_adr(NAMETABLE_A);
    vram_unrle(background);
}

void clearScreen(void)
{
    vram_adr(NAMETABLE_A);
    vram_fill(0x50, 0x3A0); 
}


void updateScore(unsigned char valueToAdd)
{
    score += valueToAdd;
    scoreTab[0] = score%10;
    scoreTab[1] = score >= 10 ? score/10%10 : 0;
    scoreTab[2] = score >= 100 ? score/100%10 : 0;
    scoreTab[3] = score >= 1000 ? score/1000%10 : 0;
    scoreTab[4] = score >= 10000 ? score/10000 : 0;
}

void printScore(void)
{
    spr=oam_spr(20,16,'S'- 0x21,3,spr);
    spr=oam_spr(28,16,'C'- 0x21,3,spr);
    spr=oam_spr(36,16,'O'- 0x21,3,spr);
    spr=oam_spr(44,16,'R'- 0x21,3,spr);
    spr=oam_spr(52,16,'E'- 0x21,3,spr);

    spr=oam_spr(20,24, scoreTab[4] + 0x0f,3,spr);
    spr=oam_spr(28,24, scoreTab[3] + 0x0f,3,spr);
    spr=oam_spr(36,24, scoreTab[2] + 0x0f,3,spr);
    spr=oam_spr(44,24, scoreTab[1] + 0x0f,3,spr);
    spr=oam_spr(52,24, scoreTab[0] + 0x0f,3,spr);
}


void printLevel(void)
{
    put_str(NTADR_A(20,3), "LEVEL");
    put_nb(NTADR_A(26,3), currentLevelNb);
}

void checkLevelCompletion()
{
    isLevelFinished = TRUE;
    for(i = 0; i < 0x03A0; ++i)
    {
        if(level[i] > 1)
        {
            isLevelFinished = FALSE;
            break;
        }
    }
}


void checkCollisionOfBall(void)
{
    static unsigned char xPos = 0;
    //We loop to test all the corners of the ball 
    for(i = 0; i <= 8; i += 4)
    {
        for(j = 0; j <= 8; j += 8)
        {
            currentPosition = ((x_ball+i)/ 8) + ((y_ball-j)/ 8) * 32;
            if(canDestroyBrick && level[currentPosition] > 1)
            {
                canDestroyBrick = FALSE;
                //We make the ball bounce on the brick
                y_ball_speed = -y_ball_speed; 
                //To set the update of the background with the new broken tile
                //Those fancy computations are here to make sure to get an even number for the x-axis
                
                xPos = (((x_ball+i)/8)/2) * 2;//Doing the computation directly in the NTADR_A macro doesn't work somehow

                //so as to always destroy one brick and not halves of bricks
                update_list[0] = MSB(NTADR_A(xPos, (y_ball-j)/8 + 1))|NT_UPD_HORZ; //The high byte of its address
                update_list[1] = LSB(NTADR_A(xPos, (y_ball-j)/8 + 1)); //The low byte of its address
                update_list[2] = 2; //The number of tiles to write
                update_list[3] = level[currentPosition] == 3 ? 0x48 : 0xff; //The 1st tile
                update_list[4] = level[currentPosition] == 3 ? 0x49 : 0xff; //The 2nd
                update_list[5] = NT_UPD_EOF; //The end
        
                set_vram_update(update_list); //And we set the update in the vram
                if(level[currentPosition] == 3)
                {
                    updateScore(50);//We increment the score of 50 when we damage the brick
                }
                else
                {
                    updateScore(100);//We increment the score of 100 when break the break
                }

                if(currentPosition % 2 == 0)
                {
                    level[currentPosition] -= 1;
                    level[currentPosition+1] -= 1;
                }
                else
                {
                    level[currentPosition-1] -= 1;
                    level[currentPosition] -= 1;
                }           
            }
        }
    }
 
    if(xPos != 0)
    {
        checkLevelCompletion();
    }

}


void lostScreen(void)
{
    pal_clear();
    ppu_off();

    clearScreen();

    pal_bg(palLogo);
    vram_adr(NAMETABLE_A);
    vram_unrle(lose);

    ppu_on_all();

	while(1)
	{
	    ppu_wait_nmi();

		key=pad_trigger(0);

		//We wait that the user presses A (F on a PC)
		if(key&PAD_A){
            pal_clear(); //Set every color to black, hence hide everything
            clearScreen();
            ppu_off();
            break;
		}
	}
}

void victoryScreen(void)
{
    pal_clear();
    ppu_off();

    clearScreen();

    pal_bg(palLogo);

    vram_adr(NAMETABLE_A);
    vram_unrle(win);

    ppu_on_all();

	while(1)
	{
	    ppu_wait_nmi();

		key=pad_trigger(0);

		//We wait that the user presses A (F on a PC)
		if(key&PAD_A){
            pal_clear(); //Set every color to black, hence hide everything
            clearScreen();
            ppu_off();
            break;
		}
	}
}


void home(void){

    ppu_off();
    pal_clear();

    clearScreen();

    pal_bg(palLogo);

    vram_adr(NAMETABLE_A);
    vram_unrle(logo);

	ppu_on_all();

	while(1)
	{
	    ppu_wait_nmi();

		key=pad_trigger(0);

		//We wait that the user presses A (F on a PC)
		if(key&PAD_A){
            sfx_play(0,0);
            pal_clear(); //Set every color to black, hence hide everything
            clearScreen();
            ppu_off();
            break;
		}
	}
}



char move_ball(void)
{
    spr=oam_spr(x_ball,y_ball,0x45,1,spr);//0x45 is tile number, 1 is palette
    if(canDestroyBrick != TRUE && frame > 2)
    {
        frame = 0;
        canDestroyBrick = TRUE;
    }
    else if(canDestroyBrick != TRUE)
    {
        frame++;
    }

    //move the ball

    checkCollisionOfBall();

    if(!(x_ball + 8 < x_paddle || x_ball > x_paddle + 40
    || y_ball + 6 < y_paddle))
    {
        if(x_ball + 4 >= x_paddle + 16)
        {
            x_ball_speed = -(((x_paddle + 16) - (x_ball + 4))/16);
        }
        else
        {
            x_ball_speed = ((x_ball + 4) - (x_paddle + 16))/16;
        }
        y_ball_speed = -y_ball_speed;
    } else {
        if(x_ball>=wallRightPos-8)
        {
            x_ball_speed = -x_ball_speed;
            x_ball = wallRightPos - 10;
        } 
        else if(x_ball<=wallLeftPos)
        {
             x_ball_speed = -x_ball_speed;
             x_ball = wallLeftPos + 2;
        }
        else if(y_ball <= wallTopPos)
        {
            y_ball_speed = -y_ball_speed;
            y_ball = wallTopPos + 2;
        } 
        else if(y_ball>=(230-8))
        {
            return FALSE;
        }
    }
    x_ball += x_ball_speed;
    y_ball += y_ball_speed;
    return TRUE;

}

void move_paddle(void)
{

    spr=oam_meta_spr(x_paddle,y_paddle,spr,paddle);

    //poll pad and change coordinates
    //This is a single controller game, so 0 (1st controller) will do just fine
    pad=pad_poll(0);

    if(pad&PAD_LEFT && x_paddle > wallLeftPos)
    {
        x_paddle-=4;
    } 
    if(pad&PAD_RIGHT && x_paddle < wallRightPos - 32)
    {
        x_paddle+=4;
    } 
    
}

void generateLevel(void)
{
    if(currentLevelNb == 1)
    {
        memcpy(level, level1, sizeof(level1));
    }
    else if(currentLevelNb == 2)
    {
        memcpy(level, level2, sizeof(level2));
    }

    wallLeftPos = 2 * 8;
    wallRightPos = 256 - (2 * 8);
    wallTopPos = 2 * 8;
    for(i = 0; i < 0x3A0; i += 2)
    {
        if(level[i] == 3)
        {
            put_brick(NTADR_A(i%32, i/32+1));
        }
        else if(level[i] == 1)
        {
            vram_adr(NTADR_A(i%32, i/32+1));
            vram_put(0xff);
            vram_put(0xff);
        }
    }
}

void showNextLevel(void)
{
    pal_clear();

    clearScreen();

    pal_col(1, 0x05);

    put_str(NTADR_A(5,13), "GET READY FOR LEVEL");
    put_nb(NTADR_A(25,13), currentLevelNb);

    ppu_on_all();

    delay(120);

    ppu_off();
}


void goToNextLevel(void)
{   
    
    ppu_off();
    currentLevelNb++;
    showNextLevel();
    spr = 0;
    canDestroyBrick = FALSE;
    set_vram_update(NULL);
    isLevelFinished = FALSE;
    x_paddle = 100;
    x_ball= 116;
    y_ball = 200;
    x_ball_speed = -2;
    y_ball_speed = -2;
    frame = 0;

    pal_spr(palSprites);

    drawBackground();
    
    generateLevel();

    printLevel();

    spr=oam_meta_spr(x_paddle,y_paddle,spr,paddle);
    spr=oam_spr(x_ball,y_ball,0x45,1,spr);

    ppu_on_all();
    canDestroyBrick = TRUE;
    //delay(60);//Wait 60 frames, so pretty much 1 seconds
}



void main(void)
{
    home();
    
    while(1)
    {
        ppu_off();

        showNextLevel();

        drawBackground();

        pal_spr(palSprites);//set palette for sprites

        generateLevel();

        printLevel();

        ppu_on_all();//enable rendering

        //now the main loop
    
        while(1)
        {

            ppu_wait_nmi();//wait for next TV frame

            set_vram_update(NULL);

            spr = 0;
            key=pad_trigger(0);

            //To pause when the user presses START (Enter on a PC)
            if(key&PAD_START && !pause){
                pause = TRUE;
            } else if(key&PAD_START && pause){
                pause = FALSE;
            } else if(pause){
                continue;
            }

            if(!move_ball())
            {
                lostScreen();
                break;
            }
            
            move_paddle();

            printScore();

            if(isLevelFinished)
            {
                delay(30);
                if(currentLevelNb >= 2)
                {
                    victoryScreen();
                    break;
                }

                goToNextLevel();

                continue;
            }

        }

        canDestroyBrick = TRUE;
        isLevelFinished = FALSE;
        x_paddle = 100;
        x_ball= 116;
        y_ball = 200;
        x_ball_speed = -2;
        y_ball_speed = -2;
        frame = 0;
        score = 0;
        updateScore(0);
        currentLevelNb = 1;
    }

}