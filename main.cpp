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
					//if (x != prevX)
					//	printf("BLACK @ %i, %i\n",x,y);
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
	short width;
	short height;
	LD31Sprite()
	{
	}
	LD31Sprite(ww::gfx::Texture *tex, unsigned int width, unsigned int height, unsigned int u, unsigned int v, unsigned int w, unsigned int h)
	{
		this->width = width;
		this->height = height;
		float u1 = (/*.5f+*/(float)u) / (float)tex->getWidth();
		float v1 = (/*.5f+*/(float)v) / (float)tex->getHeight();
		float u2 = (/*-.5f+*/(float)(u+w)) / (float)tex->getWidth();
		float v2 = (/*-.5f+*/(float)(v+h)) / (float)tex->getHeight();

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

#define SPRITETILE_EXT2(X,Y,W,H,W2,H2) LD31Sprite(&supertex,8*(W2),8*(H2),8*(X),8*(Y),8*(W),8*(H))

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

unsigned int window_width = 512;
unsigned int window_height = 384;

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

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)//main(int argc, char *argv[])
{
	ww::gui::initialize();
	int window = ww::gui::window_create(0,320,320,"SETTINGS");
	ww::gui::text_create(window,16,16,64,32,"Game Width:");
	int widthbox = ww::gui::textbox_create(window,80,12,48,24,true,false,false,false,true);
	int widthupdown = ww::gui::updown_create(window,80+48,12,16,24);

	ww::gui::text_create(window,160,16,80,32,"Game Height:");
	int heightbox = ww::gui::textbox_create(window,230,12,48,24,true,false,false,false,true);
	int heightupdown = ww::gui::updown_create(window,230+48,12,16,24);

	ww::gui::text_create(window,32,96,320-64,120-64,"Choose ADVANCED for special GFX or if you are special.\n\nChoose SIMPLE if your computer is old and or slow.");

	ww::gui::text_create(window,32,240,320-64,96,"CONTROLS:\n\nLeft/Right - Move\nUp - Jump\nA - Action\n\nF5 to enter/exit editor.             No cheating!");

	int _1x = ww::gui::button_create(window,32,52,48,24,"1X");
	int _2x = ww::gui::button_create(window,96,52,48,24,"2X");
	int _3x = ww::gui::button_create(window,160,52,48,24,"3X");
	int _4x = ww::gui::button_create(window,224,52,48,24,"4X");

	int u = 0;
	int simple = ww::gui::button_create(window,32,180,120,48,"SIMPLE");
	int advanced = ww::gui::button_create(window,320-32-120,180,120,48,"ADVANCED");
	while(1)
	{
		ww::gui::handle_messages();
		if (!ww::gui::window_isopen(window))
			return 0;
		window_width -= ww::gui::updown_get_delta(widthupdown);
		window_height -= ww::gui::updown_get_delta(heightupdown);
		if (window_width < 256)
			window_width = 256;
		if (window_height < 192)
			window_height = 192;
		char bob[120];
		sprintf(bob,"%i",window_width);
		ww::gui::textbox_settext(widthbox,bob);
		sprintf(bob,"%i",window_height);
		ww::gui::textbox_settext(heightbox,bob);
		if (ww::gui::button_check(_1x))
		{
			window_width = 256;
			window_height = 192;
		}
		if (ww::gui::button_check(_2x))
		{
			window_width = 256*2;
			window_height = 192*2;
		}
		if (ww::gui::button_check(_3x))
		{
			window_width = 256*3;
			window_height = 192*3;
		}
		if (ww::gui::button_check(_4x))
		{
			window_width = 256*4;
			window_height = 192*4;
		}
		if (ww::gui::button_check(simple))
		{
			u = ww::sys::CONFIG_OPENGL1;
			break;
		}
		if (ww::gui::button_check(advanced))
		{
			u = ww::sys::CONFIG_OPENGL2;
			break;
		}
		Sleep(30);
	}
	ww::gui::hide_control(window);
	ww::gfx::setWindowSize(window_width,window_height);
	ww::sys::setInitCallback(doinit);
	ww::sys::setTimerCallback(dorun);
	ww::sys::setDeinitCallback(doend);
	ww::sys::setTimerResolution(30);
	//ww::setForegroundCallback(NULL);
	//ww::setBackgroundCallback(NULL);
	return ww::sys::setup(u | ww::sys::CONFIG_DISABLE_OPENGL_DEPTHBUFFER);
}

bool EDITOR = false;

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

const unsigned char TILE_NOTHING = 0;
const unsigned char TILE_GROUND = 1;
const unsigned char TILE_SNOW = 2;
const unsigned char TILE_WOOD = 3;
const unsigned char TILE_LAVA_TOP = 4;
const unsigned char TILE_LAVA = 5;
const unsigned char TILE_TITLE = 6;
const unsigned char TILE_TRISTONE = 7;

const unsigned char OBJ_PLAYER = 1;
const unsigned char OBJ_BOULDER = 2;
const unsigned char OBJ_DOOR = 3;
const unsigned char OBJ_CAKE = 4;
const unsigned char OBJ_TELEPHONE = 5;

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
LD31Sprite *font[16*6];
LD31Sprite *sprTileGround;
LD31Sprite *sprTileSnow;
LD31Sprite *sprTileWood;
LD31Sprite *sprTileLavaTop[16];
LD31Sprite *sprTileLava[8];
LD31Sprite *sprTileBorder[16];
LD31Sprite *sprTileTriStone[4];
LD31Sprite *sprTileTitle;

LD31Sprite *sprSelectedObject;

LD31Sprite *sprPlayer;
LD31Sprite *sprBoulder;
LD31Sprite *sprDoor;
LD31Sprite *sprCake;
LD31Sprite *sprTelephone;

LD31Sprite *sprPenguin[4];
LD31Sprite *sprPenguinFly[2];
LD31Sprite *sprTelephoneAnim[2];

SongRoller sfx_fall;
SongRoller sfx_jump;
SongRoller sfx_move;
SongRoller sfx_phone;
SongRoller sfx_pickup;
SongRoller sfx_place;
SongRoller sfx_wingame;
SongRoller sfx_wingame_intro;


class LD31Object
{
public:
	unsigned char type;
	short x;
	short y;
	char *str1;
	char *str2;
	char *str3;
	char *str4;

	short width, height;
	LD31Object()
	{
		str1 = str2 = str3 = str4 = NULL;
	}
	~LD31Object()
	{
		delete str1;
		delete str2;
		delete str3;
		delete str4;
	}
};

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
	char *leftExit;
	char *rightExit;
	char *upExit;
	char *downExit;
	std::vector<LD31Object*> objects;
	LD31Tile tiles[32][24];
	unsigned int COLORS[32*32];
	LD31LevelData()
	{
		leftExit = NULL;
		rightExit = NULL;
		upExit = NULL;
		downExit = NULL;
		for(int i=0;i<32*24;i++)
			tiles[i%32][i/32].type = 0;
	}
	~LD31LevelData()
	{
		while(objects.size())
		{
			delete objects.at(0);
			objects.erase(objects.begin());
		}
	}
		
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

