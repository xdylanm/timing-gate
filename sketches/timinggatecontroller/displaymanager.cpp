#include "displaymanager.h"
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

DisplayManager::DisplayManager(
    int const w /*=128*/,
    int const h /*=64*/,
    int const addr /*=0x3C*/) 
    : title_canvas_(w,FreeSans12pt7b.yAdvance-6), subtitle_canvas_(w,FreeSans9pt7b.yAdvance), 
    status_canvas_(w, h - FreeSans12pt7b.yAdvance + 4), display_(w,h,&Wire,OLED_RESET), 
    addr_(addr), left_margin_(0)
{
  title_canvas_.setFont(&FreeSans12pt7b);
  title_canvas_.setTextSize(1);
  title_canvas_.setTextColor(SSD1306_WHITE);

  subtitle_canvas_.setFont(&FreeSans9pt7b);
  subtitle_canvas_.setTextSize(1);
  subtitle_canvas_.setTextColor(SSD1306_WHITE);

  status_canvas_.setFont(&FreeMono9pt7b);
  status_canvas_.setTextSize(1);
  status_canvas_.setTextColor(SSD1306_WHITE);
}

bool DisplayManager::begin(const char* title, const char* subtitle)
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display_.begin(SSD1306_SWITCHCAPVCC, addr_)) {
    return false;
  }
  display_.clearDisplay();
  update_title(title);
  update_subtitle(subtitle);
  display_.display();
  return true;
}

void DisplayManager::clear_all() 
{
  display_.clearDisplay();
}

void DisplayManager::update_rect(
    int const x, 
    int const y, 
    const char* msg, 
    int const lmargin,  
    GFXcanvas1& canvas)
{
  canvas.fillScreen(SSD1306_BLACK);
  canvas.setCursor(lmargin, canvas.height()-4);
  canvas.print(msg);

  display_.drawBitmap(x,y,canvas.getBuffer(),canvas.width(),canvas.height(),
      SSD1306_WHITE,SSD1306_BLACK);
}

void DisplayManager::update_vcenter_title(const char* msg)
{
  int const y0 = (display_.height() - title_canvas_.height())/2;
  update_rect(0,y0,msg,left_margin_,title_canvas_);
}

void DisplayManager::update_title(const char* msg)
{
  update_rect(0,0,msg,left_margin_,title_canvas_);
}

void DisplayManager::update_subtitle(const char* msg)
{
  update_rect(0,title_canvas_.height(),msg,left_margin_+4,subtitle_canvas_);
}

void DisplayManager::update_status(const char* ln0, const char* ln1, const char* ln2) 
{
  int y0 = title_canvas_.height();
  int const dy = status_canvas_.height()/3;

  status_canvas_.fillScreen(SSD1306_BLACK);

  if (ln0) {
    status_canvas_.setCursor(left_margin_+4, dy);
    status_canvas_.print(ln0);
  }
  if (ln1) {
    status_canvas_.setCursor(left_margin_+4, 2*dy);
    status_canvas_.print(ln1);
  }
  if (ln2) {
    status_canvas_.setCursor(left_margin_+4, 3*dy);
    status_canvas_.print(ln2);
  }

  display_.drawBitmap(0,y0,status_canvas_.getBuffer(),status_canvas_.width(),status_canvas_.height(),
      SSD1306_WHITE,SSD1306_BLACK);

}
