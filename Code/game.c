#include "neslib.h" //We import the neslib library (provided in the project's folder)
#include "background.h" //We import the rle for the background of the game (simply put the wall)
#include "lose.h" //We import the rle for the defeat screen
#include "win.h" //We import the rle for the victory screen
#include "logo.h" //We import the rle for the start screen containing the logo
#include "level1.h" //We import the homemade descriptive table of level 1
#include "level2.h" //We import the homemade descriptive table of level 2

//A lot of global variable, that's not within the usual convention, but here for this particular piece of
//technology that is the NES, we better off with global variable

//Also, considering the limitation of the NES, the variable types are being chosen very carefully and strictly here.
//Most of the time we use char because we do not need a big number (not above 255 that is), we require the use of
//int only when strictly necessary. We also use unsigned numbers to get a better range on the positive side as we
//most of the time do not deal with negative numbers.

static unsigned int i, j; //For all our loops throughout all the different function defined below
static unsigned char spr; //Point to the next sprite position to able to manage several sprite at once
static unsigned char pad; //For the user input
static unsigned char key; //For the user input (but only used for single key press)
static unsigned char pause = FALSE; //To indicate if the game is paused or not
static unsigned char frame = 0; //Frame counter use for ball collisions with bricks

static unsigned char x_paddle = 100; //The paddle's position on the x-axis 
static const unsigned char y_paddle = 212; //The paddle's position on the y-axis (constant because the user can move upwards and downwards, but only sideways on the left or right)

static unsigned char x_ball= 116; //The ball's horizontal position
static unsigned char y_ball = 200; //The ball's vertical position
static char x_ball_speed = -2; //The ball's horizontal speed (speed vector's first value to be more correct)
static char y_ball_speed = -2; //The ball's vertical speed(speed vector's second value to be more correct)

static unsigned int score = 0; //The current score 
static unsigned char scoreTab[5]; //The table version of the score (digit by digit), needed to print on the screen afterwards

static unsigned int currentPosition = 0; //The current position of the ball within the descriptive table of the current level
static unsigned char wallLeftPos = 0; //The wall's left position
static unsigned char wallRightPos = 0; //The wall's right position
static unsigned char wallTopPos = 0; //The wall's top position

static unsigned char update_list[6]; //List used to set background update for V-Blank

static unsigned char currentLevelNb = 1; //The current level
static unsigned char isLevelFinished = FALSE; //Indicate if the level has been finished by the user, i.e. that he has destroyed all the bricks of the current level.

static unsigned char canDestroyBrick = TRUE; //Indicate if a brick can be destroyed or not

static unsigned char level[0x3A0]; //Will store a copy of the current level

//A representation of the paddle fitting the metasprite description table for the neslib library
const unsigned char paddle[]={
    //First the horizontal offset of the tile, then the vertical one, the tile number and the attribute (which of the 4 parts of the palette, 0 to 3)
	0,	0,	0x41,	1, 
	8,	0,	0x42,	1,
	16,	0,	0x43,	1,
	24,	0,	0x44,	1,
	128
};

//The palette used for the sprites
const unsigned char palSprites[16]={
	0x0f,0x17,0x27,0x37, //First part (gray scales)
	0x0f,0x01,0x02,0x12, //The second part (blues)
	0x0f,0x15,0x25,0x35, //The third part (pinks)
	0x0F,0x05,0x02,0x30  //The fourth part (a mix)
};

//The palette used for the background
const unsigned char paletteBG[16]={ 
    0x0f,0x00,0x10,0x30, //The first part (gray scale)
    0x0f,0x01,0x02,0x12, //The second part (blues)
    0x0f,0x06,0x16,0x26, //The third part (reds)
    0x0f,0x09,0x19,0x29 //The fourth part (greens)
};

//The palette used for the logo, defeat and victory screen
const unsigned char palLogo[] = {
    0x0F,0x05,0x02,0x30 //Just one part here
};


