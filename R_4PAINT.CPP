#include "gl.h"
#include "imguigl.hpp"
#include "imgui.h"
#include "filedlg.hpp"
#include "image.hpp"
#include "p4pal.hpp"

int XRES = 800;
int YRES = 600;

unsigned int *converted;
unsigned int *bitmap;
unsigned int *backdrop;
unsigned int *undo;

String fileNameBitmap;
String fileNameBackdrop;
String fileNamePRG;

#define NONE 0
#define LOAD_BITMAP 1
#define SAVE_BITMAP 2
#define LOAD_BACKDROP 3
#define TRY_QUIT 4
#define SAVE_PRG 5
int serialize = NONE;
bool quit = false;

bool wideScreen = true;
bool backdropOn = false;
bool backdropInFront = false;
float backdropSaturation = 1.0;
float backdropTransparency = 0.5;
float backdropContrast = 1;
float backdropBrightness = 0;
bool backdropGreyScale = false;
bool grid = false;
bool onePixelGrid = true; 
float gridIntensity = 0.25;
unsigned int paintColor = 0xffffffff;
int mousePointerX = 0;
int mousePointerY = 0;
bool mouseCaptured = false;
bool multiColor = false;

#define OVERALLSCALE 1.3

bool hasUndo = true;

int canvasX0;
int canvasY0;
int canvasX1;
int canvasY1;

int paletteX0;
int paletteY0;
int paletteX1;
int paletteY1;
int paletteColor = 0;
unsigned int paletteHovering = 0;

int selectX0;
int selectY0;
int selectX1;
int selectY1;
bool displayColorSelector = false;

int SLIDERSIZE = 64;

float convertAlpha = 1;
bool showTestConvert = false;

unsigned int stub[22]={
  0x100b1001,
  0x349e0000,
  0x00393031,
  0x8d780000,
  0xffa9ff3f,
  0xa9ff0c8d,
  0xff0d8dff,
  0x8d17fdad,
  0xfeadff15,
  0xff168d17,
  0x8d17ffad,
  0x12adff19,
  0x09c329ff,
  0xff128d08,
  0x148d18a9,
  0x8d3ba9ff,
  0x07adff06,
  0x094029ff,
  0x17fcae88,
  0x100902f0,
  0x4cff078d,
  0xff001052,
};

int colorRam(int c1,int c2) {
  return ((c1 & 0x07)<<0)+((c2 & 0x07)<<4);
}

int luminanceRam(int c1,int c2) {
  return ((c1>>4)<<4)+((c2>>4)<<0);
}