// ONLY WORKS FOR SIZE 2
unsigned char hex2uchar(char *hex)
{
	unsigned char ret = 0;

	if (hex[0] >= '0' && hex[0] <= '9')
		ret += 16 * (hex[0]-'0');
	if (hex[0] >= 'A' && hex[0] <= 'F')
		ret += 16 * (hex[0]-'A');
	if (hex[0] >= 'a' && hex[0] <= 'f')
		ret += 16 * (hex[0]-'a');

	if (hex[1] >= '0' && hex[1] <= '9')
		ret += (hex[1]-'0');
	if (hex[1] >= 'A' && hex[1] <= 'F')
		ret += (hex[1]-'A');
	if (hex[1] >= 'a' && hex[1] <= 'f')
		ret += (hex[1]-'a');
	return ret;
}

void uchar2hex(unsigned char u, char *dest)
{
	char hex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	dest[0] = hex[(u & 0xF0) >> 4];
	dest[1] = hex[u & 0x0F];
}

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
	char *allocXMLcstring(rapidxml::xml_node<char> *node)
	{
		char *ret = NULL;
		if (node == NULL)
			ret = NULL;
		else
		{
			int l = node->value_size();
			if (l > 0)
			{
				ret = new char[node->value_size()+1];
				sprintf(ret,node->value());
			}
			else
				ret = NULL;
		}
		return ret;
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
					
					data->name = allocXMLcstring(namenode);
					data->noveltyText = allocXMLcstring(xmllevel->first_node("novelty"));

					data->leftExit = allocXMLcstring(xmllevel->first_node("leftExit"));
					data->rightExit = allocXMLcstring(xmllevel->first_node("rightExit"));
					data->upExit = allocXMLcstring(xmllevel->first_node("upExit"));
					data->downExit = allocXMLcstring(xmllevel->first_node("downExit"));
					
					rapidxml::xml_node<char> *colorsnode = xmllevel->first_node("colors");
					if (colorsnode != NULL)
					{
						if (colorsnode->value_size() < 32*24)
						{
							printf("Level has insuffient colordata! Skipping.\n");
							delete data;
							continue;
						}
						char *cols = colorsnode->value()+2; // the 2 is \r\n
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
						char *tiledata = tilesnode->value();
						char *dest = tiledata+2; // I DON'T KNOW WHAT THAT OFF BY TWO BYTES IS!         YES I DO! IT'S THE \r\n !!!!!!!!!!
						for(int y=0;y<23;y++)
						{
							for(int x=0;x<32;x++)
							{
								data->tiles[x][y].type = hex2uchar(dest);
								//printf("TILE %i %i - %i\n",x,y,data->tiles[x][y].type);
								dest += 2;
							}
						}
						//for(rapidxml::xml_node<char> *tile = tilesnode->first_node("tile");tile != NULL;tile = tile->next_sibling("tile"))
						//	data->setTileID(atoi(tile->first_attribute("x")->value()),atoi(tile->first_attribute("y")->value()),atoi(tile->first_attribute("id")->value()));
					}
					else
					{
						printf("Level has no tiledata! Skipping.\n");
						delete data;
						continue;
					}
					rapidxml::xml_node<char> *objectsnode = xmllevel->first_node("objects");
					if (objectsnode != NULL)
					{
						for(rapidxml::xml_node<char> *object = objectsnode->first_node("object");object != NULL;object = object->next_sibling("object"))
						{
							LD31Object *ldobject = new LD31Object();
							ldobject->type = atoi(object->first_attribute("id")->value());
							ldobject->x = atoi(object->first_attribute("x")->value());
							ldobject->y = atoi(object->first_attribute("y")->value());
							ldobject->width = atoi(object->first_attribute("width")->value());
							ldobject->height = atoi(object->first_attribute("height")->value());

							int sz = 0;
							
							sz = object->first_attribute("str1")->value_size();
							if (sz > 0)
							{
								char *str = new char[sz + 1];
								strcpy(str,object->first_attribute("str1")->value());
								ldobject->str1 = str;
							}
							sz = object->first_attribute("str2")->value_size();
							if (sz > 0)
							{
								char *str = new char[sz + 1];
								strcpy(str,object->first_attribute("str2")->value());
								ldobject->str2 = str;
							}
							sz = object->first_attribute("str3")->value_size();
							if (sz > 0)
							{
								char *str = new char[sz + 1];
								strcpy(str,object->first_attribute("str3")->value());
								ldobject->str3 = str;
							}
							sz = object->first_attribute("str4")->value_size();
							if (sz > 0)
							{
								char *str = new char[sz + 1];
								strcpy(str,object->first_attribute("str4")->value());
								ldobject->str4 = str;
							}
							data->objects.push_back(ldobject);
						}
					}
					else
					{
						printf("Level has no objectdata! Skipping.\n");
						//delete data;
						//continue;
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

			fprintf(f,"\t\t\t<leftExit>%s</leftExit>\r\n",data->leftExit?data->leftExit:"");
			fprintf(f,"\t\t\t<rightExit>%s</rightExit>\r\n",data->rightExit?data->rightExit:"");
			fprintf(f,"\t\t\t<upExit>%s</upExit>\r\n",data->upExit?data->upExit:"");
			fprintf(f,"\t\t\t<downExit>%s</downExit>\r\n",data->downExit?data->downExit:"");




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
			fprintf(f,"\t\t\t<colors>\r\n%s\r\n</colors>\r\n",colorData);
			delete colorData;




			fprintf(f,"\t\t\t<tiles>\r\n");
				char *tiledata = new char[32*23*2 + 1];
				memset(tiledata,0,32*23*2 + 1);
				char *dest = tiledata;
				for(int y=0;y<23;y++)
					for(int x=0;x<32;x++)
					{
						uchar2hex(data->tiles[x][y].type,dest);
						dest += 2;
					}
					fprintf(f,"%s",tiledata);
					//fprintf(f,"\t\t\t\t<tile id=\"%i\" x=\"%i\" y=\"%i\" />\r\n",
					//	data->tiles[x][y].type,x,y);//,data->tiles[x][y].num1,data->tiles[x][y].num2,data->tiles[x][y].num3,data->tiles[x][y].str1,data->tiles[x][y].str2,data->tiles[x][y].str3);
			fprintf(f,"\t\t\t</tiles>\r\n");
			delete tiledata;




			fprintf(f,"\t\t\t<objects>\r\n");
			for(int i=0;i<data->objects.size();i++)
				fprintf(f,"\t\t\t\t<object id=\"%i\" x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" str1=\"%s\" str2=\"%s\" str3=\"%s\" str4=\"%s\"/>\r\n",
								data->objects.at(i)->type,
								data->objects.at(i)->x,
								data->objects.at(i)->y,
								data->objects.at(i)->width,
								data->objects.at(i)->height,
								data->objects.at(i)->str1?data->objects.at(i)->str1:"",
								data->objects.at(i)->str2?data->objects.at(i)->str2:"",
								data->objects.at(i)->str3?data->objects.at(i)->str3:"",
								data->objects.at(i)->str4?data->objects.at(i)->str4:"");
			fprintf(f,"\t\t\t</objects>\r\n");
			fprintf(f,"\t\t</level>\r\n");
		}
		fprintf(f,"\t</levels>\r\n");
		fprintf(f,"</adventure>\r\n");
		fclose(f);
	}
};