//Put a string of letters (CAPS only) starting at the specified address
void put_str(unsigned int adr,const char *str)
{
	vram_adr(adr);

	while(1)
	{
        //We break when we've reached the end of the string (as we're in C, it's more a pointer of char)
		if(!*str) break;
        
        //In our tileset (tiles.chr), the ASCII symbols start at the tile 0
        //We get the number of the character in the ASCII table (easy in C, as a char is just a number)
        //We then substract 0x21 to it to get the right tile number (e.g. A is tile 0x20 and 0x41 in the ASCII table and 0x41 - 0x21 = 0x20, so we do get the right tile number)
		vram_put((*str++)-0x21); 
	}
}

//Pretty simlar to the previous function but this time for the number (only one digit)
void put_nb(unsigned int adr, const char nb)
{
	vram_adr(adr); 
    vram_put(0x0f + nb); //The numbers starts at 0f in our tile set, the computation is then trivial
}


//Put a brick starting at the specified address
void put_brick(unsigned int adr)
{
    vram_adr(adr);
    vram_put(0x46); //The left half of the brick
    vram_put(0x47); //The right half of the brick
}


//Draw the game background (the wall)
void drawBackground(void)
{
    //We set the background palette to the one we previously defined
    pal_bg(paletteBG);
    //We gonna draw on the entire screen, so we start at the beginning of the nametable A (we only use that one throughout the entire game as we don't need scrolling)
    vram_adr(NAMETABLE_A);
    //Then we unpack the rle descriptive talbe of the backgroudn to the vram so as to draw it on the screen
    vram_unrle(background);
}

//Clear the screen by putting blank tile (black) on the entire screen
void clearScreen(void)
{
    vram_adr(NAMETABLE_A);
    //0xff is the last tile and is a blank one
    vram_fill(0xff, 0x3A0);//We use vram_fill to specify the range we want to draw 0x3A0 being the entire screen
}

//Add a specified value to the score
void updateScore(unsigned char valueToAdd)
{
    score += valueToAdd;//We add the value
    //And then we put every digit in the score table
    scoreTab[0] = score%10; //The unit
    scoreTab[1] = score >= 10 ? score/10%10 : 0; //The tens
    scoreTab[2] = score >= 100 ? score/100%10 : 0; //The hundreds
    scoreTab[3] = score >= 1000 ? score/1000%10 : 0; //The thousands
    scoreTab[4] = score >= 10000 ? score/10000 : 0; //The ten of thousands
}

//Print the score as a set of consecutive sprites (for an easier update and to set the color much more easily)
void printScore(void)
{
    //We first print the SCORE label using the palette's third part
    spr=oam_spr(20,16,'S'- 0x21,3,spr); //position x, y, the tile number (same logic as put_str), attribute (color),sprite pointer
    spr=oam_spr(28,16,'C'- 0x21,3,spr);
    spr=oam_spr(36,16,'O'- 0x21,3,spr);
    spr=oam_spr(44,16,'R'- 0x21,3,spr);
    spr=oam_spr(52,16,'E'- 0x21,3,spr);

    //We then print the score under the label by getting it digit by digit thanks to scoreTab
    spr=oam_spr(20,24, scoreTab[4] + 0x0f,3,spr); //Same disposition (with the same logic as put_nb)
    spr=oam_spr(28,24, scoreTab[3] + 0x0f,3,spr);
    spr=oam_spr(36,24, scoreTab[2] + 0x0f,3,spr);
    spr=oam_spr(44,24, scoreTab[1] + 0x0f,3,spr);
    spr=oam_spr(52,24, scoreTab[0] + 0x0f,3,spr);
}

//Print the current level on the top right of the screen
void printLevel(void)
{
    //This time we use put_str and put_nb as we're going to print as a background element as it's not updated often
    put_str(NTADR_A(20,3), "LEVEL");
    put_nb(NTADR_A(26,3), currentLevelNb);
}

//Check if every bricks has been broken or not
void checkLevelCompletion()
{
    //First we assume that every brick is broken
    isLevelFinished = TRUE;
    for(i = 0; i < 0x3A0; ++i)
    {
        if(level[i] > 1)
        {
            //Then if we get to a number greater than (2 or 3), it means that there's a brick there.
            //And if there's a brick, well then the level is not finished
            isLevelFinished = FALSE;
            break;
        }
    }
}