void blitCanvas() {
  int k2 = 0;
  int tg = gridIntensity*256;
  int ta = convertAlpha*256;
  int bc = backdropContrast*256;
  int bb = backdropBrightness*backdropContrast*256;
  int bg = backdropTransparency*256;
  int bs = backdropSaturation*256;
  int gridX = onePixelGrid ? (multiColor?1:0):3;
  int gridY = onePixelGrid ? 0:3;

  for (int y = canvasY0; y < canvasY1; y++) {
    int ix = 0;
    int ixa = (320<<16)/(canvasX1-canvasX0);
    int iy = ((y - canvasY0)*200/(canvasY1-canvasY0));
    unsigned int *s = bitmap + iy*320;
    unsigned int *c = converted + iy*320;
    unsigned int *b = backdrop + iy*320;
    unsigned int *write = &glFrameBuffer[y*glFrameBufferWidth+canvasX0];
    bool grid2 = false;
    if (grid&&(iy>>gridY)!=((((y - canvasY0-1)*200/(canvasY1-canvasY0)))>>gridY)) {
      grid2 = true;
    }
    int shift = multiColor ? 1 : 0;
    int and = multiColor ? 0xfffffffe : 0xffffffff;
    for (int x = canvasX0; x < canvasX1; x++) {
      int ix2 = (ix>>16) & and;
      unsigned int rgba = s[ix2];
      if (backdropOn) {
        unsigned int rgba2 = b[ix2];
        int r1 = rgba & 255;
        int g1 = (rgba>>8) & 255;
        int b1 = (rgba>>16) & 255;
        int r2 = rgba2 & 255;
        int g2 = (rgba2>>8) & 255;
        int b2 = (rgba2>>16) & 255;
        int grey = r2*0.25 + g2 * 0.6 + b2 * 0.15;
        r2 = (((r2 - grey)*bs)>>8)+grey;
        g2 = (((g2 - grey)*bs)>>8)+grey;
        b2 = (((b2 - grey)*bs)>>8)+grey;
        r2 = (((r2 - 128)*bc)>>8)+128+bb;
        g2 = (((g2 - 128)*bc)>>8)+128+bb;
        b2 = (((b2 - 128)*bc)>>8)+128+bb;

        if (!backdropInFront) {
          int r = (((r2 - r1)*bg)>>8)+r1;
          int g = (((g2 - g1)*bg)>>8)+g1;
          int b = (((b2 - b1)*bg)>>8)+b1;
          if (r < 0) r = 0;
          if (g < 0) g = 0;
          if (b < 0) b = 0;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          rgba = r|(g<<8)|(b<<16);
        } else {
          int r = ((r2*bg)>>8)+r1;
          int g = ((g2*bg)>>8)+g1;
          int b = ((b2*bg)>>8)+b1;
          if (r < 0) r = 0;
          if (g < 0) g = 0;
          if (b < 0) b = 0;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          rgba = r|(g<<8)|(b<<16);
        }
      }
      if (showTestConvert) {
        int r1 = rgba & 255;
        int g1 = (rgba>>8) & 255;
        int b1 = (rgba>>16) & 255;
        unsigned int rgba2 = c[ix2];
        int r2 = rgba2 & 255;
        int g2 = (rgba2>>8) & 255;
        int b2 = (rgba2>>16) & 255;
        int r = (((r2 - r1)*ta)>>8)+r1;
        int g = (((g2 - g1)*ta)>>8)+g1;
        int b = (((b2 - b1)*ta)>>8)+b1;
        rgba = r|(g<<8)|(b<<16);
      }
      if (iy == mousePointerY && (ix2>>shift) == (mousePointerX>>shift)) rgba = rgba ^ 0xffffffff;
      if ((grid&&(ix2>>gridX)!=((((ix-ixa)>>16) & and)>>gridX))||grid2) {
        int r1 = rgba & 255;
        int g1 = (rgba>>8) & 255;
        int b1 = (rgba>>16) & 255;
        unsigned int grey = r1 + g1 + b1;
        unsigned int gridColor = 0xffffffff;
        if (grey>128*3) gridColor = 0xff000000;
        int r2 = gridColor & 255;
        int g2 = (gridColor>>8) & 255;
        int b2 = (gridColor>>16) & 255;
        int r = (((r2 - r1)*tg)>>8)+r1;
        int g = (((g2 - g1)*tg)>>8)+g1;
        int b = (((b2 - b1)*tg)>>8)+b1;
        rgba = r|(g<<8)|(b<<16);
      }
      *write++ = rgba|0xff000000;
      ix += ixa;
    }
    *write++= 0xffffffff;
  }
  
  for (int x = canvasX0; x < canvasX1; x++) {
    glFrameBuffer[canvasY1*glFrameBufferWidth+x] = 0xffffffff;
  }
}

void blitLupe() {
  int L = 44;
  unsigned int and = multiColor ? 0xfffffffe:0xffffffff;
  for (int y = -L; y <= L; y++) {
    for (int x = -L; x <= L; x++) {
      int kx = (x+L) / 4-L/4;
      int ky = (y+L) / 4-L/4;
      unsigned int rgba = 0xffffffff;
      int rx = kx+mousePointerX;
      int ry = ky+mousePointerY;
      if (rx>=0&&rx<320&&ry>=0&&ry<200) rgba = bitmap[rx+ry*320];
      if ((kx&and)==0&&ky==0) rgba ^= 0xffffffff;
      glFrameBuffer[x+glFrameBufferWidth-L-1+(y+L+1)*glFrameBufferWidth] = rgba|0xff000000;
    }
  }
}