LD31Adventure testAdventure;

























class Entity;
class Penguin;
class Game
{
public:
	int currentLevelID;
	std::vector<Entity*> entities;
	Penguin *penguin;
};
Game *gamesingleton = new Game();


void startGame();





int toolWindow;
int tabs, tabLevels, tabLevelInfo, tabObjects, tabTiles, tabColors;
int fileGroup, levelInfoGroup, objectsGroup, tilesGroup, colorsGroup;
// FILE
int adventurePathText, newButton, openButton, saveButton, saveAsButton,
levelListBox, addLevelButton, deleteLevelButton;
// LEVELINFO
int levelNameText, levelNameSetButton, levelNoveltyText, levelNoveltySetButton,
	levelLEFTtext, levelUPtext, levelRIGHTtext, levelDOWNtext;
// OBJECTS
int objectsView;
// SEL OBJ
int selObjectGroup;
int selObjectDeleteButton, selObjectString1, selObjectString2, selObjectString3, selObjectString4;
int selObjectLeftButton, selObjectUpButton, selObjectRightButton, selObjectDownButton;
// TILES
int tilesView, LDTV_GROUND, LDTV_SNOW, LDTV_WOOD, LDTV_LAVATOP, LDTV_LAVA, LDTV_TRISTONE, LDTV_TITLE;
// COLORS
int colorView;

bool doinit()
{
	sfx_fall.load("sfx/sfx_fall.png");
	sfx_move.load("sfx/sfx_move.png");
	sfx_phone.load("sfx/sfx_phone.png");
	sfx_pickup.load("sfx/sfx_pickup.png");
	sfx_place.load("sfx/sfx_place.png");
	sfx_wingame.load("sfx/sfx_wingame.png");
	sfx_wingame_intro.load("sfx/sfx_wingame_intro.png");
	sfx_jump.load("sfx/sfx_jump.png");

	toolWindow = ww::gui::window_create(1,256,384,"Editor");

	tabs = ww::gui::tabcontroller_create(toolWindow,8,8,240,368);
	tabLevels = ww::gui::tabcontroller_addtab(tabs,0,"Levels");
	tabLevelInfo = ww::gui::tabcontroller_addtab(tabs,1,"Level Info");
	tabObjects = ww::gui::tabcontroller_addtab(tabs,2,"Objects");
	tabTiles = ww::gui::tabcontroller_addtab(tabs,3,"Tiles");
	tabColors = ww::gui::tabcontroller_addtab(tabs,4,"Colors");
	//int tabInfo = ww::gui::tabcontroller_addtab(tabs,3,"Info");

	fileGroup = ww::gui::controlgroup_create();
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
		levelListBox = ww::gui::listbox_create(tabs,16,124,208,216);
			ww::gui::controlgroup_addcontrol(fileGroup,levelListBox);
		addLevelButton = ww::gui::button_create(tabs,16,124+224-12,48,24,"Add");
			ww::gui::controlgroup_addcontrol(fileGroup,addLevelButton);
		deleteLevelButton = ww::gui::button_create(tabs,16+208-48,124+224-12,48,24,"Delete");
			ww::gui::controlgroup_addcontrol(fileGroup,deleteLevelButton);
		//levelNoveltySetButton = ww::gui::button_create(tabs,32+176-32,272+24+40,32,20,"Set");
		//	ww::gui::controlgroup_addcontrol(fileGroup,levelNoveltySetButton);
		//
	ww::gui::controlgroup_setvisible(fileGroup,true);


	levelInfoGroup = ww::gui::controlgroup_create();
		ww::gui::controlgroup_addcontrol(levelInfoGroup,ww::gui::groupbox_create(tabs,16,24,208,300,"Level Info:"));
		ww::gui::controlgroup_addcontrol(levelInfoGroup,ww::gui::text_create(tabs,32,48,128,20,"Level Name:"));
		levelNameText = ww::gui::textbox_create(tabs,32,48+16,176-32,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelNameText,20);
			ww::gui::controlgroup_addcontrol(levelInfoGroup,levelNameText);
		levelNameSetButton = ww::gui::button_create(tabs,32+176-32,48+16,32,20,"Set");
			ww::gui::controlgroup_addcontrol(levelInfoGroup,levelNameSetButton);

		ww::gui::controlgroup_addcontrol(levelInfoGroup,ww::gui::text_create(tabs,32,92,128,20,"Novelty Text:"));
		levelNoveltyText = ww::gui::textbox_create(tabs,32,88+20,176,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelNoveltyText,32);
			ww::gui::controlgroup_addcontrol(levelInfoGroup,levelNoveltyText);

		ww::gui::controlgroup_addcontrol(levelInfoGroup,ww::gui::text_create(tabs,32,136,128,20,"Left Exit:"));
		levelLEFTtext = ww::gui::textbox_create(tabs,32,136+16,176,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelLEFTtext,20);
			ww::gui::controlgroup_addcontrol(levelInfoGroup,levelLEFTtext);

		ww::gui::controlgroup_addcontrol(levelInfoGroup,ww::gui::text_create(tabs,32,136+40,128,20,"Right Exit:"));
		levelRIGHTtext = ww::gui::textbox_create(tabs,32,136+16+40,176,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelRIGHTtext,20);
			ww::gui::controlgroup_addcontrol(levelInfoGroup,levelRIGHTtext);

		ww::gui::controlgroup_addcontrol(levelInfoGroup,ww::gui::text_create(tabs,32,136+40+40,128,20,"Up Exit:"));
		levelUPtext = ww::gui::textbox_create(tabs,32,136+16+40+40,176,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelUPtext,20);
			ww::gui::controlgroup_addcontrol(levelInfoGroup,levelUPtext);

		ww::gui::controlgroup_addcontrol(levelInfoGroup,ww::gui::text_create(tabs,32,136+40+40+40,128,20,"Down Exit:"));
		levelDOWNtext = ww::gui::textbox_create(tabs,32,136+16+40+40+40,176,20,false,false,true,false,true);
			ww::gui::textbox_setmaxlength(levelDOWNtext,20);
			ww::gui::controlgroup_addcontrol(levelInfoGroup,levelDOWNtext);

	ww::gui::controlgroup_setvisible(levelInfoGroup,false);

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
		LDTV_TRISTONE = ww::gui::listview_additem(tilesView,"TRISTONE",ww::gui::imagelist_addimage(tilesImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,32,88,8,8)));
		LDTV_TITLE = ww::gui::listview_additem(tilesView,"TITLE",ww::gui::imagelist_addimage(tilesImgList,ww::gui::image_createfromptrsubrect(texPtr,texW,texH,24,88,8,8)));
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
		objectsView = ww::gui::listview_create(tabs,12,32,216,112,false);
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

	selObjectGroup = ww::gui::controlgroup_create();
		ww::gui::controlgroup_addcontrol(selObjectGroup,ww::gui::groupbox_create(tabs,16,152,208,208,"Object Info"));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectDeleteButton = ww::gui::button_create(tabs,24,328,192,24,"Delete Object"));
		ww::gui::controlgroup_addcontrol(selObjectGroup,ww::gui::text_create(tabs,24,152+20,192,20,"Strings:"));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectString1 = ww::gui::textbox_create(tabs,24,152+20+16,192,20,false,false,true,false,true));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectString2 = ww::gui::textbox_create(tabs,24,152+20+16+24,192,20,false,false,true,false,true));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectString3 = ww::gui::textbox_create(tabs,24,152+20+16+24+24,192,20,false,false,true,false,true));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectString4 = ww::gui::textbox_create(tabs,24,152+20+16+24+24+24,192,20,false,false,true,false,true));
		ww::gui::controlgroup_addcontrol(selObjectGroup,ww::gui::text_create(tabs,24,152+20+16+24+24+24+24,32,24,"Move:"));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectLeftButton = ww::gui::button_create(tabs,24+48,152+20+16+24+24+24+24,24,24,"<"));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectUpButton = ww::gui::button_create(tabs,24+48+32,152+20+16+24+24+24+24,24,24,"/\\"));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectRightButton = ww::gui::button_create(tabs,24+48+32+32,152+20+16+24+24+24+24,24,24,">"));
		ww::gui::controlgroup_addcontrol(selObjectGroup,selObjectDownButton = ww::gui::button_create(tabs,24+48+32+32+32,152+20+16+24+24+24+24,24,24,"\\/"));
	ww::gui::controlgroup_setvisible(selObjectGroup,false);

	printf("Button handle is [%i] %i\n",selObjectDeleteButton,ww::gui::get_handle(selObjectDeleteButton));

	ww::gfx::Texture palette;
	palette.load("gfx/palette.png");
	unsigned int w = palette.getWidth();
	unsigned int h = palette.getHeight();
	for(int i=0;i < 16;i++)
		ZX_COLORS[i] = palette.getBGRPixel(24 * (i%8),24 * (1-i/8));


	ww::gui::hide_control(toolWindow);

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
		fancyfont[i] = new SPRITETILE(16+i%16,8+i/16,1,1);
	for(int i=0;i<16*6;i++)
		font[i] = new SPRITETILE(16+i%16,i/16,1,1);
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

	for(int i=0;i<4;i++)
		sprTileTriStone[i] = new SPRITETILE(2+i%2,9+i/2,1,1);
	sprTileTitle = new SPRITETILE(0,20,32,12);


	sprPlayer = new SPRITETILE(0,0,2,2);
	sprBoulder = new SPRITETILE(2,0,2,2);
	sprDoor = new SPRITETILE(4,0,2,2);
	sprCake = new SPRITETILE(6,0,2,2);
	sprTelephone = new SPRITETILE(8,0,2,2);

	sprSelectedObject = new SPRITETILE_EXT2(0+(15%4),12+(15/4),1,1,2,2);

	sprPenguin[0] = new SPRITETILE(0,0,2,2);
	sprPenguin[1] = new SPRITETILE(0,2,2,2);
	sprPenguin[2] = new SPRITETILE(0,0,2,2);
	sprPenguin[3] = new SPRITETILE(0,4,2,2);

	sprPenguinFly[0] = new SPRITETILE(0,6,2,2);
	sprPenguinFly[1] = new SPRITETILE(0,8,2,2);

	sprTelephoneAnim[0] = new SPRITETILE(8,0,2,2);
	sprTelephoneAnim[1] = new SPRITETILE(10,0,2,2);

	testAdventure.loadFromFile("test.xml");
	startGame();

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

