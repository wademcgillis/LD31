#include <WadeWork/wadework.h>
#include <time.h>
#include <vector>
#include "rapidxml.hpp"

#define M_PI 3.1415926535f

// ------------------------------------------------------------------------------
// *
// *
// *
// *			CLASSES AND STUFF NOT GAME RELATED. GFX + SFX + ETC
// *
// *
// *
// ------------------------------------------------------------------------------

class SFX
{
private:
	char *DATA;
	unsigned int MAXSAMPLECOUNT;
	unsigned int POSITION;
	ww::sfx::Sound sound;
	bool finished;
public:
	SFX()
	{
		MAXSAMPLECOUNT = 4000; // one second
		DATA = new char[MAXSAMPLECOUNT];
		POSITION = 0;
		finished = false;
	}
	SFX(unsigned int duration_ms)
	{
		MAXSAMPLECOUNT = duration_ms * 4;
		DATA = new char[MAXSAMPLECOUNT];
		POSITION = 0;
		finished = false;
	}
	void TONE(unsigned char FREQ, unsigned int duration_ms)
	{
		if (finished)
			return;
		unsigned int i = 0;
		for(i=0;i<duration_ms * 4 && POSITION+i < MAXSAMPLECOUNT;i++)
		{
			if (FREQ == 0)
				DATA[POSITION+i] = 0;
			else
				DATA[POSITION+i] = 255*((i/FREQ) % 2);
		}
		POSITION += i;
	}
	void FINISH()
	{
		if (finished)
			return;
		finished = true;
		sound.gen((const char*)DATA,POSITION,4000);
	}
	ww::sfx::Sound *GET()
	{
		if (finished)
			return &sound;
		return NULL;
	}
};

class SongRoller
{
private:
	ww::sfx::Sound sound;
public:
	SongRoller()
	{
	}
	bool load(const char *path)
	{
		ww::gfx::Texture *tex = new ww::gfx::Texture();
		if (!tex->load(path))
		{
			delete tex;
			return false;
		}
		unsigned int smallest_note = 32; // smallest note is a sixteenth note.
		unsigned int NOTE_DUR = 4000 / smallest_note; // 4000 is the data rate.
		int LEN = tex->getWidth();
		int H = tex->getHeight();
		int MFREQ = tex->getHeight();
		if (MFREQ > 256)
			MFREQ = 256;
		unsigned short *data = new unsigned short[NOTE_DUR*LEN];
		memset(data,0,2 * NOTE_DUR * LEN); // 2 because short is two bytes. OR IT SHOULD BE! >:C
		unsigned int prevX = -1;
		for(unsigned int i=0;i<NOTE_DUR*LEN;i++)
		{
			unsigned int x = i / NOTE_DUR;
			for(unsigned int y=0;y<MFREQ;y++)
				if (tex->getRGBPixel(x,y) == 0xFF000000)
				{
					if (x != prevX)
						printf("BLACK @ %i, %i\n",x,y);
					data[i] += (unsigned char)(255*(      (i/(H-y)) % 2       ));
				}
			prevX = x;
		}
		unsigned char *data_out = new unsigned char[NOTE_DUR*LEN];
		for(unsigned int i=0;i<NOTE_DUR*LEN;i++)
		{
			if (data[i] > 127)
				data_out[i] = 127;
			else if (data[i] < -128)
				data_out[i] = -128;
			else
				data_out[i] = data[i];
		}


		sound.gen((const char*)data_out,NOTE_DUR*LEN,4000);
		delete data;
		delete data_out;
		delete tex;
		return true;
	}
	ww::sfx::Sound *getSound()
	{
		return &sound;
	}
};

class LD31Sprite
{
public:
	ww::gfx::Vertex vertices[6];
	LD31Sprite()
	{
	}
	LD31Sprite(ww::gfx::Texture *tex, unsigned int width, unsigned int height, unsigned int u, unsigned int v, unsigned int w, unsigned int h)
	{
		float u1 = (/*.5f+*/(float)u) / (float)tex->getWidth();
		float v1 = (/*.5f+*/(float)v) / (float)tex->getHeight();
		float u2 = (/*-.5f+*/(float)(u+width)) / (float)tex->getWidth();
		float v2 = (/*-.5f+*/(float)(v+height)) / (float)tex->getHeight();

		vertices[0] = ww::gfx::MakeVertex(width,height,0,0xFFFFFFFF,u2,v2);
		vertices[1] = ww::gfx::MakeVertex(width,0,0,0xFFFFFFFF,u2,v1);
		vertices[2] = ww::gfx::MakeVertex(0,0,0,0xFFFFFFFF,u1,v1);
		vertices[3] = ww::gfx::MakeVertex(width,height,0,0xFFFFFFFF,u2,v2);
		vertices[4] = ww::gfx::MakeVertex(0,0,0,0xFFFFFFFF,u1,v1);
		vertices[5] = ww::gfx::MakeVertex(0,height,0,0xFFFFFFFF,u1,v2);
	}
};

#define SPRITETILE(X,Y,W,H) LD31Sprite(&supertex,8*(W),8*(H),8*(X),8*(Y),8*(W),8*(H))

#define SPRITETILE_EXT(X,Y,W,H,OX,OY) LD31Sprite(&supertex,8*(W),8*(H),8*(X)+(OX),8*(Y)+(OY),8*(W),8*(H))

void batch_add_sprite(ww::gfx::VertexBatch *batch, LD31Sprite *sprite, int x, int y)
{
	batch->pushvertices(sprite->vertices,6);
	if (x != 0 || y != 0)
	{
		int n = batch->getVertexCount();
		ww::gfx::Vertex *vertices = (ww::gfx::Vertex *)batch->getVertices();
		vertices[n-1].x += x;
		vertices[n-1].y += y;
		vertices[n-2].x += x;
		vertices[n-2].y += y;
		vertices[n-3].x += x;
		vertices[n-3].y += y;
		vertices[n-4].x += x;
		vertices[n-4].y += y;
		vertices[n-5].x += x;
		vertices[n-5].y += y;
		vertices[n-6].x += x;
		vertices[n-6].y += y;
	}
}