void blitPalette() {
  paletteHovering = 0;
  for (int y = paletteY0; y < paletteY1; y++) {
    int ry = (y - paletteY0)*16/(paletteY1-paletteY0);
    for (int x = paletteX0; x < paletteX1; x++) {
      int rx = (x - paletteX0)*8/(paletteX1-paletteX0);
      int index = rx*16+ry;
      if ((int)mouseX == x && (int)mouseY == y) {
        paletteHovering = plus4PaletteStandard[index];
      }
      if (mouseButtons & 3) {
        if (!mouseCaptured && (int)mouseX == x && (int)mouseY == y) {
          paletteColor = index;
          paintColor = plus4PaletteStandard[index];
        }
      }
      int rgba = plus4PaletteStandard[index];
      if (paletteColor == index) {
        if ((x+y) & 2)
          rgba = ((x+y)&4)?0xffffffff:0x00000000;
      }
      glFrameBuffer[x+y*glFrameBufferWidth] = rgba|0xff000000;
    }
  }
}

int peak(int a, int s) {
  a -= s;
  a -= 60+180;
  a = (a+360*2)%360;
  if (a < 60) return a * 255 / 60;
  if (a < 60+120) return 255;
  if (a < 240) return 255-(a-(60+120))*255/60;
  return 0;
}

unsigned int hsvColor = 0;

void blitColorSelect() {
  bool already = false;
  {for (int y = selectY0; y < selectY1+10; y++) {
    int saturation = 0<<8;
    int saturationAdd = (256<<8)/(selectX1-selectX0);
    int r = hsvColor&255;
    int g = (hsvColor>>8)&255;
    int b = (hsvColor>>16)&255;
    int grey = 128;
    int light = (y - selectY0)*256/(selectY1-selectY0)*2;
    for (int x = selectX0; x < selectX1; x++) {
      int r2 = (((((r - grey)*saturation)>>16)+grey)*light)>>8;
      int g2 = (((((g - grey)*saturation)>>16)+grey)*light)>>8;
      int b2 = (((((b - grey)*saturation)>>16)+grey)*light)>>8;
      if (r2>255) r2 = 255;
      if (g2>255) g2 = 255;
      if (b2>255) b2 = 255;
      unsigned int rgba = r2|(g2<<8)|(b2<<16)|0xff000000;
      if ((int)mouseX == x && (int)mouseY == y) {
        if(mouseButtons&3) {
          paintColor = rgba;
          paletteColor = -1;
        }
        paletteHovering = rgba;
      }
      if (rgba == paintColor && (!already)) {
        already=true;
        rgba = 0xff000000;
        glFrameBuffer[x+(y-1)*glFrameBufferWidth] = rgba;
        rgba = 0xffffffff;
      }
      glFrameBuffer[x+y*glFrameBufferWidth] = rgba;
      saturation += saturationAdd;
    }
  }}

  {for (int y = selectY1+15; y < selectY1+25; y++) {
    int xp = 0<<8;
    int xa = ((256*360)<<8)/(selectX1-selectX0);
    for (int x = selectX0; x < selectX1; x++) {
      int r2 = peak(xp>>16,0);
      int g2 = peak(xp>>16,120);
      int b2 = peak(xp>>16,240);
      unsigned int rgba = r2|(g2<<8)|(b2<<16)|0xff000000;
      if ((int)mouseX == x && (int)mouseY == y) {
        if (mouseButtons & 3)
          hsvColor = rgba;
      }
      if (rgba == hsvColor) rgba = 0xffffffff;
      glFrameBuffer[x+y*glFrameBufferWidth] = rgba;
      xp+=xa;
    }
  }}

}

String fileDialog(bool loadInsteadOfSave, const char *filter) {
  FileDialog *dlg;
  dlg = new FileDialog();
  dlg->open(loadInsteadOfSave, filter);

  while(true) {
    ImGuiNewFrame();
    bool ret = dlg->display(); 
    glClearColor(10.f/255.f,40.f/255.f,30.f/255.f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    glRefresh();
    if (ret) break;
  }
  String ret = dlg->getSelectedFileName();
  delete dlg;
  return ret;
}

int modalYesNo(const String &m) {
  int button = -1;

  while(button == -1) {
    ImGuiNewFrame();
    ImGui::Begin("Attention");
    ImGui::Text("%s", m.c_str());
    if (ImGui::Button("No")) button = 0;
    ImGui::SameLine();
    if (ImGui::Button("Yes")) button = 1;
    ImGui::End();
    glClearColor(10.f/255.f,40.f/255.f,30.f/255.f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    glRefresh();
  }
  return button;
}

void loadBitmap(const String &fileName, unsigned int *dest, int width, int height) {
  if (fileName.empty()) return;
  RGBAImage image = RGBAImage::fromFile(fileName.c_str());
  RGBAImage image2 = image.getResized(width,height);
  image.free();
  memcpy(dest,image2.data,width*height*sizeof(unsigned int));
  image2.free();
}

void saveBitmap(const String &fileName2, unsigned int *source, int width, int height, bool doublePixels) {
  String fileName = fileName2;
  if (fileName.findLast(".")<0) fileName += ".png";
  if (fileName.empty()) return;
  RGBAImage image;
  image.width = width;
  image.height = height;
  image.data = source;
  if (doublePixels) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        source[x+y*width] = source[x/2*2+y*width];
      }
    }
  }
  image.savePNG(fileName.c_str());
}

