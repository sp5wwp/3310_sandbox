//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit2.h"
#include "dict_en.h"
#include "fonts.h"

#define RES_X       84
#define RES_Y       48
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm2 *Form2;

typedef enum
{
	TEXT_T9,
    TEXT_NORM
} text_entry_t;

text_entry_t text_mode = TEXT_NORM;

typedef enum
{
	DISP_NONE,
	DISP_SPLASH,
	DISP_MAIN_SCR,
	DISP_MENU,
    DISP_TEXT_ENTRY
} disp_state_t;

disp_state_t disp_state = DISP_NONE;

const uint8_t p_width = 2;
const uint32_t col_bg = ((98<<16) | (143<<8) | 110); //BGR

char code[15]={0};
char message[128]={0};
uint8_t pos=0;

//linking t9.c didn't work, *shrug*
uint8_t getDigit(const char c)
{
    return "22233344455566677778889999"[c-'a'];
}

char* getWord(char *dict, char *code)
{
    char *word = dict;

    uint8_t code_len=strlen(code);

    //count asterisks
    uint8_t depth=0;
    for(uint8_t i=0; i<code_len; i++)
    {
        if(code[i]=='*')
            depth++;
    }

    //subtract the amount of asterisks from the code length
    code_len-=depth;

    do
    {
        if(strlen(word)==code_len)
        {
            //speed up the search a bit. TODO: there's room for improvement here
            if(getDigit(word[0])>code[0])
            {
                break;
            }

            uint8_t sum=0;

            for(uint8_t i=0; i<code_len && sum==0; i++)
            {
                sum=getDigit(word[i])-code[i];
            }

            if(sum==0)
            {
                if(depth==0)
                    return word;
                depth--;
            }
        }

        word += strlen(word)+1;
    } while(strlen(word)!=0);

    return (char*)"";
}
//---------------------------------------------------------------------------

__fastcall TForm2::TForm2(TComponent* Owner)
	: TForm(Owner)
{
	;//
}
//---------------------------------------------------------------------------

void setPixel(uint8_t x, uint8_t y, uint8_t color)
{
	const uint8_t start_x = 60;
	const uint8_t start_y = 190;
	const uint8_t p_gap = 0;
	uint32_t col = color * col_bg;

	if(x<RES_X && y<RES_Y)
	{
		Form2->Canvas->Brush->Color = TColor(col);
		Form2->Canvas->FillRect({start_x+x*(p_width+p_gap), start_y+y*(p_width+p_gap), start_x+x*(p_width+p_gap)+p_width, start_y+y*(p_width+p_gap)+p_width});
	}
}

void setChar(uint8_t x, uint8_t y, const font_t *f, char c, uint8_t color)
{
	uint8_t h=f->height;
	c-=' ';

	for(uint8_t i=0; i<h; i++)
	{
		for(uint8_t j=0; j<f->symbol[c].width; j++)
		{
			if(f->symbol[c].rows[i] & 1<<(((h>8)?(h+3):(h))-1-j)) //fonts are right-aligned
				setPixel(x+j, y+i, color);
		}
	}
}

//TODO: fix multiline text alignment when not in ALIGN_LEFT mode
void setString(uint8_t x, uint8_t y, const font_t *f, char *str, uint8_t color, align_t align)
{
	uint8_t xp=0, w=0;

    //get width
    for(uint8_t i=0; i<strlen(str); i++)
		w+=f->symbol[str[i]-' '].width;

	switch(align)
	{
		case ALIGN_LEFT:
			xp=0;
		break;

		case ALIGN_CENTER:
			xp=(RES_X-w)/2;
		break;

		case ALIGN_RIGHT:
			xp=RES_X-w;
		break;

		case ALIGN_ARB:
			xp=x;
		break;

		default: //ALIGN_LEFT
			xp=0;
        break;
	}

	for(uint8_t i=0; i<strlen(str); i++)
	{
		if(xp > RES_X-f->symbol[str[i]-' '].width)
		{
			y+=f->height+1;
			xp=0; //ALIGN_LEFT
		}

		setChar(xp, y, f, str[i], color);
		xp+=f->symbol[str[i]-' '].width;
	}
}

void drawRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color, uint8_t fill)
{
	x0 = (x0>=RES_X) ? RES_X-1 : x0;
	y0 = (y0>=RES_Y) ? RES_Y-1 : y0;
	x1 = (x1>=RES_X) ? RES_X-1 : x1;
	y1 = (y1>=RES_Y) ? RES_Y-1 : y1;

	if(fill)
	{
		for(uint8_t i=x0; i<=x1; i++)
		{
			for(uint8_t j=y0; j<=y1; j++)
			{
				setPixel(i, j, color);
            }
		}
	}
	else
	{
		for(uint8_t i=x0; i<=x1; i++)
		{
			setPixel(i, y0, color);
			setPixel(i, y1, color);
		}
		for(uint8_t i=y0+1; i<=y1-1; i++)
		{
			setPixel(x0, i, color);
			setPixel(x1, i, color);
        }
    }
}
//---------------------------------------------------------------------------

void clrScreen(uint8_t color)
{
	for(uint8_t i=0; i<RES_X; i++)
	{
		for(uint8_t j=0; j<RES_Y; j++)
		{
			setPixel(i, j, color);
		}
	}
}

char *addCode(char *code, char symbol)
{
	code[strlen(code)] = symbol;

	return getWord(dict_en, code);
}

void showMenu(char *title)
{
    disp_state = DISP_MENU;
	clrScreen(1);

	setString(0, 0, &nokia_small_bold, title, 0, ALIGN_CENTER);

    drawRect(0, 8, RES_X-1, 2*9-1, 0, 1);
	setString(1, 1*9, &nokia_small, (char*)"Messaging", 1, ALIGN_ARB);
	setString(1, 2*9, &nokia_small, (char*)"RF settings", 0, ALIGN_ARB);
	setString(1, 3*9, &nokia_small, (char*)"M17 settings", 0, ALIGN_ARB);
	setString(1, 4*9, &nokia_small, (char*)"Misc.", 0, ALIGN_ARB);
}

void showMainScreen(void)
{
	disp_state = DISP_MAIN_SCR;
	clrScreen(1);

	setString(0, 0*9, &nokia_small, (char*)"M17", 0, ALIGN_LEFT);

	setString(0, 15, &nokia_big, (char*)"SR5MS", 0, ALIGN_CENTER);
	setString(0, 30, &nokia_small, (char*)"438.8125", 0, ALIGN_CENTER);

	;
}

void showTextEntry(void)
{
	disp_state = DISP_TEXT_ENTRY;
	clrScreen(1);

    if(text_mode==TEXT_T9)
		setString(0, 0, &nokia_small, (char*)"T9", 0, ALIGN_LEFT);
	else
		setString(0, 0, &nokia_small, (char*)"Abc", 0, ALIGN_LEFT);

	setString(0, RES_Y-8, &nokia_small_bold, (char*)"Options", 0, ALIGN_CENTER);
}