void draw_text(ww::gfx::VertexBatch *dest, int x, int y, char *text, unsigned char color)
{
	if (text == NULL)
		return;
	if (color > 15)
		return;
	int len = strlen(text);
	for(int i=0;i<len;i++)
	{
		batch_add_sprite(dest,font[text[i]-' '],8*(x+i),8*y);
		SET_COLOR(x+i,y,color);
	}
}


int selectedLevelIndex;
LD31Object *selectedObject = NULL;

namespace ww
{
	namespace etc
	{
		extern glm::vec2 inv_crtcoord(float x, float y);
	}
}

bool doeditor();

class Entity
{
public:
	int x, y;
	int type;
	ww::Rectanglei hitbox;
public:
	Entity()
	{

	}
	virtual void begin() {}
	virtual void update() {}
	virtual void draw(ww::gfx::VertexBatch *batch) {}
	virtual void end() {}
	Entity *collides(int xx, int yy, int type)
	{
		hitbox.x = xx;
		hitbox.y = yy;
		for(int i=0;i<gamesingleton->entities.size();i++)
		{
			Entity *e = gamesingleton->entities.at(i);
			if (e->type == type && e != this)
			{
				e->hitbox.x = e->x;
				e->hitbox.y = e->y;
				if (hitbox.intersects(e->hitbox))
					return e;
			}
		}
		return NULL;
	}
	bool collidesTerrain(int xx, int yy)
	{
		hitbox.x = xx;
		hitbox.y = yy;
		int x1 = (hitbox.x+hitbox.xoffset)/8;
		int x2 = (hitbox.x+hitbox.xoffset+hitbox.width)/8;
		int y1 = (hitbox.y+hitbox.yoffset)/8;
		int y2 = (hitbox.y+hitbox.yoffset+hitbox.height)/8;
		for(int i = x1; i <= x2; i++)
			for(int j = y1; j <= y2; j++)
			{
				if (tileIsSolid(testAdventure.levels.at(gamesingleton->currentLevelID)->getTileID(i,j)))
				{
					if (hitbox.intersects(ww::Rectanglei(8*i,8*j,8,8)))
						return true;
				}
			}
		return false;		
	}
	bool collidesTile(int xx, int yy, int tileID)
	{
		hitbox.x = xx;
		hitbox.y = yy;
		int x1 = (hitbox.x+hitbox.xoffset)/8;
		int x2 = (hitbox.x+hitbox.xoffset+hitbox.width)/8;
		int y1 = (hitbox.y+hitbox.yoffset)/8;
		int y2 = (hitbox.y+hitbox.yoffset+hitbox.height)/8;
		for(int i = x1; i <= x2; i++)
			for(int j = y1; j <= y2; j++)
			{
				if (testAdventure.levels.at(gamesingleton->currentLevelID)->getTileID(i,j) == tileID)
				{
					if (hitbox.intersects(ww::Rectanglei(8*i,8*j,8,8)))
						return true;
				}
			}
		return false;
	}
};

int farrrrr[] = {	0,0,0,0,0,0,
					0,0,0,0,0,0,
					0,0,0,0,0,0,
					0,0,0,0,0,0,
					1,1,0,1,1,0,
					1,1,0,1,1,0,
					1,1,0,1,1,0};