// ------------------------------------------------------------------------------
// *
// *
// *
// *						GAMEPLAY AND STUFF!			HOORAY
// *
// *
// *
// ------------------------------------------------------------------------------

bool doinit();
bool dorun();
bool doend();

const unsigned int window_width = 1024;
const unsigned int window_height = window_width * .75f;

char *file_alloc_contents(const char *path)
{
	FILE *f = fopen(path,"rb");
	if (f == NULL)
		return NULL;
	fseek(f,0,SEEK_END);
	long len = ftell(f);
	fseek(f,0,SEEK_SET);
	char *contents = new char[len+1];
	memset(contents,0,len+1);
	fread(contents, len, 1, f);
	fclose(f);
	return contents;
}

int main(int argc, char *argv[])
{
	ww::gfx::setWindowSize(window_width,window_height);
	ww::sys::setInitCallback(doinit);
	ww::sys::setTimerCallback(dorun);
	ww::sys::setDeinitCallback(doend);
	ww::sys::setTimerResolution(30);
	//ww::setForegroundCallback(NULL);
	//ww::setBackgroundCallback(NULL);
	return ww::sys::setup(ww::sys::CONFIG_OPENGL2 | ww::sys::CONFIG_DISABLE_OPENGL_DEPTHBUFFER);
}

ww::gfx::Shader *defaultShader = NULL;
GLint defaultVertexMatrixID;

#include <WadeWork/etc/CRT.h>
ww::etc::CRT *myCRT = NULL;

ww::gfx::Texture supertex;
ww::gfx::Texture exampleScanlines;
ww::gfx::VertexBatch exampleBatch;

ww::gfx::VertexBatch colorBatch;

ww::gfx::Texture FGTEX;
GLuint FGCOL;

unsigned int FG_ARRAY[32*32];
unsigned int *ZXCOLORPTR = FG_ARRAY;

const unsigned char ZX_BLACK = 0;
const unsigned char ZX_BLUE = 1;
const unsigned char ZX_FUCHSIA = 2;
const unsigned char ZX_RED = 3;
const unsigned char ZX_GREEN = 4;
const unsigned char ZX_CYAN = 5;
const unsigned char ZX_YELLOW = 6;
const unsigned char ZX_GRAY = 7;

const unsigned char ZX_LTBLACK = 8;
const unsigned char ZX_LTBLUE = 9;
const unsigned char ZX_LTFUCHSIA = 10;
const unsigned char ZX_LTRED = 11;
const unsigned char ZX_LTGREEN = 12;
const unsigned char ZX_LTCYAN = 13;
const unsigned char ZX_LTYELLOW = 14;
const unsigned char ZX_LTGRAY = 15;

unsigned int ZX_COLORS[16];

void SET_COLOR(unsigned char x, unsigned char y, unsigned char col)
{
	if (x > 31 || y > 23)
		return;
	ZXCOLORPTR[32*y + x] = ZX_COLORS[col];
}

void updateColors()
{
	glBindTexture(GL_TEXTURE_2D,FGCOL);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,32,32,0,GL_BGRA,GL_UNSIGNED_BYTE,ZXCOLORPTR);
	glBindTexture(GL_TEXTURE_2D,0);
}

SongRoller song1;
SongRoller song2;
SongRoller song3;

const unsigned char TILE_NOTHING = 0;
const unsigned char TILE_GROUND = 1;
const unsigned char TILE_SNOW = 2;
const unsigned char TILE_WOOD = 3;
const unsigned char TILE_LAVA_TOP = 4;
const unsigned char TILE_LAVA = 5;

bool tileIsSolid(unsigned char tileID)
{
	switch(tileID)
	{
	case TILE_NOTHING: case TILE_LAVA_TOP: case TILE_LAVA:
		return false;
	default:
		return true;
	}
}


LD31Sprite *fancyfont[16*6];
LD31Sprite *sprTileGround;
LD31Sprite *sprTileSnow;
LD31Sprite *sprTileWood;
LD31Sprite *sprTileLavaTop[16];
LD31Sprite *sprTileLava[8];
LD31Sprite *sprTileBorder[16];


int toolWindow;
int tabs, tabLevels, tabObjects, tabTiles, tabColors;
int fileGroup, objectsGroup, tilesGroup, colorsGroup;
// FILE
int adventurePathText, newButton, openButton, saveButton, saveAsButton,
levelListBox, addLevelButton, deleteLevelButton, levelNameText, levelNameSetButton, levelNoveltyText, levelNoveltySetButton;
// OBJECTS
int objectsView;
// TILES
int tilesView, LDTV_GROUND, LDTV_SNOW, LDTV_WOOD, LDTV_LAVATOP, LDTV_LAVA;
// COLORS
int colorView;