String real(const String &r) {
  int a = r.findLast("\\");
  return r.substr(a+1);
}

unsigned int plus4Color(unsigned int rgba) {
  int r1 = rgba & 255;
  int g1 = (rgba>>8) & 255;
  int b1 = (rgba>>16) & 255;
  int ld = 0;
  int found = 0;
  for (int i = 0; i < 128; i++) {
    unsigned int rgba2 = plus4PaletteStandard[i];
    int r2 = rgba2 & 255;
    int g2 = (rgba2>>8) & 255;
    int b2 = (rgba2>>16) & 255;
    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;
    int d = dr*dr+dg*dg+db*db;
    if(d < ld || i == 0) {
      found = i;
      ld = d;
    }
  }
  return found;
}

void colorize() {
  for (int i = 0; i < 320*200; i++) bitmap[i] = plus4PaletteStandard[plus4Color(bitmap[i])];
}


void setPixel(int x, int y) {
  if (multiColor) x=x/2*2;
  if (x>=0&&y>=0&&x<320&&y<200) {
    bitmap[x+y*320] = paintColor;
    if (multiColor)
      bitmap[x+1+y*320] = paintColor;
  }
}

void floodFill(int x, int y) {
  if (x>=0&&y>=0&&x<320&&y<200) {
    unsigned int colorHere = bitmap[x+y*320];
    const unsigned int FLOORDMARK = 0x12345678;
    bitmap[x+y*320] = FLOORDMARK;
    bool somethingFound = true;
    while(somethingFound) {
      somethingFound = false;
      for (int y2 = 0; y2 < 200; y2++) {
        for (int x2 = 0; x2 < 320; x2++) {
          if (bitmap[x2+y2*320]==FLOORDMARK) {
            if (x2<319) if (bitmap[x2+1+y2*320]==colorHere) {bitmap[x2+1+y2*320]=FLOORDMARK;somethingFound = true;continue;}
            if (x2>0) if (bitmap[x2-1+y2*320]==colorHere) {bitmap[x2-1+y2*320]=FLOORDMARK;somethingFound = true;continue;}
            if (y2<199) if (bitmap[x2+y2*320+320]==colorHere) {bitmap[x2+y2*320+320]=FLOORDMARK;somethingFound = true;continue;}
            if (y2>0) if (bitmap[x2+y2*320-320]==colorHere) {bitmap[x2+y2*320-320]=FLOORDMARK;somethingFound = true;continue;}
          }
        }
      }
    }
    for (int i = 0; i < 320*200; i++) if (bitmap[i]==FLOORDMARK) bitmap[i]=paintColor;
  }
}

static void drawMouseArrow(int mouseX, int mouseY) {
  const int width = 8;
  const int height = 8;
  const char *bitmap =
  "60000000"
  "98900000"
  "98889000"
  "09888990"
  "09888889"
  "00989999"
  "00989000"
  "00099000"
  ;
  mouseX -= glFrameBufferWidth*0.5f-glFrameBufferWidth*0.5f*glGetZoomX();
  bool first = true;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      char c = bitmap[x+y*width];
      if (c != '0') {
        unsigned int color = 0xffffffff;
        if (c == '9')
          color = 0xff000000;
        if (c == '6')
          color = 0xffffffff;
        glPixel(first,0,0,1,x+mouseX,y+mouseY,color);
        first = false;
      }
    }
  }
}

unsigned int color0 = 0;
unsigned int color1[40*25];
unsigned int color2[40*25];
unsigned int color3 = 0;