//Check any eventual collisions between the ball and the bricks
void checkCollisionOfBall(void)
{
    //One of the rare cases where variable is declared in a function, it would have been to much out of context
    //out of the function. It needs to be declared before any other code within the function though.
    static unsigned char xPos = 0;
    //We loop to test all the corners of the ball (the ball is an entire tile large, so 8)
    for(i = 0; i <= 8; i += 4)
    {
        for(j = 0; j <= 8; j += 8)
        {
            //We get the current ball ball position within the descriptive table of the current level according to
            //its current position.
            currentPosition = ((x_ball+i)/ 8) + ((y_ball-j)/ 8) * 32;
            //We check if we can destroy the brick and if there's a brick there
            if(canDestroyBrick && level[currentPosition] > 1)
            {
                canDestroyBrick = FALSE;
                //We make the ball bounce on the brick by pushing it the other way
                y_ball_speed = -y_ball_speed; 
                //To set the update of the background with the new broken tile
                //Those fancy computations are here to make sure to get an even number for the x-axis
                //so as to always destroy one brick and not halves of bricks, as bricks are always pairs of tiles.
                xPos = (((x_ball+i)/8)/2) * 2;//Doing the computation directly in the NTADR_A macro doesn't work somehow

                //We put in the update_list all the require data that's going to be used to draw the changes during
                //the next V-Blank as we can't do it immediately because the screen is on 
                update_list[0] = MSB(NTADR_A(xPos, (y_ball-j)/8 + 1))|NT_UPD_HORZ; //The high byte of its address
                update_list[1] = LSB(NTADR_A(xPos, (y_ball-j)/8 + 1)); //The low byte of its address
                update_list[2] = 2; //The number of tiles to write
                update_list[3] = level[currentPosition] == 3 ? 0x48 : 0xff; //The 1st tile (either the cracked brick or a blank tile as it takes two shot to break a brick completely)
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

                if(currentPosition % 2 == 0)//If we hit the first half of the brick
                {
                    level[currentPosition] -= 1; //The first half of the brick
                    level[currentPosition+1] -= 1; //The second half of the brick
                }
                else//If we hit the second half of the brick
                {
                    level[currentPosition-1] -= 1; //The first half of the brick
                    level[currentPosition] -= 1; //The second half of the brick
                }           
            }
        }
    }
 
    if(xPos != 0)
    {
        //If the value of xPos has changed, meaning that the ball has touched a brick, we check whether the user
        //has broken all the bricks or not
        checkLevelCompletion();
    }

}


//Show the defeat screen, when the ball fall off the screen
void lostScreen(void)
{
    pal_clear(); //Empty the palette
    ppu_off(); //Turn off the screen to be able to render background updates

    clearScreen(); //Clear the screen

    pal_bg(palLogo); //Set the palette for the defeat screen
    //Draw the defeat screen
    vram_adr(NAMETABLE_A);
    vram_unrle(lose);

    //Set the screen back on, showing all the modifications previously made
    ppu_on_all();

	while(1)
	{
	    ppu_wait_nmi(); //Wait for the next frame 

		key=pad_trigger(0); //Get an eventual key press for the first controller

		//We wait that the user presses A (F on a PC)
		if(key&PAD_A){ //Bitwise and to search for pattern for the PAD_A within key bit by bit
            //In this case the user hit A, so proceed so as to turn off the screen and hand back the
            //process to whatever function called this function
            pal_clear(); 
            clearScreen();
            ppu_off(); //Turn off the screen
            break;
		}
	}
}

//Show the victory screen, when the user manages to complete all the levels
//Same logic as previously
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
            pal_clear();
            clearScreen();
            ppu_off();
            break;
		}
	}
}


//Show the start screen, at beginning of the game
//Same logic as previously
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
            sfx_play(0,0); //Play a sound
            pal_clear();
            clearScreen();
            ppu_off();
            break;
		}
	}
}


