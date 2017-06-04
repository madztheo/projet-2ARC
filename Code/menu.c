//this example shows music and sound effects use

#include "neslib.h"

const unsigned char palette[16]={ 0x0f,0x21,0x10,0x30,0x0f,0x14,0x21,0x31,0x0f,0x29,0x16,0x26,0x0f,0x09,0x19,0x29 };	//palette data


void put_str(unsigned int adr,const char *str)
{
	vram_adr(adr);

	while(1)
	{
		if(!*str) break;

		vram_put((*str++)-0x20);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

void game(void){
    //pal_bg(palBackground);
	//pal_spr(palSprites);
}

void home(void){
    static unsigned char i,pause;

	pause=FALSE;

	pal_col(1,0x21);
	//pal_bg(palette);//set background palette from an array

    put_str(NTADR_A(4,1), "OOOO OOOO OOOO OOOO OOOO");
    put_str(NTADR_A(4,2), "O    O  O O    O    O   ");
    put_str(NTADR_A(4,3), "O    OOOO  OOO  OOO OOOO");
    put_str(NTADR_A(4,4), "O    O  O    O    O O   ");
    put_str(NTADR_A(4,5), "OOOO O  O OOOO OOOO OOOO");

    put_str(NTADR_A(2,7), "OOOO OOOO OOO OOOO O  O OOOO");
    put_str(NTADR_A(2,8), "O  O O  O  O  O  O O  O O   ");
    put_str(NTADR_A(2,9), "OOO  OOOO  O  O  O O  O OOOO");
    put_str(NTADR_A(2,10),"O  O O O   O  O  O O  O O   ");
    put_str(NTADR_A(2,11),"OOOO O  O OOO OOOO OOOO OOOO");
    put_str(NTADR_A(2,12),"                O           ");
    put_str(NTADR_A(10,18),"PRESS  START");

	ppu_on_all();

	while(1)
	{
	    ppu_wait_frame();

		i=pad_trigger(0);

		//On attend que l'utilisateur appuie sur start (Enter)
		if(i&PAD_START){
            sfx_play(0,0);
            pal_clear(); //On efface l'�cran
		}
	}
}


void main(void)
{
	home();
}