bool doinit()
{
	toolWindow = ww::gui::window_create(1,256,384,"Editor");

	tabs = ww::gui::tabcontroller_create(toolWindow,8,8,240,368);
	tabLevels = ww::gui::tabcontroller_addtab(tabs,0,"Levels");
	tabObjects = ww::gui::tabcontroller_addtab(tabs,1,"Objects");
	tabTiles = ww::gui::tabcontroller_addtab(tabs,2,"Tiles");
	tabColors = ww::gui::tabcontroller_addtab(tabs,3,"Colors");
	//int tabInfo = ww::gui::tabcontroller_addtab(tabs,3,"Info");

	int fileGroup = ww::gui::controlgroup_create();
		ww::gui::controlgroup_addcontrol(fileGroup,ww::gui::text_create(tabs,16,32,208,20,"Adventure Path:"));
		adventurePathText = ww::gui::textbox_create(tabs,16,48,208,20,false,false,false,false,true);	
			ww::gui::controlgroup_addcontrol(fileGroup,adventurePathText);
		newButton = ww::gui::button_create(tabs,16,72,48,24,"New");
			ww::gui::controlgroup_addcontrol(fileGroup,newButton);
		openButton = ww::gui::button_create(tabs,64,72,48,24,"Open");
			ww::gui::controlgroup_addcontrol(fileGroup,openButton);
		saveButton = ww::gui::button_create(tabs,112,72,48,24,"Save");
			ww::gui::controlgroup_addcontrol(fileGroup,saveButton);
		saveAsButton = ww::gui::button_create(tabs,160,72,56,24,"Save As");
			ww::gui::controlgroup_addcontrol(fileGroup,saveAsButton);
		ww::gui::controlgroup_addcontrol(fileGroup,ww::gui::text_create(tabs,16,108,208,20,"Levels:"));
		levelListBox = ww::gui::listbox_create(tabs,16,124,208,120);
			ww::gui::controlgroup_addcontrol(fileGroup,levelListBox);
		addLevelButton = ww::gui::button_create(tabs,16,236,48,24,"Add");
			ww::gui::controlgroup_addcontrol(fileGroup,addLevelButton);
		deleteLevelButton = ww::gui::button_create(tabs,16+208-48,236,48,24,"Delete");
			ww::gui::controlgroup_addcontrol(fileGroup,deleteLevelButton);
		ww::gui::controlgroup_addcontrol(fileGroup,ww::gui::groupbox_create(tabs,16,264,208,100,"Level Info:"));
		ww::gui::controlgroup_addcontrol(fileGroup,ww::gui::text_create(tabs,32,272+8,128,20,"Level Name:"));
		levelNameText = ww::gui::textbox_create(tabs,32,272+24,176-32,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelNameText,16);
			ww::gui::controlgroup_addcontrol(fileGroup,levelNameText);
		levelNameSetButton = ww::gui::button_create(tabs,32+176-32,272+24,32,20,"Set");
			ww::gui::controlgroup_addcontrol(fileGroup,levelNameSetButton);
		ww::gui::controlgroup_addcontrol(fileGroup,ww::gui::text_create(tabs,32,272+8+40,128,20,"Novelty Text:"));
		levelNoveltyText = ww::gui::textbox_create(tabs,32,272+24+40,176,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelNoveltyText,32);
			ww::gui::controlgroup_addcontrol(fileGroup,levelNoveltyText);
		//levelNoveltySetButton = ww::gui::button_create(tabs,32+176-32,272+24+40,32,20,"Set");
		//	ww::gui::controlgroup_addcontrol(fileGroup,levelNoveltySetButton);
		//

	ww::gui::controlgroup_setvisible(fileGroup,true);


	unsigned char *texPtr;
	int texW, texH;
	texPtr = ww::utils::load_image("gfx/supertex.png",&texW,&texH);

	tilesGroup = ww::gui::controlgroup_create();
		tilesView = ww::gui::listview_create(tabs,12,32,216,224,false);
		int tilesImgList = ww::gui::imagelist_create(8,8);
		ww::gui::controlgroup_addcontrol(tilesGroup,tilesView);
		ww::gui::listview_setimagelist(tilesView,tilesImgList);
		ww::gui::listview_seticonspacing(tilesView,64,32);

		LDTV_GROUND = ww::gui::listview_additem(tilesView,"GROUND",ww::gui::imagelist_addimage(tilesImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,0,88,8,8)));
		LDTV_SNOW = ww::gui::listview_additem(tilesView,"SNOW",ww::gui::imagelist_addimage(tilesImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,8,88,8,8)));
		LDTV_WOOD = ww::gui::listview_additem(tilesView,"WOOD",ww::gui::imagelist_addimage(tilesImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,16,88,8,8)));
		LDTV_LAVATOP = ww::gui::listview_additem(tilesView,"LAVA TOP",ww::gui::imagelist_addimage(tilesImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,96,0,8,8)));
		LDTV_LAVA = ww::gui::listview_additem(tilesView,"LAVA",ww::gui::imagelist_addimage(tilesImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,96,8,8,8)));
	ww::gui::controlgroup_setvisible(tilesGroup,false);

	colorsGroup = ww::gui::controlgroup_create();
		colorView = ww::gui::listview_create(tabs,12,32,216,224,false);
		ww::gui::controlgroup_addcontrol(colorsGroup,colorView);
		unsigned char *colorPtr;
		int colorW, colorH;
		colorPtr = ww::utils::load_image("gfx/palette.png",&colorW,&colorH);
		int colorImgList = ww::gui::imagelist_create(24,24);
		ww::gui::listview_setimagelist(colorView,colorImgList);
		ww::gui::listview_seticonspacing(colorView,64,32);

		ww::gui::listview_additem(colorView,"BLACK",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,0,0,24,24)));
		ww::gui::listview_additem(colorView,"BLUE",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,24,0,24,24)));
		ww::gui::listview_additem(colorView,"FUCHSIA",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,48,0,24,24)));
		ww::gui::listview_additem(colorView,"RED",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,72,0,24,24)));
		ww::gui::listview_additem(colorView,"GREEN",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,96,0,24,24)));
		ww::gui::listview_additem(colorView,"CYAN",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,120,0,24,24)));
		ww::gui::listview_additem(colorView,"YELLOW",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,144,0,24,24)));
		ww::gui::listview_additem(colorView,"GRAY",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,168,0,24,24)));

		ww::gui::listview_additem(colorView,"LT_BLACK",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,0,24,24,24)));
		ww::gui::listview_additem(colorView,"LT_BLUE",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,24,24,24,24)));
		ww::gui::listview_additem(colorView,"LT_FUCHSIA",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,48,24,24,24)));
		ww::gui::listview_additem(colorView,"LT_RED",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,72,24,24,24)));
		ww::gui::listview_additem(colorView,"LT_GREEN",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,96,24,24,24)));
		ww::gui::listview_additem(colorView,"LT_CYAN",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,120,24,24,24)));
		ww::gui::listview_additem(colorView,"LT_YELLOW",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,144,24,24,24)));
		ww::gui::listview_additem(colorView,"LT_GRAY",ww::gui::imagelist_addimage(colorImgList,ww::gui::image_createfromptrsubrect(colorPtr,colorW,colorH,168,24,24,24)));
	ww::gui::controlgroup_setvisible(colorsGroup,false);

	// load the super texture from the data we loaded before for win32.
	supertex.loadLinearFromMemory((unsigned int*)texPtr,texW,texH,true);
	supertex.setLinearInterpolation(false);
	// Then replace all blue pixels with transparent!
	for(int x=0;x<supertex.getWidth();x++)
		for(int y=0;y<supertex.getHeight();y++)
			if (supertex.getBGRPixel(x,y) == 0xFF0000FF)
				supertex.setRGBPixel(x,y,0x00000000);
	supertex.update();
	// NOW we do the objects view
	objectsGroup = ww::gui::controlgroup_create();
		objectsView = ww::gui::listview_create(tabs,12,32,216,224,false);
		ww::gui::controlgroup_addcontrol(objectsGroup,objectsView);
		int objectImgList = ww::gui::imagelist_create(16,16);
		ww::gui::listview_setimagelist(objectsView,objectImgList);
		ww::gui::listview_seticonspacing(objectsView,64,32);

		ww::gui::listview_additem(objectsView,"Player",ww::gui::imagelist_addimage(objectImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,0,0,16,16)));
		ww::gui::listview_additem(objectsView,"Boulder",ww::gui::imagelist_addimage(objectImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,16,0,16,16)));
		ww::gui::listview_additem(objectsView,"Door",ww::gui::imagelist_addimage(objectImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,32,0,16,16)));
		ww::gui::listview_additem(objectsView,"Cake",ww::gui::imagelist_addimage(objectImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,48,0,16,16)));
		ww::gui::listview_additem(objectsView,"Telephone",ww::gui::imagelist_addimage(objectImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,80,0,16,16)));
	ww::gui::controlgroup_setvisible(objectsGroup,false);


	song1.load("sfx/test1.png");
	song2.load("sfx/test2.png");
	song3.load("sfx/test3.png");

	ww::gfx::Texture palette;
	palette.load("gfx/palette.png");
	unsigned int w = palette.getWidth();
	unsigned int h = palette.getHeight();
	for(int i=0;i < 16;i++)
		ZX_COLORS[i] = palette.getBGRPixel(24 * (i%8),24 * (1-i/8));




	exampleScanlines.load("gfx/scanline.png");

	ww::gfx::Texture fgt;
	fgt.loadLinearFromMemory(FG_ARRAY,32,32);
	fgt.setLinearInterpolation(false);
	FGCOL = fgt.getTexture();

	for(int i = 0;i<32*24;i++)
	{
		SET_COLOR(i%32,i/32,ZX_LTGRAY-rand()%5);
	}
	updateColors();

	if (ww::gfx::supportsOpenGL2())
	{
		defaultShader = ww::gfx::ShaderBuilder::genericShader(ww::gfx::ShaderBuilder::DEFAULT_SHADER_NOLIGHTING);
		if (defaultShader == NULL)
			defaultVertexMatrixID = defaultShader->getUniformIDFromName("matrix");
		myCRT = new ww::etc::CRT(256,192,false,&exampleScanlines,256/2,192/2);
	}
	float x1, x2, y1, y2;
	x1 = 0.f;
	y1 = 0.f;
	x2 = 256.f / 256.f;
	y2 = 192.f / 256.f;
	exampleBatch.clear();
		exampleBatch.pushvertex(ww::gfx::MakeVertex(256,192,0,0xFFFFFFFF,x2,y2));
		exampleBatch.pushvertex(ww::gfx::MakeVertex(256,0,0,0xFFFFFFFF,x2,y1));
		exampleBatch.pushvertex(ww::gfx::MakeVertex(0,0,0,0xFFFFFFFF,x1,y1));
		exampleBatch.pushvertex(ww::gfx::MakeVertex(256,192,0,0xFFFFFFFF,x2,y2));
		exampleBatch.pushvertex(ww::gfx::MakeVertex(0,0,0,0xFFFFFFFF,x1,y1));
		exampleBatch.pushvertex(ww::gfx::MakeVertex(0,192,0,0xFFFFFFFF,x1,y2));
	exampleBatch.update();
	x1 = 0.f;
	y1 = 0.f;
	x2 = 32.f / 32.f;
	y2 = 24.f / 32.f;
	colorBatch.clear();
		colorBatch.pushvertex(ww::gfx::MakeVertex(256,192,0,0xFFFFFFFF,x2,y2));
		colorBatch.pushvertex(ww::gfx::MakeVertex(256,0,0,0xFFFFFFFF,x2,y1));
		colorBatch.pushvertex(ww::gfx::MakeVertex(0,0,0,0xFFFFFFFF,x1,y1));
		colorBatch.pushvertex(ww::gfx::MakeVertex(256,192,0,0xFFFFFFFF,x2,y2));
		colorBatch.pushvertex(ww::gfx::MakeVertex(0,0,0,0xFFFFFFFF,x1,y1));
		colorBatch.pushvertex(ww::gfx::MakeVertex(0,192,0,0xFFFFFFFF,x1,y2));
	colorBatch.update();
	for(int i=0;i<16*6;i++)
		fancyfont[i] = new SPRITETILE(i%16,24+i/16,1,1);
	sprTileGround = new SPRITETILE(0,11,1,1);
	sprTileSnow = new SPRITETILE(1,11,1,1);
	sprTileWood = new SPRITETILE(2,11,1,1);
	for(int i=0;i<8;i++)
		sprTileLava[i] = new SPRITETILE_EXT(12,1,1,1,2*i,0);
	for(int i=0;i<16;i++)
		sprTileLavaTop[i] = new SPRITETILE_EXT(12,0,1,1,2*i,0);
	for(int i=0;i<16;i++)
	{
		sprTileBorder[i] = new SPRITETILE(0+(i%4),12+(i/4),1,1);
	}
	return true;
}