//Move the ball, return FALSE if the ball fell off the screen
char move_ball(void)
{
    //Update the sprite associated to the ball with the new coordinates
    spr=oam_spr(x_ball,y_ball,0x45,1,spr);//0x45 is tile number, 1 is palette
    //This condition built around the number of frames is get around a tricky issues concerning the ball collisions
    //with the bricks
    if(canDestroyBrick != TRUE && frame > 2)
    {
        frame = 0;
        canDestroyBrick = TRUE;
    }
    else if(canDestroyBrick != TRUE)
    {
        frame++;
    }

    //Check for any collissions with the bricks
    checkCollisionOfBall();

    if(!(x_ball + 8 < x_paddle || x_ball > x_paddle + 40
    || y_ball + 6 < y_paddle)) //Check if the ball has hit the paddle
    {
        if(x_ball + 4 >= x_paddle + 16) //When the ball hit the center or right part of the paddle
        {
            //We change the horizontal speed of the ball so as to move it further away to the right or in a //straight line if it is smack in the middle of the paddle
            x_ball_speed = -(((x_paddle + 16) - (x_ball + 4))/16);
        }
        else //When the ball hit the  left part of the paddle
        {
            //We change the horizontal speed of the ball so as to move it further away to the left 
            x_ball_speed = ((x_ball + 4) - (x_paddle + 16))/16;
        }
        //The vertical speed will be always the opposite of itself after the ball's collission with the paddle
        y_ball_speed = -y_ball_speed;
    } else {
        if(x_ball>=wallRightPos-8) //Check if the ball hit the right part of the wall
        {
            x_ball_speed = -x_ball_speed; //To push the ball in the other direction
            x_ball = wallRightPos - 10; //Push the ball away from the wall (8 for the ball size and 2 to make sure it's out if)
        } 
        else if(x_ball<=wallLeftPos) //Check if the ball hit the left part of the wall
        {
             x_ball_speed = -x_ball_speed; //To push the ball in the other direction
             x_ball = wallLeftPos + 2; //Push the ball away from the wall (2 to make sure it's out if)
        }
        else if(y_ball <= wallTopPos) //Check if the ball hit the upper part of the wall
        {
            y_ball_speed = -y_ball_speed; //To push the ball in the other direction
            y_ball = wallTopPos + 2; //Push the ball away from the wall (2 to make sure it's out if)
        } 
        else if(y_ball>=(230-8)) //Check if the ball has fallen off the screen
        {
            //Return FALSE, so 0, to notify the main loop
            return FALSE;
        }
    }
    //Move the ball according to its current speed
    x_ball += x_ball_speed;
    y_ball += y_ball_speed;
    return TRUE;

}

//Move the paddle according to the user input
void move_paddle(void)
{
    //We update the metasprite associated to the paddle
    spr=oam_meta_spr(x_paddle,y_paddle,spr,paddle);

    //We get the user input
    //This is a single controller game, so 0 (1st controller) will do just fine
    pad = pad_poll(0);

    if(pad&PAD_LEFT && x_paddle > wallLeftPos) //If the user pressed the left key and the paddle is not going in the wall
    {
        x_paddle-=4; //We move the paddle of 4 pixels to the left
    } 
    if(pad&PAD_RIGHT && x_paddle < wallRightPos - 32) //If the user pressed the right key and the paddle is not going in the wall
    {
        x_paddle+=4; //We move the paddle of 4 pixels to the right
    } 
    
}