void convert() {
  if (!multiColor) {
    for (int y = 0; y < 25; y++) {
      for (int x = 0; x < 40; x++) {
        int c1 = -1;
        int c2 = -1;
        {for (int y2 = 0; y2 < 8; y2++) {
          for (int x2 = 0; x2 < 8; x2++) {
            unsigned int c = bitmap[x2+x*8+(y2+y*8)*320];
            c = plus4Color(c);
            if (c1 == -1) c1 = c; else
            if (c != c1 && c2 == -1) c2 = c;
          }
        }}
        if (c2 == -1) c2 = 0;
        if (c1 > c2) {
          int t = c1; c1 = c2; c2 = t;
        }
        color1[x+y*40] = c1;
        color2[x+y*40] = c2;
        {for (int y2 = 0; y2 < 8; y2++) {
          for (int x2 = 0; x2 < 8; x2++) {
            unsigned int c = bitmap[x2+x*8+(y2+y*8)*320];
            c = plus4Color(c);
            if (c == c1 || c == c2) {
              converted[x2+x*8+(y2+y*8)*320] = plus4PaletteStandard[c];
            } else {
              converted[x2+x*8+(y2+y*8)*320] = 0xff0000ff;
            }
          }
        }}
      }
    }
  } else {
    color0 = plus4Color(bitmap[320*0]);
    color3 = plus4Color(bitmap[320*1]);
    for (int y = 0; y < 25; y++) {
      for (int x = 0; x < 40; x++) {
        int c1 = -1;
        int c2 = -1;
        {for (int y2 = 0; y2 < 8; y2++) {
          for (int x2 = 0; x2 < 8; x2+=2) {
            unsigned int c = bitmap[x2+x*8+(y2+y*8)*320];
            c = plus4Color(c);
            if (c == color0 || c == color3) continue;
            if (c1 == -1) c1 = c; else
            if (c != c1 && c2 == -1) c2 = c;
          }
        }}
        if (c1 == -1) c1 = 0;
        if (c2 == -1) c2 = 0;
        if (c1 > c2) {
          int t = c1; c1 = c2; c2 = t;
        }
        color1[x+y*40] = c1;
        color2[x+y*40] = c2;
        {for (int y2 = 0; y2 < 8; y2++) {
          for (int x2 = 0; x2 < 8; x2+=2) {
            unsigned int c = bitmap[x2+x*8+(y2+y*8)*320];
            c = plus4Color(c);
            if (c == c1 || c == c2 || c == color0 || c == color3) {
              converted[x2+x*8+(y2+y*8)*320] = plus4PaletteStandard[c];
              converted[x2+x*8+1+(y2+y*8)*320] = plus4PaletteStandard[c];
            } else {
              converted[x2+x*8+(y2+y*8)*320] = 0xff0000ff;
              converted[x2+x*8+1+(y2+y*8)*320] = 0xff0000ff;
            }
          }
        }}
      }
    }
  }
}

void markUndo() {
  memcpy(undo,bitmap,320*200*sizeof(unsigned int));
  hasUndo = true;
}

void doUndo() {
  for (int i = 0; i < 320*200; i++) {
    int t = bitmap[i];
    bitmap[i] = undo[i];
    undo[i] = t;
  }
}

void setCanvasScale() {
  float scale = 1.7*OVERALLSCALE;
  canvasX0 = 0;
  canvasY0 = 0;
  canvasX1 = 320*scale;
  canvasY1 = 200*scale*(wideScreen?(16.0/9.0*YRES/XRES):1);
}

void generateStubHPP() {
  FILE *in = fopen("stub.prg","rb");
  fseek(in,0,SEEK_END);
  int fsize = ftell(in);
  fseek(in,0,SEEK_SET);
  unsigned int *fk = new unsigned int[(fsize+3)/4];
  fread(fk,1,fsize,in);
  fclose(in);
  FILE *out = fopen("stub.hpp","wb");
  fprintf(out,"unsigned int stub[%d]={\n",(fsize+3)/4);
  for (int i = 0; i < (fsize+3)/4; i++) {
    fprintf(out,"\t0x%08x,\n",fk[i]);
  }
  fprintf(out,"};\n",(fsize+3)/4);
  fclose(out);
  delete fk;
}