void __fastcall TForm2::FormPaint(TObject *Sender)
{
	Form2->Canvas->Brush->Color = TColor(col_bg);
	Form2->Canvas->FillRect({40, 140, 220, 220});
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton1Click(TObject *Sender)
{
	//OK
	if(disp_state==DISP_MAIN_SCR)
	{
		showMenu((char*)"Main menu");
	}
	else if(disp_state==DISP_MENU)
	{
		showTextEntry();
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton2Click(TObject *Sender)
{
	//Cancel
	if(disp_state==DISP_MENU)
	{
		showMainScreen();
    }
	else if(disp_state==DISP_TEXT_ENTRY)
	{
		if(strlen(message)>0)
		{
			memset(&message[strlen(message)-1], 0, sizeof(message)-strlen(message));
			pos=strlen(message);
			memset(code, 0, strlen(code));

			drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
			setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton3Click(TObject *Sender)
{
	//Right/Up
	pos=strlen(message);
	memset(code, 0, strlen(code));
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton4Click(TObject *Sender)
{
	//Left/Down
    ;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton5Click(TObject *Sender)
{
	//1
	char *last = &message[pos];

	if(*last=='.')
		*last=',';
	else if(*last==',')
		*last='\'';
	else if(*last=='\'')
		*last='!';
	else if(*last=='!')
		*last='?';
	else if(*last=='?')
		*last='.';
	else
		message[pos] = '.';

	memset(code, 0, strlen(code));

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);

	Timer1->Enabled = false;
    Timer1->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton6Click(TObject *Sender)
{
	//2
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '2');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('a'):
				*last = 'b';
			break;

			case('b'):
				*last = 'c';
			break;

			case('c'):
				*last = '2';
			break;

			case('2'):
				*last = 'a';
			break;

			default:
				*last = 'a';
			break;
		}

        Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton7Click(TObject *Sender)
{
	//3
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '3');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('d'):
				*last = 'e';
			break;

			case('e'):
				*last = 'f';
			break;

			case('f'):
				*last = '3';
			break;

			case('3'):
				*last = 'd';
			break;

			default:
				*last = 'd';
			break;
		}

        Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton8Click(TObject *Sender)
{
	//4
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '4');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('g'):
				*last = 'h';
			break;

			case('h'):
				*last = 'i';
			break;

			case('i'):
				*last = '4';
			break;

			case('4'):
				*last = 'g';
			break;

			default:
				*last = 'g';
			break;
		}

        Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton9Click(TObject *Sender)
{
	//5
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '5');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
            pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('j'):
				*last = 'k';
			break;

			case('k'):
				*last = 'l';
			break;

			case('l'):
				*last = '5';
			break;

			case('5'):
				*last = 'j';
			break;

			default:
				*last = 'j';
			break;
		}

        Timer1->Enabled = false;
		Timer1->Enabled = true;
    }

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton10Click(TObject *Sender)
{
	//6
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '6');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('m'):
				*last = 'n';
			break;

			case('n'):
				*last = 'o';
			break;

			case('o'):
				*last = '6';
			break;

			case('6'):
				*last = 'm';
			break;

			default:
				*last = 'm';
			break;
		}

        Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton11Click(TObject *Sender)
{
	//7
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '7');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('p'):
				*last = 'q';
			break;

			case('q'):
				*last = 'r';
			break;

			case('r'):
				*last = 's';
			break;

			case('s'):
				*last = '7';
			break;

			case('7'):
				*last = 'p';
			break;

			default:
				*last = 'p';
			break;
		}

        Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton12Click(TObject *Sender)
{
	//8
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '8');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('t'):
				*last = 'u';
			break;

			case('u'):
				*last = 'v';
			break;

			case('v'):
				*last = '8';
			break;

			case('8'):
				*last = 't';
			break;

			default:
				*last = 't';
			break;
		}

        Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton13Click(TObject *Sender)
{
	//9
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '9');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
		else
		{
			message[strlen(message)]='?';
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

    	Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('w'):
				*last = 'x';
			break;

			case('x'):
				*last = 'y';
			break;

			case('y'):
				*last = 'z';
			break;

			case('z'):
				*last = '9';
			break;

			case('9'):
				*last = 'w';
			break;

			default:
				*last = 'w';
			break;
		}

		Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton14Click(TObject *Sender)
{
	//*
	if(text_mode==TEXT_T9)
	{
		char *w = addCode(code, '*');

		if(strlen(w)!=0)
		{
			strcpy(&message[pos], w);
		}
	}
	else
	{
		if(Timer1->Enabled)
		{
			pos++;
		}

		Timer1->Enabled = false;
		char *last = &message[pos];

		switch(*last)
		{
			case('*'):
				*last = '+';
			break;

			case('+'):
				*last = '*';
			break;

			default:
				*last = '*';
			break;
		}

		Timer1->Enabled = false;
		Timer1->Enabled = true;
	}

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
	setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton15Click(TObject *Sender)
{
	//0
	message[strlen(message)]=' ';
    pos=strlen(message);

	drawRect(0, 10, RES_X-1, RES_Y-9, 1, 1);
    setString(0, 10, &nokia_small, message, 0, ALIGN_LEFT);

	memset(code, 0, strlen(code));
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton16Click(TObject *Sender)
{
	//#
	if(disp_state==DISP_TEXT_ENTRY)
	{
		if(text_mode==TEXT_T9)
			text_mode = TEXT_NORM;
		else
			text_mode = TEXT_T9;

		drawRect(0, 0, 15, 8, 1, 1);

		if(text_mode==TEXT_T9)
			setString(0, 0, &nokia_small, (char*)"T9", 0, ALIGN_LEFT);
		else
			setString(0, 0, &nokia_small, (char*)"Abc", 0, ALIGN_LEFT);
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm2::SpeedButton17Click(TObject *Sender)
{
	//On/Off
	disp_state = DISP_SPLASH;

    clrScreen(1);

	setString(0, 9, &nokia_big, (char*)"Welcome", 0, ALIGN_CENTER);
	setString(0, 22, &nokia_big, (char*)"message", 0, ALIGN_CENTER);
	setString(0, 41, &nokia_small, (char*)"SP5WWP", 0, ALIGN_CENTER);

    Sleep(1000);

	clrScreen(1);

	showMainScreen();
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Timer1Timer(TObject *Sender)
{
    pos=strlen(message);
	memset(code, 0, strlen(code));
    Timer1->Enabled = false;
}
//---------------------------------------------------------------------------