class Penguin : public Entity
{
public:
	int vspeed;
	int frame;
	LD31Sprite **spr;
	bool canMove;
public:
	Penguin();
	void begin();
	void update();
	void draw(ww::gfx::VertexBatch *batch);
	void end();
};

class Telephone : public Entity
{
public:
	int frame;
	bool talking;
	int *frameArray;
	char *str[4];
	char *currentText;
	int textPos;
	int textID;
public:
	Telephone()
	{
		type = OBJ_TELEPHONE;
		frameArray = new int[sizeof(farrrrr)/sizeof(int)];
		memcpy(frameArray,farrrrr,sizeof(farrrrr));
		hitbox.width = 16;
		hitbox.height = 16;
		frame = 0;
		talking = false;
		str[0] = NULL;
		str[1] = NULL;
		str[2] = NULL;
		str[3] = NULL;
		textID = 0;
	}
	void begin()
	{

	}
	void update()
	{
		frame++;
		if (frame == 48 && !talking)
			sfx_phone.getSound()->play();
		frame = frame % 84;//sizeof(frameArray)/sizeof(int);
	}
	void draw(ww::gfx::VertexBatch *batch)
	{
		batch_add_sprite(batch,sprTelephoneAnim[frameArray[frame/2]],x,y);

		if (talking)
		{
			char b4 = currentText[textPos];
			currentText[textPos] = 0;
			draw_text(batch,16-textPos/2,20,currentText,ZX_LTGRAY);
			currentText[textPos] = b4;
			if (textPos < strlen(currentText))
				textPos++;
		}
	}
	void talk()
	{
		//printf("str[textID (%i)] = %s\n",textID,str[textID]);
		if (textID == 4 || str[textID] == NULL)
		{
			talking = false;
			gamesingleton->penguin->canMove = true;
			textID = 0;
		}
		else
		{
			talking = true;
			textPos = 0;
			currentText = str[textID];
			textID++;
		}
	}
	void end()
	{

	}
};

class Door : public Entity
{
public:
	char *dest;
public:
	Door()
	{
		type = OBJ_DOOR;
		hitbox.width = 16;
		hitbox.height = 16;
		dest = NULL;
	}
	void draw(ww::gfx::VertexBatch *batch)
	{
		batch_add_sprite(batch,sprDoor,x,y);
	}
};

void gotoRoom(char *dest);


Penguin::Penguin()
{
	hitbox.xoffset = 2;
	hitbox.yoffset = 2;
	hitbox.width = 12;
	hitbox.height = 14;
	vspeed = 0;
	frame = 0;
	spr = sprPenguin;
	canMove = true;
}
void Penguin::begin()
{

}
void Penguin::update()
{
	//printf("y = %i\n",y);
	if (!collidesTerrain(x,y+vspeed))
	{
		y += vspeed;
		vspeed += 1;
		if (!collidesTerrain(x,y+1))
		{
			frame++;
			frame = frame % 4;
			spr = sprPenguinFly;
		}
	}
	else
	{
		int s = (vspeed < 0)?-1:1;
		while(!collidesTerrain(x,y+s))
			y += s;
		sfx_fall.getSound()->stop();
		vspeed = 0;
		if (ww::input::keyboard::isKeyDown(ww::input::key::Up) && canMove)
		{
			if (!collidesTerrain(x,y-1))
			{
				sfx_jump.getSound()->play();
				vspeed = -7;
			}
		}
		spr = sprPenguin;
		
	}
	if (ww::input::keyboard::isKeyDown(ww::input::key::Left) && canMove)
	{
		//if (vspeed == 0)
		//	if (collidesTerrain(x,y+1))
		//		if (!collidesTerrain(x-4,y+1))
		//			sfx_fall.getSound()->play();
		if (!collidesTerrain(x-4,y))
			x -= 4;
		if (spr == sprPenguin)
			frame++;
	}
	if (ww::input::keyboard::isKeyDown(ww::input::key::Right) && canMove)
	{
		//if (vspeed == 0)
		//	if (collidesTerrain(x,y+1))
		//		if (!collidesTerrain(x+4,y+1))
		//			sfx_fall.getSound()->play();
		if (!collidesTerrain(x+4,y))
			x += 4;
		if (spr == sprPenguin)
			frame++;
	}		
	if (spr == sprPenguin)
		frame = frame % 8;

	if (ww::input::keyboard::isKeyPressed(ww::input::key::A))
	{
		Telephone *t = (Telephone*)collides(x,y,OBJ_TELEPHONE);
		if (t != NULL)
		{
			canMove = false;
			t->talk();
		}
		else
		{
			Door *d = (Door*)collides(x,y,OBJ_DOOR);
			if (d != NULL)
				gotoRoom(d->dest);
		}
	}

	if (y > 23*8)
	{
		y -= 24*8;
		gotoRoom(testAdventure.levels.at(gamesingleton->currentLevelID)->downExit);
	}

	if (y < -8)
	{
		y += 24*8;
		gotoRoom(testAdventure.levels.at(gamesingleton->currentLevelID)->upExit);
	}

	if (x < -8)
	{
		x += 256;
		gotoRoom(testAdventure.levels.at(gamesingleton->currentLevelID)->leftExit);
	}

	if (x > 31*8)
	{
		x -= 32*8;
		gotoRoom(testAdventure.levels.at(gamesingleton->currentLevelID)->rightExit);
	}
}
void Penguin::draw(ww::gfx::VertexBatch *batch)
{
	batch_add_sprite(batch,spr[frame/2],x,y);
}
void Penguin::end()
{

}


void gotoRoom(char *dest)
{
	if (dest == NULL)
		return;
	for(int i=0;i<testAdventure.levels.size();i++)
	{
		if (strcmp(testAdventure.levels.at(i)->name,dest) == 0)
		{
			gamesingleton->currentLevelID = i;
			while(gamesingleton->entities.size())
			{
				delete gamesingleton->entities.at(0);
				gamesingleton->entities.erase(gamesingleton->entities.begin());
			}
			break;
		}
	}
	for(int i=0;i<testAdventure.levels.at(gamesingleton->currentLevelID)->objects.size();i++)
	{
		if (testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->type == OBJ_PLAYER)
		{
			Penguin *p = new Penguin();
			p->x = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->x;
			p->y = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->y;
			gamesingleton->penguin = p;
		}
		if (testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->type == OBJ_TELEPHONE)
		{
			Telephone *t = new Telephone();
			t->x = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->x;
			t->y = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->y;
			t->str[0] = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->str1;
			t->str[1] = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->str2;
			t->str[2] = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->str3;
			t->str[3] = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->str4;
			gamesingleton->entities.push_back(t);
		}
		if (testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->type == OBJ_DOOR)
		{
			Door *d = new Door();
			d->x = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->x;
			d->y = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->y;
			d->dest = testAdventure.levels.at(gamesingleton->currentLevelID)->objects.at(i)->str1;
			gamesingleton->entities.push_back(d);
		}
	}
	ZXCOLORPTR = testAdventure.levels.at(gamesingleton->currentLevelID)->COLORS;
}