void savePRG(const String &fileName2) {
  String fileName = fileName2;
  if (fileName.empty()) return;
  if (fileName.findLast(".")<0) fileName += ".prg";
  convert();
  FILE *out = fopen(fileName.c_str(),"wb");
  int memPos = 0x1001-2;
  {for (int i = 0; i < sizeof(stub); i++) {
    fwrite(&(((char*)&stub)[i]),1,1,out);
    memPos++;
  }}
  unsigned char e = 0;
  for (;memPos<0x17fc;memPos++) fwrite(&e,1,1,out);
  int c0 = plus4Color(bitmap[320*0]);
  int c3 = plus4Color(bitmap[320*1]);
  e = multiColor ? 1 : 0; fwrite(&e,1,1,out);
  e = c0; fwrite(&e,1,1,out);
  e = c3; fwrite(&e,1,1,out);
  e = 0x00; fwrite(&e,1,1,out);
  {for (int i = 0; i < 0x400; i++) {
    e = i<40*25?luminanceRam(color1[i],color2[i]):0;
    fwrite(&e,1,1,out);
  }}
  {for (int i = 0; i < 0x400; i++) {
    e = i<40*25?colorRam(color1[i],color2[i]):0;
    fwrite(&e,1,1,out);
  }}
  {for (int i = 0; i < 320*200; i+=8) {
    int cy7 = (i/8) & 7;
    int cy8 = (i/8) / 8 / 40;
    int cx8 = ((i/8) / 8) % 40;
    int cy = cy7 + cy8 * 8;
    int cx = cx8 * 8;
    int c1 = color1[cx8+cy8*40];
    int c2 = color2[cx8+cy8*40];
    unsigned char b=0;
    if (!multiColor) {
      for (int x = 0; x < 8; x++) {
        int p = (cx+x)+cy*320;
        int c = plus4Color(bitmap[p]);
        if (c == c1) b|=0<<(7-x);
        if (c == c2) b|=1<<(7-x);
      }
    } else {
      for (int x = 0; x < 8; x+=2) {
        int p = (cx+x)+cy*320;
        int c = plus4Color(bitmap[p]);
        if (c == c0) b|=0<<(6-x);
        if (c == c1) b|=2<<(6-x);
        if (c == c2) b|=1<<(6-x);
        if (c == c3) b|=3<<(6-x);
      }
    }
    fwrite(&b,1,1,out);
  }}
  fclose(out);
}