//Generate the level according to the current level number
void generateLevel(void)
{
    //The level descriptive array structure is explained in the level1.h file
    if(currentLevelNb == 1) //First level
    {
        //We get the first level descriptive array and copy it into the current level array, to use it and modify it afterwards
        memcpy(level, level1, sizeof(level1));
    }
    else if(currentLevelNb == 2) //Second level
    {
        //We get the second level descriptive array and copy it into the current level array, to use it and modify it afterwards
        memcpy(level, level2, sizeof(level2));
    }

    wallLeftPos = 2 * 8;//The wall is 2 tiles wide so 16
    wallRightPos = 256 - (2 * 8); //256 is the edge of the screen
    wallTopPos = 2 * 8;
    for(i = 0; i < 0x3A0; i += 2) //0x3A0 is the amount of tiles on the screen (incrementation of 2 because bricks are pairs of tiles)
    {
        if(level[i] == 3)
        {
            //For each 3 we draw a brick
            put_brick(NTADR_A(i%32, i/32+1));
        }
        else if(level[i] == 1)
        {
            //For each 1 we draw a blank tile
            vram_adr(NTADR_A(i%32, i/32+1));
            vram_put(0xff);
            vram_put(0xff);
        }
    }
}

//Show a screen to prep the user for the next level
void showNextLevel(void)
{
    pal_clear();

    clearScreen();

    //Set the second background palette's color to a dark red that'll be the color of the text 
    pal_col(1, 0x05);

    //Print GET READY FOR LEVEL and the number of that level afterwards
    put_str(NTADR_A(5,13), "GET READY FOR LEVEL");
    put_nb(NTADR_A(25,13), currentLevelNb);

    //Turn the screen on
    ppu_on_all();

    //Wait 120 frames, to let the user at least see the screen and read the short sentence on it
    delay(120);

    //Turn off the screen
    ppu_off();
}


//Go to the next level
void goToNextLevel(void)
{   
    
    ppu_off();//Turn off the screen
    currentLevelNb++; //Increment the current level
    showNextLevel(); //Show the prep screen
    //Reset several variable for the next level
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

    //Set the sprite palette
    pal_spr(palSprites);

    //Redraw the background (wall)
    drawBackground();
    
    //Generate the level
    generateLevel();

    //Print the level number
    printLevel();

    //Reset the paddle's and ball's sprites
    spr=oam_meta_spr(x_paddle,y_paddle,spr,paddle);
    spr=oam_spr(x_ball,y_ball,0x45,1,spr);

    ppu_on_all(); //Turn the screen back on
    canDestroyBrick = TRUE;
}


//The main function
void main(void)
{
    //Show the start screen
    home();
    
    while(1) //The loop to let user replay after defeat or victory
    {
        //Turn the screen off
        ppu_off();

        //Show the prep screen
        showNextLevel();

        //Draw the background (wall)
        drawBackground();

        pal_spr(palSprites);//set palette for sprites

        //Generate the level
        generateLevel();

        //Print the current level number
        printLevel();

        //Turn the screen back on to render everything
        ppu_on_all();

        //The main loop containing all of our game logic
        while(1)
        {

            ppu_wait_nmi(); //Wait for next frame

            //Set the vram update to null in case it's been filled when a brick's been hit
            set_vram_update(NULL);

            spr = 0;
            //We get the user input
            key=pad_trigger(0);

            //To pause when the user presses START (Enter on a PC)
            if(key&PAD_START && !pause){
                pause = TRUE;
            } else if(key&PAD_START && pause){
                pause = FALSE;
            } else if(pause){
                //If the game is on pause, we just ignore everything after by skipping this iteration
                continue;
            }
            //We move the ball
            if(!move_ball())
            {
                //If we got false it means the ball fell off the screen, so we act upon it by showing the defeat screen
                lostScreen();
                break;
            }
            
            //We move the paddle (if the user pressed the expected keys for that)
            move_paddle();

            //We print the score
            printScore();

            if(isLevelFinished) //In this case the level is finished, all bricks have broken
            {
                //We wait 30 frames to let the user see the last brick being broken
                delay(30);
                if(currentLevelNb >= 2) //If we're on level 2, there are no other levels, so the user won
                {
                    //We show the victory screen
                    victoryScreen();
                    break;
                }
                //In other cases, there's another level to go through, so we make sure to get to that
                goToNextLevel();
            }

        }
        //If we get here, it means the user either lost or won, so we reset all of our variable, to get back at the beginning of our loop with proper values for a fresh start
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