void startGame()
{
	for(int i=0;i<testAdventure.levels.size();i++)
	{
		for(int j=0;j<testAdventure.levels.at(i)->objects.size();j++)
		{
			if (testAdventure.levels.at(i)->objects.at(j)->type == OBJ_PLAYER)
			{
				gotoRoom(testAdventure.levels.at(i)->name);
				i = INT_MAX-2;
				break;
			}
		}
	}
	//gamesingleton->currentLevelID = ;
	ww::gui::hide_control(toolWindow);
}

bool dogame()
{
	if (testAdventure.levels.size() == 0)
		return true;

	for(int i=0;i<gamesingleton->entities.size();i++)
		gamesingleton->entities.at(i)->update();
	if (gamesingleton->penguin != NULL)
		gamesingleton->penguin->update();
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
	for(int i=0;i<32*23;i++)
	{
		int x = i%32;
		int y = i/32;
		LD31Tile *t = &testAdventure.levels.at(gamesingleton->currentLevelID)->tiles[x][y];
		if (t->type == TILE_GROUND)
			batch_add_sprite(scratch,sprTileGround,8*x,8*y);
		if (t->type == TILE_SNOW)
			batch_add_sprite(scratch,sprTileSnow,8*x,8*y);
		if (t->type == TILE_WOOD)
			batch_add_sprite(scratch,sprTileWood,8*x,8*y);
		if (t->type == TILE_LAVA)
		{
			batch_add_sprite(scratch,sprTileLava[lavaFrame/8],8*x,8*y);
			testAdventure.levels.at(gamesingleton->currentLevelID)->setColor(x,y,lavaFrame>=32?ZX_LTRED:ZX_LTYELLOW);
		}
		if (t->type == TILE_LAVA_TOP)
		{
			batch_add_sprite(scratch,sprTileLavaTop[4*(x%2)+lavaFrame/8],8*x,8*y);
			testAdventure.levels.at(gamesingleton->currentLevelID)->setColor(x,y,lavaFrame>=32?ZX_LTRED:ZX_LTYELLOW);
		}
		if (t->type == TILE_TRISTONE)
			batch_add_sprite(scratch,sprTileTriStone[x%2 + 2*(y%2)],8*x,8*y);
		if (t->type == TILE_TITLE)
			batch_add_sprite(scratch,sprTileTitle,8*x,8*y);
		else if (tileIsSolid(testAdventure.levels.at(gamesingleton->currentLevelID)->getTileID(x,y)))
		{
			unsigned char S = !(tileIsSolid(testAdventure.levels.at(gamesingleton->currentLevelID)->getTileID(x+1,y)) || (x == 31))
								+ 2*!(tileIsSolid(testAdventure.levels.at(gamesingleton->currentLevelID)->getTileID(x,y-1)) || (y == 0))
								+ 4*!(tileIsSolid(testAdventure.levels.at(gamesingleton->currentLevelID)->getTileID(x-1,y)) || (x == 0))
								+ 8*!(tileIsSolid(testAdventure.levels.at(gamesingleton->currentLevelID)->getTileID(x,y+1)) || (y == 22));
			if (S > 0)
				batch_add_sprite(scratch,sprTileBorder[S],8*x,8*y);
		}
	}
	scratch->markAsDirty();	
	scratch->draw();

	// draw objects
	scratch->clear();
	//LD31LevelData *lvl = testAdventure.levels.at(gamesingleton->currentLevelID);
	for(int i=0;i<gamesingleton->entities.size();i++)
	{
		if (gamesingleton->entities.at(i) != gamesingleton->penguin)
			gamesingleton->entities.at(i)->draw(scratch);
	}
	if (gamesingleton->penguin != NULL)
		gamesingleton->penguin->draw(scratch);
	/*for(int i=0;i<lvl->objects.size();i++)
	{
		switch(lvl->objects.at(i)->type)
		{
		case OBJ_PLAYER:
			batch_add_sprite(scratch,sprPlayer,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
			break;
		case OBJ_BOULDER:
			batch_add_sprite(scratch,sprBoulder,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
			break;
		case OBJ_DOOR:
			batch_add_sprite(scratch,sprDoor,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
			break;
		case OBJ_CAKE:
			batch_add_sprite(scratch,sprCake,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
			break;
		case OBJ_TELEPHONE:
			batch_add_sprite(scratch,sprTelephone,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
			break;
		}
	}*/
	scratch->markAsDirty();	
	scratch->draw();

	// DRAW THE WORD (not bird)
	scratch->clear();
	if (selectedLevelIndex >= 0)
		draw_fancy_text(scratch,0,23,testAdventure.levels.at(gamesingleton->currentLevelID)->noveltyText,ZX_LTGRAY);
	scratch->markAsDirty();
	scratch->draw();
	delete scratch;

	// draw the colors
	updateColors();
	ww::gfx::setBlendMode(ww::gfx::BM_MULTIPLY);
	glBindTexture(GL_TEXTURE_2D, FGCOL);
	colorBatch.draw();
	ww::gfx::setBlendMode(ww::gfx::BM_NORMAL);
	return true;
}



bool dodo()
{
	if (ww::input::keyboard::isKeyPressed(ww::input::key::F5))
	{
		EDITOR = !EDITOR;
		if (EDITOR)
			ww::gui::show_control(toolWindow);
		else
		{
			startGame();
		}
	}
	if (EDITOR)
		return doeditor();
	else
		return dogame();
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
			ret = dodo();
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
		ret = dodo();
	return ret;
}

bool doend()
{
	//ww::terminate();
	printf("doend\n");
	return true;
}