int main(int argc, const char *argv) {

  glVesa(XRES,YRES,32);
  InitImGui(XRES, YRES);

  generateStubHPP();

  bitmap = new unsigned int[320 * 200];
  converted = new unsigned int[320 * 200];
  backdrop = new unsigned int[320 * 200];
  undo = new unsigned int[320 * 200];

  memset(bitmap,0,320*200*4);
  memset(converted,0,320*200*4);
  memset(backdrop,0,320*200*4);
  memset(undo,0,320*200*4);

  bitmap[320*1+0] = plus4PaletteStandard[0x71];
  bitmap[320*1+1] = plus4PaletteStandard[0x71];

  setCanvasScale();

  paletteX0 = glFrameBufferWidth-88;
  paletteY0 = 88+20;
  paletteX1 = glFrameBufferWidth;
  paletteY1 = 300;
  paletteColor = 0;
  paletteHovering = 0;

  selectX0 = glFrameBufferWidth-88;
  selectY0 = 88+20+200;
  selectX1 = glFrameBufferWidth;
  selectY1 = 250+200;


  bool lastWasPressed = false;
  int lastMouseX = 0;
  int lastMouseY = 0;
  bool menuEnabled = true;

  while(!quit) {
    int currentKey = glNextKey();
    if (currentKey == GL_VK_ESCAPE) serialize = TRY_QUIT;
    if (currentKey == 'q') menuEnabled = !menuEnabled;
    mousePointerX = ((int)mouseX-canvasX0)*320/(canvasX1-canvasX0)-1;
    mousePointerY = ((int)mouseY-canvasY0)*200/(canvasY1-canvasY0)-1;
    if (multiColor) mousePointerX = mousePointerX/2*2;

    if (!mouseCaptured) {
      if (mouseButtons & 3) showTestConvert = false;
      if (mouseButtons & 2) {
        if (mousePointerX>=0&&mousePointerY>=0&&mousePointerX<320&&mousePointerY<200) {
          paintColor = bitmap[mousePointerX+mousePointerY*320];
          int c = plus4Color(paintColor);
          if (plus4PaletteStandard[c]==paintColor)
            paletteColor = c;
        }
      }
      if (mouseButtons & 1) {
        if (!lastWasPressed) markUndo();
        if (lastWasPressed) {
          double px = lastMouseX;
          double py = lastMouseY;
          double ax = mousePointerX-px;
          double ay = mousePointerY-py;
          int maxDelta = abs(ax) + 1;
          if (abs(ay) + 1 > maxDelta) maxDelta = abs(ay) + 1;
          ax /= maxDelta;
          ay /= maxDelta;
          for (int i = 0; i < maxDelta; i++)
            setPixel(i*ax+px,i*ay+py);
        }
        lastWasPressed = true;
        lastMouseX = mousePointerX;
        lastMouseY = mousePointerY;
      } else {
        lastWasPressed = false;
      }
      if (currentKey == 'f') {
        markUndo();
        floodFill(mousePointerX,mousePointerY);
      }
      if (currentKey == 'u') {
        doUndo();
        hasUndo = !hasUndo;
      }
    } else {
      lastWasPressed = false;
    }

    if (serialize != NONE) {
      switch(serialize) {
        case LOAD_BITMAP: {fileNameBitmap = fileDialog(true,"*.png\0*.*\0\0"); loadBitmap(fileNameBitmap,bitmap,320,200);fileNameBitmap=real(real(fileNameBitmap));} break;
        case SAVE_BITMAP: {fileNameBitmap = fileDialog(false,"*.png\0*.*\0\0"); saveBitmap(fileNameBitmap,bitmap,320,200,multiColor);fileNameBitmap=real(real(fileNameBitmap));} break;
        case LOAD_BACKDROP: {fileNameBackdrop = fileDialog(true,"*.png\0*.*\0\0"); loadBitmap(fileNameBackdrop,backdrop,320,200);fileNameBackdrop=real(real(fileNameBackdrop));} break;
        case TRY_QUIT: {int i = modalYesNo("Really Exit?"); if (i == 1) quit=true;} break;
        case SAVE_PRG: {fileNamePRG = fileDialog(false,"*.prg\0*.*\0\0"); savePRG(fileNamePRG);fileNamePRG=real(real(fileNamePRG));} break;
      }
      showTestConvert = false;
      serialize = NONE;
    }

    if (menuEnabled) {
      ImGuiNewFrame();
  
      ImGui::Begin("4PAINT v1a");
      ImGui::Text("Image (320x200)");
      ImGui::Text("Pos:%d/%d Col:#%02x ABGR:%08x",mousePointerX,mousePointerY,paletteColor<0?0:paletteColor,paletteHovering != 0 ? paletteHovering : paintColor);
      if (ImGui::Button("Load")) {serialize = LOAD_BITMAP;}
      ImGui::SameLine();
      if (ImGui::Button("Save")) {serialize = SAVE_BITMAP;}
      ImGui::SameLine();
      ImGui::Text("%s",fileNameBitmap.c_str());
      ImGui::Checkbox("Double Pixels",&multiColor);
      ImGui::SameLine();
      if (ImGui::Checkbox("16:9",&wideScreen)) {
        setCanvasScale();
      }
      ImGui::Checkbox("Grid",&grid);
      ImGui::SameLine();
      ImGui::PushItemWidth(SLIDERSIZE);ImGui::SliderFloat("Intensity",&gridIntensity,0,1);ImGui::PopItemWidth();
      ImGui::Checkbox("Grid 1 Pixel",&onePixelGrid);
      ImGui::Separator();
      ImGui::Text("Backdrop");
      if (ImGui::Button("Load Backdrop")) {serialize = LOAD_BACKDROP;backdropOn=true;}
      ImGui::SameLine();
      ImGui::Text("%s",fileNameBackdrop.c_str());
      ImGui::Checkbox("Backdrop On", &backdropOn);
      ImGui::PushItemWidth(SLIDERSIZE);ImGui::SliderFloat("Transparency",&backdropTransparency,0,1);ImGui::PopItemWidth();
      ImGui::PushItemWidth(SLIDERSIZE);ImGui::SliderFloat("Saturation",&backdropSaturation,0,1);ImGui::PopItemWidth();
      ImGui::PushItemWidth(SLIDERSIZE);ImGui::SliderFloat("Contrast",&backdropContrast,0,5);ImGui::PopItemWidth();
      ImGui::PushItemWidth(SLIDERSIZE);ImGui::SliderFloat("Brightness",&backdropBrightness,-1,1);ImGui::PopItemWidth();
      if (ImGui::Button("Reset Values")) {
        backdropTransparency = 0.5;
        backdropSaturation = 1.0;
        backdropInFront = false;
        backdropContrast = 1.0;
        backdropBrightness = 0;
      }
      ImGui::SameLine();
      ImGui::Checkbox("Additive", &backdropInFront);
      ImGui::SameLine();
      if (ImGui::Button(hasUndo ? "Undo'U'" : "Redo'U'")) {
        doUndo();
        hasUndo = !hasUndo;
      }
      ImGui::Separator();
      if (ImGui::Button("Palettize Image")) {
        colorize();
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear Image")) {
        memset(bitmap,0,320*200*4);
        memset(converted,0,320*200*4);
        memset(backdrop,0,320*200*4);
        showTestConvert = false;
      }
      ImGui::Separator();
      ImGui::Text("Press 'F' to fill.");
      ImGui::Text("Press 'Q' for more performance.");
      ImGui::Text("Right mouse button = get color.");
      if (!showTestConvert ) {
        if (ImGui::Button("Test Convert 'S'")||currentKey=='s') {
          convert();
          showTestConvert = true;
        }
        if (multiColor) {
          ImGui::SameLine();
          ImGui::Text("Colors xy 0/0,0/1");
        }
      } else if (showTestConvert) {
        if (ImGui::Button("Clear Convert 'S'")||currentKey=='s')
          showTestConvert = false;
        ImGui::SameLine();
        ImGui::PushItemWidth(SLIDERSIZE);ImGui::SliderFloat("Alpha",&convertAlpha,0,1);ImGui::PopItemWidth();
      }
      if (ImGui::Button(multiColor ? "Save MultiColor .prg":"Save Hires .prg")) {
        serialize = SAVE_PRG;
      }
      ImGui::SameLine();
      ImGui::Text("%s",fileNamePRG.c_str());

      ImGui::Separator();
      ImGui::Checkbox("Ext",&displayColorSelector);
      ImGui::SameLine();
      float mouseSpeed2 = mouseSpeed;
      ImGui::PushItemWidth(SLIDERSIZE);ImGui::SliderFloat("MouseSpeed",&mouseSpeed2,0.1,2);ImGui::PopItemWidth();
      mouseSpeed = mouseSpeed2;
      ImGui::SameLine();
      if (ImGui::Button("Exit")) serialize = TRY_QUIT;

      ImGui::Separator();
      ImGui::End();
      mouseCaptured = ImGui::GetIO().WantCaptureMouse;
    } else {
      double mouseDeltaX, mouseDeltaY;
      glNextMouseDelta(&mouseDeltaX, &mouseDeltaY);
      mouseX += mouseDeltaX * mouseSpeed; mouseY += mouseDeltaY * mouseSpeed;
      if (mouseX < 0) mouseX = 0;
      if (mouseY < 0) mouseY = 0;
      if (mouseX > glFrameBufferWidth-1) mouseX = glFrameBufferWidth-1;
      if (mouseY > glFrameBufferHeight-1) mouseY = glFrameBufferHeight-1;
      mouseButtons = glMouseButtons();
      mouseCaptured = false;
    }


    glClearColor(10.f/255.f,40.f/255.f,30.f/255.f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    blitCanvas();
    blitLupe();
    blitPalette();
    if (displayColorSelector) blitColorSelect();
    if (menuEnabled)
      ImGui::Render();
    else {
      const float L = 0.0f;
      const float R = ImGui::GetIO().DisplaySize.x;
      const float B = ImGui::GetIO().DisplaySize.y;
      const float T = 0.0f;
      const float mvp[4][4] = 
      {
        { 2.0f/(R-L),	0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f/(T-B), 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { -(R+L)/(R-L), -(T+B)/(T-B), 0.0f, 1.0f },
      };
      
      // Setup orthographic projection
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glLoadMatrixf(&mvp[0][0]);
      drawMouseArrow(mouseX,mouseY);
    }
    glRefresh();
  }
  glDone();
  delete[] bitmap;
  delete[] converted;
  delete[] backdrop;
  delete[] undo;
  return 0;
}