void draw_fancy_text(ww::gfx::VertexBatch *dest, int x, int y, char *text, unsigned char color)
{
	if (text == NULL)
		return;
	if (color > 15)
		return;
	int len = strlen(text);
	for(int i=0;i<len;i++)
	{
		batch_add_sprite(dest,fancyfont[text[i]-' '],8*(x+i),8*y);
		SET_COLOR(x+i,y,color);
	}
}

typedef struct
{
	unsigned char type;
	/*unsigned short num1;
	unsigned short num2;
	unsigned short num3;
	char *str1;
	char *str2;
	char *str3;*/
} LD31Tile;

class LD31LevelData
{
public:
	char *name;
	char *noveltyText;
	LD31Tile tiles[32][24];
	unsigned int COLORS[32*32];
	void setColor(int x, int y, unsigned char color)
	{
		if (x < 0 || x > 31 || y < 0 || y > 23 || color > 15)
			return;
		COLORS[x + 32*y] = ZX_COLORS[color];
	}
	void setTileID(int x, int y, unsigned char tileID)
	{
		if (x < 0 || x > 31 || y < 0 || y > 23)
			return;
		tiles[x][y].type = tileID;
	}
	unsigned char getTileID(int x, int y)
	{
		if (x < 0 || x > 31 || y < 0 || y > 23)
			return TILE_NOTHING;
		return tiles[x][y].type;
	}
};