bool doeditor()
{
	if (ww::gui::tab_check(tabLevels))
	{
		ww::gui::controlgroup_setvisible(fileGroup,true);
		ww::gui::controlgroup_setvisible(levelInfoGroup,false);
		ww::gui::controlgroup_setvisible(objectsGroup,false);
		ww::gui::controlgroup_setvisible(tilesGroup,false);
		ww::gui::controlgroup_setvisible(colorsGroup,false);
		ww::gui::controlgroup_setvisible(selObjectGroup,false);
		selectedObject = NULL;
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}
	if (ww::gui::tab_check(tabLevelInfo))
	{
		ww::gui::controlgroup_setvisible(fileGroup,false);
		ww::gui::controlgroup_setvisible(levelInfoGroup,true);
		ww::gui::controlgroup_setvisible(objectsGroup,false);
		ww::gui::controlgroup_setvisible(tilesGroup,false);
		ww::gui::controlgroup_setvisible(colorsGroup,false);
		ww::gui::controlgroup_setvisible(selObjectGroup,false);
		selectedObject = NULL;
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}
	if (ww::gui::tab_check(tabObjects))
	{
		ww::gui::controlgroup_setvisible(fileGroup,false);
		ww::gui::controlgroup_setvisible(levelInfoGroup,false);
		ww::gui::controlgroup_setvisible(objectsGroup,true);
		ww::gui::controlgroup_setvisible(tilesGroup,false);
		ww::gui::controlgroup_setvisible(colorsGroup,false);
		if (selectedObject)
			ww::gui::controlgroup_setvisible(selObjectGroup,true);
		else
			ww::gui::controlgroup_setvisible(selObjectGroup,false);
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}
	if (ww::gui::tab_check(tabTiles))
	{
		ww::gui::controlgroup_setvisible(fileGroup,false);
		ww::gui::controlgroup_setvisible(levelInfoGroup,false);
		ww::gui::controlgroup_setvisible(objectsGroup,false);
		ww::gui::controlgroup_setvisible(tilesGroup,true);
		ww::gui::controlgroup_setvisible(colorsGroup,false);
		ww::gui::controlgroup_setvisible(selObjectGroup,false);
		selectedObject = NULL;
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}
	if (ww::gui::tab_check(tabColors))
	{
		ww::gui::controlgroup_setvisible(fileGroup,false);
		ww::gui::controlgroup_setvisible(levelInfoGroup,false);
		ww::gui::controlgroup_setvisible(objectsGroup,false);
		ww::gui::controlgroup_setvisible(tilesGroup,false);
		ww::gui::controlgroup_setvisible(colorsGroup,true);
		ww::gui::controlgroup_setvisible(selObjectGroup,false);
		//ww::gui::controlgroup_setvisible(infoGroup,false);
	}

	if (ww::gui::controlgroup_getvisible(fileGroup) || ww::gui::controlgroup_getvisible(levelInfoGroup))
	{
		static int selectedLevelIndexBefore = -1;
		selectedLevelIndex = ww::gui::listbox_getselectedindex(levelListBox);

		

		char noveltemp[34];
		static int noveltyLengthBefore = -1;
		ww::gui::textbox_gettext(levelNoveltyText,noveltemp,33);
		int noveltyLength = strlen(noveltemp);

		char lefttemp[34];
		static int leftLengthBefore = -1;
		ww::gui::textbox_gettext(levelLEFTtext,lefttemp,33);
		int leftLength = strlen(lefttemp);

		char righttemp[34];
		static int rightLengthBefore = -1;
		ww::gui::textbox_gettext(levelRIGHTtext,righttemp,33);
		int rightLength = strlen(righttemp);

		char uptemp[34];
		static int upLengthBefore = -1;
		ww::gui::textbox_gettext(levelUPtext,uptemp,33);
		int upLength = strlen(uptemp);

		char downtemp[34];
		static int downLengthBefore = -1;
		ww::gui::textbox_gettext(levelDOWNtext,downtemp,33);
		int downLength = strlen(downtemp);


		if (selectedLevelIndex != selectedLevelIndexBefore && selectedLevelIndex >= 0)
		{
			ZXCOLORPTR = testAdventure.levels.at(selectedLevelIndex)->COLORS;
			ww::gui::textbox_settext(levelNameText,testAdventure.levels.at(selectedLevelIndex)->name);
			ww::gui::textbox_settext(levelNoveltyText,testAdventure.levels.at(selectedLevelIndex)->noveltyText);

			ww::gui::textbox_settext(levelLEFTtext,testAdventure.levels.at(selectedLevelIndex)->leftExit);
			ww::gui::textbox_settext(levelRIGHTtext,testAdventure.levels.at(selectedLevelIndex)->rightExit);
			ww::gui::textbox_settext(levelUPtext,testAdventure.levels.at(selectedLevelIndex)->upExit);
			ww::gui::textbox_settext(levelDOWNtext,testAdventure.levels.at(selectedLevelIndex)->downExit);
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
					sprintf(newtext,"%s",noveltemp);
					testAdventure.levels.at(selectedLevelIndex)->noveltyText = newtext;
				}
			}


			if (leftLength != leftLengthBefore && selectedLevelIndex >= 0)
			{
				if (leftLength == 0)
				{
					delete testAdventure.levels.at(selectedLevelIndex)->leftExit;
					testAdventure.levels.at(selectedLevelIndex)->leftExit = NULL;
				}
				else
				{
					delete testAdventure.levels.at(selectedLevelIndex)->leftExit;
					char *newtext = new char[leftLength+1];
					sprintf(newtext,"%s",lefttemp);
					testAdventure.levels.at(selectedLevelIndex)->leftExit = newtext;
				}
			}


			if (rightLength != rightLengthBefore && selectedLevelIndex >= 0)
			{
				if (rightLength == 0)
				{
					delete testAdventure.levels.at(selectedLevelIndex)->rightExit;
					testAdventure.levels.at(selectedLevelIndex)->rightExit = NULL;
				}
				else
				{
					delete testAdventure.levels.at(selectedLevelIndex)->rightExit;
					char *newtext = new char[rightLength+1];
					sprintf(newtext,"%s",righttemp);
					testAdventure.levels.at(selectedLevelIndex)->rightExit = newtext;
				}
			}


			if (upLength != upLengthBefore && selectedLevelIndex >= 0)
			{
				if (upLength == 0)
				{
					delete testAdventure.levels.at(selectedLevelIndex)->upExit;
					testAdventure.levels.at(selectedLevelIndex)->upExit = NULL;
				}
				else
				{
					delete testAdventure.levels.at(selectedLevelIndex)->upExit;
					char *newtext = new char[upLength+1];
					sprintf(newtext,"%s",uptemp);
					testAdventure.levels.at(selectedLevelIndex)->upExit = newtext;
				}
			}

			if (downLength != downLengthBefore && selectedLevelIndex >= 0)
			{
				if (downLength == 0)
				{
					delete testAdventure.levels.at(selectedLevelIndex)->downExit;
					testAdventure.levels.at(selectedLevelIndex)->downExit = NULL;
				}
				else
				{
					delete testAdventure.levels.at(selectedLevelIndex)->downExit;
					char *newtext = new char[downLength+1];
					sprintf(newtext,"%s",downtemp);
					testAdventure.levels.at(selectedLevelIndex)->downExit = newtext;
				}
			}
		}
		selectedLevelIndexBefore = selectedLevelIndex;
		noveltyLengthBefore = noveltyLength;
		leftLengthBefore = leftLength;
		rightLengthBefore = rightLength;
		upLengthBefore = upLength;
		downLengthBefore = downLength;
	}
	if (ww::gui::controlgroup_getvisible(fileGroup))
	{
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
			while(testAdventure.levels.size())
			{
				delete testAdventure.levels.at(0);
				testAdventure.levels.erase(testAdventure.levels.begin());
			}
			testAdventure.loadFromFile(path);
			while(ww::gui::listbox_getstringcount(levelListBox))
				ww::gui::listbox_deletestring(levelListBox,0);
			for(int i=0;i<testAdventure.levels.size();i++)
				ww::gui::listbox_addstring(levelListBox,testAdventure.levels.at(i)->name);
			ww::gui::listbox_setselectedindex(levelListBox,-1);
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
		
	}
	if (ww::gui::controlgroup_getvisible(levelInfoGroup))
	{
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
					if (selTile == LDTV_TRISTONE)
						testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_TRISTONE);
					if (selTile == LDTV_TITLE)
						testAdventure.levels.at(selectedLevelIndex)->setTileID(mouse_x/8,mouse_y/8,TILE_TITLE);
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
	if (ww::gui::controlgroup_getvisible(objectsGroup))
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
				if (ww::input::mouse::isButtonPressed(ww::input::mouse::LEFT))
				{
					int selObj = ww::gui::listview_get_selected(objectsView);
					if (selObj >= 0)
					{
						LD31Object *obj = new LD31Object();
						obj->x = 8*(mouse_x/8);
						obj->y = 8*(mouse_y/8);
						obj->str1 = NULL;
						obj->str2 = NULL;
						obj->str3 = NULL;
						obj->str4 = NULL;
						if (selObj == 0)
						{
							obj->type = OBJ_PLAYER; 
							obj->width = obj->height = 16;
						}
						if (selObj == 1)
						{
							obj->type = OBJ_BOULDER; 
							obj->width = obj->height = 16;
						}
						if (selObj == 2)
						{
							obj->type = OBJ_DOOR; 
							obj->width = obj->height = 16;
						}
						if (selObj == 3)
						{
							obj->type = OBJ_CAKE; 
							obj->width = obj->height = 16;
						}
						if (selObj == 4)
						{
							obj->type = OBJ_TELEPHONE; 
							obj->width = obj->height = 16;
						}
						testAdventure.levels.at(selectedLevelIndex)->objects.push_back(obj);
					}
				}
				if (ww::input::mouse::isButtonPressed(ww::input::mouse::RIGHT))
				{
					int pos = -1;
					for(int i=0;i<testAdventure.levels.at(selectedLevelIndex)->objects.size();i++)
					{
						LD31Object *obj = testAdventure.levels.at(selectedLevelIndex)->objects.at(i);
						if (mouse_x >= obj->x && mouse_y >= obj->y && mouse_x < obj->x+obj->width && mouse_y < obj->y+obj->height)
						{
							pos = i;
						}
					}
					if (pos == -1)
					{
						selectedObject = NULL;
						ww::gui::controlgroup_setvisible(selObjectGroup,false);
					}
					else
					{
						selectedObject = testAdventure.levels.at(selectedLevelIndex)->objects.at(pos);
						ww::gui::controlgroup_setvisible(selObjectGroup,true);
						ww::gui::textbox_settext(selObjectString1,selectedObject->str1);
						ww::gui::textbox_settext(selObjectString2,selectedObject->str2);
						ww::gui::textbox_settext(selObjectString3,selectedObject->str3);
						ww::gui::textbox_settext(selObjectString4,selectedObject->str4);					
					}
				}
			}
		}
		else if (ww::gui::controlgroup_getvisible(selObjectGroup))
		{
			if (ww::gui::button_check(selObjectDeleteButton))
			{
				for(int i=0;i<testAdventure.levels.at(selectedLevelIndex)->objects.size();i++)
				{
					if (testAdventure.levels.at(selectedLevelIndex)->objects.at(i) == selectedObject)
					{
						delete selectedObject;
						testAdventure.levels.at(selectedLevelIndex)->objects.erase(
							testAdventure.levels.at(selectedLevelIndex)->objects.begin()+i
							);
						selectedObject = NULL;
						break;
					}
				}
			}
			if (selectedObject)
			{
				delete selectedObject->str1;
				delete selectedObject->str2;
				delete selectedObject->str3;
				delete selectedObject->str4;
				selectedObject->str1 = new char[128];
				selectedObject->str2 = new char[128];
				selectedObject->str3 = new char[128];
				selectedObject->str4 = new char[128];
				ww::gui::textbox_gettext(selObjectString1,selectedObject->str1,120);
				ww::gui::textbox_gettext(selObjectString2,selectedObject->str2,120);
				ww::gui::textbox_gettext(selObjectString3,selectedObject->str3,120);
				ww::gui::textbox_gettext(selObjectString4,selectedObject->str4,120);

				if (ww::gui::button_check(selObjectLeftButton))
					selectedObject->x -= 8;
				if (ww::gui::button_check(selObjectUpButton))
					selectedObject->y -= 8;
				if (ww::gui::button_check(selObjectRightButton))
					selectedObject->x += 8;
				if (ww::gui::button_check(selObjectDownButton))
					selectedObject->y += 8;
			}

		}
		// end sel object group

	} // end objects group
	if (!ww::gfx::supportsOpenGL2())
	{
		glViewport(0,0,window_width,window_height);
		glScissor(0,0,window_width,window_height);
	}
	defaultShader->makeActive();
	ww::gfx::setMatrix(defaultShader,defaultVertexMatrixID,glm::value_ptr(glm::ortho(0.f,256.f,192.f,0.f,-1.f,1.f)));
	if (ww::input::keyboard::isKeyDown(ww::input::key::LShift) || ww::input::keyboard::isKeyDown(ww::input::key::RShift))
		glClearColor(1.f,1.f,1.f,1.f);
	else
		glClearColor(0.f,0.f,0.f,1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D,supertex.getTexture());

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
			if (t->type == TILE_TRISTONE)
				batch_add_sprite(scratch,sprTileTriStone[x%2 + 2*(y%2)],8*x,8*y);
			if (t->type == TILE_TITLE)
				batch_add_sprite(scratch,sprTileTitle,8*x,8*y);
			else if (tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x,y)))
			{
				unsigned char S = !(tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x+1,y)) || (x == 31))
								+ 2*!(tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x,y-1)) || (y == 0))
								+ 4*!(tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x-1,y)) || (x == 0))
								+ 8*!(tileIsSolid(testAdventure.levels.at(selectedLevelIndex)->getTileID(x,y+1)) || (y == 22));
				if (S > 0)
					batch_add_sprite(scratch,sprTileBorder[S],8*x,8*y);
			}
		}
		
	}
	scratch->markAsDirty();	
	scratch->draw();

	// draw objects
	scratch->clear();
	if (selectedLevelIndex >= 0)
	{
		LD31LevelData *lvl = testAdventure.levels.at(selectedLevelIndex);
		for(int i=0;i<lvl->objects.size();i++)
		{
			switch(lvl->objects.at(i)->type)
			{
			case OBJ_PLAYER:
				batch_add_sprite(scratch,sprPlayer,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
				break;
			case OBJ_BOULDER:
				batch_add_sprite(scratch,sprBoulder,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
				break;
			case OBJ_DOOR:
				batch_add_sprite(scratch,sprDoor,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
				break;
			case OBJ_CAKE:
				batch_add_sprite(scratch,sprCake,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
				break;
			case OBJ_TELEPHONE:
				batch_add_sprite(scratch,sprTelephone,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
				break;
			}
			if (lvl->objects.at(i) == selectedObject)
			{
				if (rand()%2)
					batch_add_sprite(scratch,sprSelectedObject,lvl->objects.at(i)->x,lvl->objects.at(i)->y);
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