class LD31Adventure
{
public:
	std::vector<LD31LevelData*> levels;
	char *author;
	LD31LevelData *addNewLevel(char *name)
	{
		LD31LevelData *newdata = new LD31LevelData();
		newdata->name = name;
		newdata->noveltyText = NULL;
		for(int i=0;i<32;i++)
			for(int j=0;j<24;j++)
			{
				newdata->tiles[i][j].type = 0;
				/*newdata->tiles[i][j].num1 = 0;
				newdata->tiles[i][j].num2 = 0;
				newdata->tiles[i][j].num3 = 0;
				newdata->tiles[i][j].str1 = NULL;
				newdata->tiles[i][j].str2 = NULL;
				newdata->tiles[i][j].str3 = NULL;*/
				newdata->COLORS[i+32*j] = ZX_COLORS[ZX_LTGRAY];
			}
		levels.push_back(newdata);
		return newdata;
	}
	bool loadFromFile(const char *path)
	{
		char *xml = file_alloc_contents(path);
		if (!xml)
			return false;
		rapidxml::xml_document<char> doc;
		doc.parse<0>(xml);
		rapidxml::xml_node<char> *adventure = doc.first_node();
		rapidxml::xml_node<char> *xmlauthor = adventure->first_node("author");
		if (xmlauthor != NULL)
			author = xmlauthor->value();
		rapidxml::xml_node<char> *xmllevels = adventure->first_node("levels");
		for(rapidxml::xml_node<char> *xmllevel = xmllevels->first_node("level");xmllevel != NULL;xmllevel = xmllevel->next_sibling("level"))
		{
			rapidxml::xml_node<char> *namenode = xmllevel->first_node("name");
			if (namenode != NULL)
			{
				int namelen = namenode->value_size();
				if (namelen > 0)
				{
					LD31LevelData *data = new LD31LevelData();
					data->name = new char[namelen+1];
					sprintf(data->name,"%s",namenode->value());
					rapidxml::xml_node<char> *noveltynode = xmllevel->first_node("novelty");
					if (noveltynode == NULL)
						data->noveltyText = NULL;
					else
					{
						int l = noveltynode->value_size();
						if (l > 0)
						{
							data->noveltyText = new char[noveltynode->value_size()+1];
							sprintf(data->noveltyText,noveltynode->value());
						}
						else
							data->noveltyText = NULL;
					}
					rapidxml::xml_node<char> *colorsnode = xmllevel->first_node("colors");
					if (colorsnode != NULL)
					{
						if (colorsnode->value_size() != 32*24)
						{
							printf("Level has insuffient colordata! Skipping.\n");
							delete data;
							continue;
						}
						char *cols = colorsnode->value();
						for(int i=0;i<32*24;i++)
							data->COLORS[i] = ZX_COLORS[(cols[i]>='0'&&cols[i]<='9')?(cols[i]-'0'):((cols[i]>='A'&&cols[i]<='F')?(0xA+cols[i]-'A'):((cols[i]>='a'&&cols[i]<='f')?(0xA+cols[i]-'a'):ZX_FUCHSIA))];
					}
					else
					{
						printf("Level has no colordata! Skipping.\n");
						delete data;
						continue;
					}
					rapidxml::xml_node<char> *tilesnode = xmllevel->first_node("tiles");
					if (tilesnode != NULL)
					{
						for(rapidxml::xml_node<char> *tile = tilesnode->first_node("tile");tile != NULL;tile = tile->next_sibling("tile"))
							data->setTileID(atoi(tile->first_attribute("x")->value()),atoi(tile->first_attribute("y")->value()),atoi(tile->first_attribute("id")->value()));
					}
					else
					{
						printf("Level has no tiledata! Skipping.\n");
						delete data;
						continue;
					}
					printf("SUCCESS            ?\n");
					levels.push_back(data);
				}
				else
					printf("Level has no name! Skipping.\n");
			}
		}
		return true;
	}
	bool writeToFile(const char *path)
	{
		FILE *f = fopen(path,"wb");
		if (!f)
			return false;
		fprintf(f,"<adventure>\r\n");
		fprintf(f,"\t<author>%s</author>\r\n",author?author:"");
		fprintf(f,"\t<levels>\r\n");
		for(int i=0;i<levels.size();i++)
		{
			fprintf(f,"\t\t<level>\r\n");
			LD31LevelData *data = levels.at(i);
			fprintf(f,"\t\t\t<name>%s</name>\r\n",data->name?data->name:"");
			fprintf(f,"\t\t\t<novelty>%s</novelty>\r\n",data->noveltyText?data->noveltyText:"");
			char *colorData = new char[32*24 + 1];
			memset(colorData,0,32*24 + 1);
			for(int c=0;c<32*24;c++)
			{
				if (data->COLORS[c] == ZX_COLORS[ZX_BLACK])
					colorData[c] = '0';
				if (data->COLORS[c] == ZX_COLORS[ZX_BLUE])
					colorData[c] = '1';
				if (data->COLORS[c] == ZX_COLORS[ZX_FUCHSIA])
					colorData[c] = '2';
				if (data->COLORS[c] == ZX_COLORS[ZX_RED])
					colorData[c] = '3';
				if (data->COLORS[c] == ZX_COLORS[ZX_GREEN])
					colorData[c] = '4';
				if (data->COLORS[c] == ZX_COLORS[ZX_CYAN])
					colorData[c] = '5';
				if (data->COLORS[c] == ZX_COLORS[ZX_YELLOW])
					colorData[c] = '6';
				if (data->COLORS[c] == ZX_COLORS[ZX_GRAY])
					colorData[c] = '7';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTBLACK])
					colorData[c] = '8';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTBLUE])
					colorData[c] = '9';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTFUCHSIA])
					colorData[c] = 'A';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTRED])
					colorData[c] = 'B';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTGREEN])
					colorData[c] = 'C';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTCYAN])
					colorData[c] = 'D';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTYELLOW])
					colorData[c] = 'E';
				if (data->COLORS[c] == ZX_COLORS[ZX_LTGRAY])
					colorData[c] = 'F';
			}
			fprintf(f,"\t\t\t<colors>%s</colors>\r\n",colorData);
			fprintf(f,"\t\t\t<tiles>\r\n",colorData);
			for(int x=0;x<32;x++)
				for(int y=0;y<23;y++)
					fprintf(f,"\t\t\t\t<tile id=\"%i\" x=\"%i\" y=\"%i\" />\r\n",
						data->tiles[x][y].type,x,y);//,data->tiles[x][y].num1,data->tiles[x][y].num2,data->tiles[x][y].num3,data->tiles[x][y].str1,data->tiles[x][y].str2,data->tiles[x][y].str3);
			fprintf(f,"\t\t\t</tiles>\r\n");
			delete colorData;
			fprintf(f,"\t\t</level>\r\n");
		}
		fprintf(f,"\t</levels>\r\n");
		fprintf(f,"</adventure>\r\n");
		fclose(f);
	}
};

LD31Adventure testAdventure;

int selectedLevelIndex;

namespace ww
{
	namespace etc
	{
		extern glm::vec2 inv_crtcoord(float x, float y);
	}
}

bool dogame()
{
	if (ww::gui::tab_check(tabLevels))
	{
		ww::gui::controlgroup_setvisible(fileGroup,true);
		ww::gui::controlgroup_setvisible(objectsGroup,false);
		ww::gui::controlgroup_setvisible(tilesGroup,false);
		ww::gui::controlgroup_setvisible(colorsGroup,false);
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}
	if (ww::gui::tab_check(tabObjects))
	{
		ww::gui::controlgroup_setvisible(fileGroup,false);
		ww::gui::controlgroup_setvisible(objectsGroup,true);
		ww::gui::controlgroup_setvisible(tilesGroup,false);
		ww::gui::controlgroup_setvisible(colorsGroup,false);
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}
	if (ww::gui::tab_check(tabTiles))
	{
		ww::gui::controlgroup_setvisible(fileGroup,false);
		ww::gui::controlgroup_setvisible(objectsGroup,false);
		ww::gui::controlgroup_setvisible(tilesGroup,true);
		ww::gui::controlgroup_setvisible(colorsGroup,false);
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}
	if (ww::gui::tab_check(tabColors))
	{
		ww::gui::controlgroup_setvisible(fileGroup,false);
		ww::gui::controlgroup_setvisible(objectsGroup,false);
		ww::gui::controlgroup_setvisible(tilesGroup,false);
		ww::gui::controlgroup_setvisible(colorsGroup,true);
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}

	
	if (ww::gui::controlgroup_getvisible(fileGroup))
	{
		static int selectedLevelIndexBefore = -1;
		selectedLevelIndex = ww::gui::listbox_getselectedindex(levelListBox);
		static int noveltyLengthBefore = -1;
		char novel[34];
		ww::gui::textbox_gettext(levelNoveltyText,novel,33);
		int noveltyLength = strlen(novel);

		if (ww::gui::button_check(saveButton))
		{
			char path[256];
			ww::gui::textbox_gettext(adventurePathText,path,255);
			testAdventure.writeToFile(path);
		}

		if (ww::gui::button_check(openButton))
		{
			char path[256];
			ww::gui::textbox_gettext(adventurePathText,path,255);
			testAdventure.loadFromFile(path);
			while(ww::gui::listbox_getstringcount(levelListBox))
				ww::gui::listbox_deletestring(levelListBox,0);
			for(int i=0;i<testAdventure.levels.size();i++)
				ww::gui::listbox_addstring(levelListBox,testAdventure.levels.at(i)->name);
		}




		if (selectedLevelIndex != selectedLevelIndexBefore && selectedLevelIndex >= 0)
		{
			ZXCOLORPTR = testAdventure.levels.at(selectedLevelIndex)->COLORS;
			ww::gui::textbox_settext(levelNameText,testAdventure.levels.at(selectedLevelIndex)->name);
			ww::gui::textbox_settext(levelNoveltyText,testAdventure.levels.at(selectedLevelIndex)->noveltyText);
		}
		else
		{
			if (noveltyLength != noveltyLengthBefore && selectedLevelIndex >= 0)
			{
				if (noveltyLength == 0)
				{
					delete testAdventure.levels.at(selectedLevelIndex)->noveltyText;
					testAdventure.levels.at(selectedLevelIndex)->noveltyText = NULL;
				}
				else
				{
					delete testAdventure.levels.at(selectedLevelIndex)->noveltyText;
					char *newtext = new char[noveltyLength+1];
					sprintf(newtext,"%s",novel);
					testAdventure.levels.at(selectedLevelIndex)->noveltyText = newtext;
				}
			}
		}
		if (ww::gui::button_check(deleteLevelButton))
		{
			if (selectedLevelIndex >= 0)
			{
				char message[128];
				
				sprintf(message,"Are you sure you want to delete the level\n\n\"%s\"?",testAdventure.levels.at(selectedLevelIndex)->name);
				if (ww::gui::show_YESNOdialog(message,"????????????????",9))
				{
					delete testAdventure.levels.at(selectedLevelIndex);
					testAdventure.levels.erase(testAdventure.levels.begin()+selectedLevelIndex);
					ww::gui::listbox_deletestring(levelListBox,selectedLevelIndex);
					if (ww::gui::listbox_getstringcount(levelListBox) == 0)
					{
						ww::gui::listbox_setselectedindex(levelListBox,-1);
						selectedLevelIndex = -1;
					}
				}
			}
		}
		if (ww::gui::button_check(addLevelButton))
		{
			int numlevels = testAdventure.levels.size();
			bool canCreate = true;
			for(int i=0;i<numlevels;i++)
			{
				if (strcmp(testAdventure.levels.at(i)->name,"[new level]") == 0)
					canCreate = false;
			}
			if (canCreate)
			{
				char *newname = new char[32];
				sprintf(newname,"%s","[new level]");
				testAdventure.addNewLevel(newname);
				ww::gui::listbox_addstring(levelListBox,newname);
			}
			else
			{
				char message[128];
				sprintf(message,"There is already a level with the name:\n\n\"%s\"","[new level]");
				ww::gui::show_OKdialog(message,"????????????????",1);
			}
		}
		if (ww::gui::button_check(levelNameSetButton) && selectedLevelIndex >= 0)
		{
			char newname[128];
			memset(newname,0,128);
			ww::gui::textbox_gettext(levelNameText,newname,64);

			int numlevels = testAdventure.levels.size();
			bool canCreate = true;
			printf("Testing for level named %s\n",newname);
			for(int i=0;i<numlevels;i++)
			{
				printf("\t[%i] %s\n",i,testAdventure.levels.at(i)->name);
				if (strcmp(testAdventure.levels.at(i)->name,newname) == 0)
				{
					printf("\t\tNOPE! This is a match. We can't change the name.\n");
					canCreate = false;
				}
			}
			if (canCreate)
			{
				printf("SET????\n");
				printf("Replacing \"%s\" with \"%s\".\n",testAdventure.levels.at(selectedLevelIndex)->name,newname);
				delete testAdventure.levels.at(selectedLevelIndex)->name;
				testAdventure.levels.at(selectedLevelIndex)->name = new char[strlen(newname)+1];
				strcpy(testAdventure.levels.at(selectedLevelIndex)->name,newname);
				ww::gui::listbox_setstring(levelListBox,selectedLevelIndex,testAdventure.levels.at(selectedLevelIndex)->name);
			}
			else
			{
				char message[128];
				sprintf(message,"There is already a level with the name:\n\n\"%s\"",newname);
				ww::gui::show_OKdialog(message,"????????????????",1);
			}
		}
		/*if (ww::gui::button_check(levelNoveltySetButton) && selectedLevelIndex >= 0)
		{
			char newname[33];
			memset(newname,0,33);
			ww::gui::textbox_gettext(levelNoveltyText,newname,32);

			delete testAdventure.levels.at(selectedLevelIndex)->noveltyText;
			testAdventure.levels.at(selectedLevelIndex)->noveltyText = new char[strlen(newname)+1];
			strcpy(testAdventure.levels.at(selectedLevelIndex)->noveltyText,newname);
		}*/
		noveltyLengthBefore = noveltyLength;
		selectedLevelIndexBefore = selectedLevelIndex;
	}
	if (ww::gui::controlgroup_getvisible(tilesGroup))
	{
		int mouse_x = ww::input::mouse::getX();
		int mouse_y = ww::input::mouse::getY();
		bool validLocation = false;
		if (ww::gfx::supportsOpenGL2())
		{
			glm::vec2 fixed = ww::etc::inv_crtcoord(mouse_x/(float)window_width,mouse_y/(float)window_height);
			if (!(fixed.x < 0.f || fixed.y < 0.f))
			{
				mouse_x = (int)(256*fixed.x);
				mouse_y = (int)(192*fixed.y);
				validLocation = true;
			}
		}
		else
		{
			mouse_x = 256.f * (mouse_x/(float)window_width);
			mouse_y = 192.f * (mouse_y/(float)window_height);
			if (mouse_x >= 0 && mouse_x < 256 && mouse_y >= 0 && mouse_y < 192)
				validLocation = true;
		}
		if (validLocation)
		{
			int selTile = ww::gui::listview_get_selected(tilesView);
			if (selectedLevelIndex >= 0)
			{
				if (ww::input::mouse::isButtonDown(ww::input::mouse::LEFT) && mouse_y/8 < 23)
				{
					if (selTile == LDTV_GROUND)
						testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_GROUND);
					if (selTile == LDTV_SNOW)
						testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_SNOW);
					if (selTile == LDTV_WOOD)
						testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_WOOD);
					if (selTile == LDTV_LAVA)
						testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_LAVA);
					if (selTile == LDTV_LAVATOP)
						testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_LAVA_TOP);
				}
				if (ww::input::mouse::isButtonDown(ww::input::mouse::RIGHT) && mouse_y/8 < 23)// && (ww::input::keyboard::isKeyDown(ww::input::key::LShift) || ww::input::keyboard::isKeyDown(ww::input::key::RShift)))
					testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_NOTHING);
			}
		}
	}
	if (ww::gui::controlgroup_getvisible(colorsGroup))
	{
		int mouse_x = ww::input::mouse::getX();
		int mouse_y = ww::input::mouse::getY();
		bool validLocation = false;
		if (ww::gfx::supportsOpenGL2())
		{
			glm::vec2 fixed = ww::etc::inv_crtcoord(mouse_x/(float)window_width,mouse_y/(float)window_height);
			if (!(fixed.x < 0.f || fixed.y < 0.f))
			{
				mouse_x = (int)(256*fixed.x);
				mouse_y = (int)(192*fixed.y);
				validLocation = true;
			}
		}
		else
		{
			mouse_x = 256.f * (mouse_x/(float)window_width);
			mouse_y = 192.f * (mouse_y/(float)window_height);
			if (mouse_x >= 0 && mouse_x < 256 && mouse_y >= 0 && mouse_y < 192)
				validLocation = true;
		}
		if (validLocation)
		{
			if (selectedLevelIndex >= 0)
			{
				if (ww::input::mouse::isButtonDown(ww::input::mouse::LEFT))
				{
					int selTile = ww::gui::listview_get_selected(colorView);
					testAdventure.levels.at(selectedLevelIndex)->setColor(mouse_x/8,mouse_y/8,selTile);
				}
				if (ww::input::mouse::isButtonDown(ww::input::mouse::RIGHT))
				{
					int selTile = ww::gui::listview_get_selected(colorView);
					testAdventure.levels.at(selectedLevelIndex)->setColor(mouse_x/8,mouse_y/8,ZX_BLACK);
				}
			}
		}
	}






































	if (ww::input::keyboard::isKeyPressed(ww::input::key::Num1))
		song1.getSound()->play();
	if (ww::input::keyboard::isKeyPressed(ww::input::key::Num2))
		song2.getSound()->play();
	if (ww::input::keyboard::isKeyPressed(ww::input::key::Num3))
		song3.getSound()->play();
	if (!ww::gfx::supportsOpenGL2())
	{
		glViewport(0,0,window_width,window_height);
		glScissor(0,0,window_width,window_height);
	}
	defaultShader->makeActive();
	ww::gfx::setMatrix(defaultShader,defaultVertexMatrixID,glm::value_ptr(glm::ortho(0.f,256.f,192.f,0.f,-1.f,1.f)));
	glClearColor(0.f,0.f,0.f,1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D,supertex.getTexture());
	//exampleBatch.draw();

	ww::gfx::VertexBatch *scratch = new ww::gfx::VertexBatch();
	// DRAW LEVEL TILES
	static int lavaFrame = 0;
	lavaFrame++;
	if (lavaFrame >= 8*8)
		lavaFrame -= 8*8;
	scratch->clear();
	if (selectedLevelIndex >= 0)
	{
		for(int i=0;i<32*23;i++)
		{
			int x = i%32;
			int y = i/32;
			LD31Tile *t = &testAdventure.levels.at(selectedLevelIndex)->tiles[x][y];
			if (t->type == TILE_GROUND)
				batch_add_sprite(scratch,sprTileGround,8*x,8*y);
			if (t->type == TILE_SNOW)
				batch_add_sprite(scratch,sprTileSnow,8*x,8*y);
			if (t->type == TILE_WOOD)
				batch_add_sprite(scratch,sprTileWood,8*x,8*y);
			if (t->type == TILE_LAVA)
			{
				batch_add_sprite(scratch,sprTileLava[lavaFrame/8],8*x,8*y);
				testAdventure.levels.at(selectedLevelIndex)->setColor(x,y,lavaFrame>=32?ZX_LTRED:ZX_LTYELLOW);
			}
			if (t->type == TILE_LAVA_TOP)
			{
				batch_add_sprite(scratch,sprTileLavaTop[4*(x%2)+lavaFrame/8],8*x,8*y);
				testAdventure.levels.at(selectedLevelIndex)->setColor(x,y,lavaFrame>=32?ZX_LTRED:ZX_LTYELLOW);
			}

			if (tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x,y)))
			{
				unsigned char S = !tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x+1,y))
									+ 2*!tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x,y-1))
									+ 4*!tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x-1,y))
									+ 8*!tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x,y+1));
				if (S > 0)
					batch_add_sprite(scratch,sprTileBorder[S],8*x,8*y);
			}
		}
		
	}
	scratch->markAsDirty();	
	scratch->draw();

	// DRAW THE WORD (not bird)
	scratch->clear();
	if (selectedLevelIndex >= 0)
		draw_fancy_text(scratch,0,23,testAdventure.levels.at(selectedLevelIndex)->noveltyText,ZX_LTGRAY);
	scratch->markAsDirty();
	scratch->draw();
	delete scratch;


	updateColors();
	ww::gfx::setBlendMode(ww::gfx::BM_MULTIPLY);
	glBindTexture(GL_TEXTURE_2D, FGCOL);
	colorBatch.draw();
	ww::gfx::setBlendMode(ww::gfx::BM_NORMAL);
	return true;
}

bool dorun()
{
	bool ret = false;
	if (ww::gfx::supportsOpenGL2())// && !ww::input::keyboard::isKeyDown(ww::input::key::LShift))
	{
		myCRT->begin();
			glViewport(0,0,256,192);
			glScissor(0,0,256,192);
			ret = dogame();
		myCRT->end();

		glViewport(0,0,window_width,window_height);
		glScissor(0,0,window_width,window_height);
		glClearColor(0.f,0.f,0.f,1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		defaultShader->makeActive();
		defaultShader->setMatrix(defaultVertexMatrixID,(float*)glm::value_ptr(glm::ortho(0.f,1.f,1.f,0.f,-1.f,1.f)));
		myCRT->draw();
	}
	else
		ret = dogame();
	return ret;
}

bool doend()
{
	//ww::terminate();
	printf("doend\n");
	return true;
}