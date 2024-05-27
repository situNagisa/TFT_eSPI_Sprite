/**
 * @file      TFT_eSPI_Sprite.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-01-23
 * @note      Use TFT_eSPI Sprite made by framebuffer , unnecessary calling during use tft.xxxx function
 */


#include "esp_arduino_version.h"

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3,0,0)

#include <TFT_eSPI.h>   //https://github.com/Bodmer/TFT_eSPI
#include <LilyGo_RGBPanel.h>
#include <esp_heap_caps.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <strstream>
#include <filesystem>
#include <utility>
#include <vector>
#include <string>

#include "SPIFFS.h" 
#include "FS.h"
#include <SD.h>
#include "SD_MMC.h"

#include <Arduino.h>

LilyGo_RGBPanel panel;

#define WIDTH  panel.width()
#define HEIGHT panel.height()

struct image_t
{
  ::std::uint32_t duration;
  ::std::uint32_t width;
  ::std::uint32_t height;

  ::std::uint8_t* data() 
  {
    return (::std::uint8_t*)(this + 1);
  }
  const ::std::uint8_t* data() const
  {
    return (const ::std::uint8_t*)(this + 1);
  }
};

struct image_sequence
{
  void* _data = nullptr;
  ::std::size_t _size{};
  ::std::vector<::std::size_t> _offset{};

  image_sequence(const char* path)
  {
    File file = SD_MMC.open(path);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    _size = file.size();
    _data = (uint16_t *)malloc(_size);
    auto read_size = file.read((uint8_t*)_data, _size);

    Serial.printf("read %s, buffer(%p) size(%d) read size(%d)\n", path, _data, _size, read_size);

    auto data = (::std::uint8_t*)_data;
    for(::std::size_t i = 0; i < _size;)
    {
      _offset.push_back(i);
      auto& image = *(image_t*)(data + i);
      i += sizeof(image_t);
      i += image.width * image.height * sizeof(::std::uint16_t);

      Serial.printf("image duration %d width %d height %d offset %d\n",image.duration, image.width,image.height, i);
    }
    Serial.println();

    file.close();
  }

  ~image_sequence()
  {
    free(_data);
  }

  image_t& operator[](::std::size_t index) const
  {
    return *(image_t*)(_data + _offset[index]);
  }

  ::std::size_t size() const
  {
    return _offset.size();
  }
};

struct player
{
  const image_sequence* is;
  ::std::size_t current_frame;
  ::std::size_t total_frame;

  player() 
    : is(nullptr)
    , current_frame(0)
    , total_frame(0)
    {}

  void set_image_sequence(const image_sequence* is)
  {
    this->is = is;
    current_frame = 0;
    total_frame = is->size();
  }
  
  void next_frame()
  {
    current_frame++;
    current_frame %= total_frame;
  }

  void render()
  {
    auto&& image = current_image();
    panel.pushColors(0, 0, image.width, image.height, (uint16_t *)image.data());
  }

  const image_t& current_image() const { return (*is)[current_frame]; }
};

struct image_reader
{
  image_sequence* _data;

  image_reader() : _data(nullptr) {}
  ~image_reader()
  {
    delete _data;
  }
  void read(const char* file)
  {
    delete _data;
    _data = new image_sequence(file);
  }

  const image_sequence* data() const { return _data; }
};

player p{};
image_reader ir{};

#define SD_MISO_PIN (38)
#define SD_MOSI_PIN (40)
#define SD_SCLK_PIN (39)
void setup()
{
    Serial.begin(115200);
    delay(1000);

    SD_MMC.setPins(SD_SCLK_PIN, SD_MOSI_PIN, SD_MISO_PIN);
    if(!SD_MMC.begin("/sdcard", true))
    {
      Serial.println("Mount Failed");
      return;
    }
    Serial.println("Mount succeed");
    delay(1000);

    // Use TFT_eSPI Sprite made by framebuffer , unnecessary calling during use tft.xxxx function
    // Use TFT_eSPI sprites requires exchanging the color order
    if (!panel.begin(LILYGO_T_RGB_ORDER_BGR)) {
        while (1) {
            Serial.println("Error, failed to initialize T-RGB"); delay(1000);
        }
    }

    panel.setBrightness(16);

    Serial.println("start to read"); 
    delay(5000);

    ir.read("/image.is");
    p.set_image_sequence(ir.data());

    delay(1000);
}



void loop()
{
  return;
    if (Serial.available() > 0)//串口接收到数据
    {
      int incode = Serial.read();//获取串口接收到的数据
      ::std::string path = ::std::string("/") + (char)(incode)+".is";
      File file = SD_MMC.open(path.c_str());
      if(file){
          Serial.printf("open success %s\n", path.c_str());
          file.close();
          ir.read(path.c_str());
          p.set_image_sequence(ir.data());
      }
    }
    p.render();
    delay(p.current_image().duration);
    p.next_frame();
}

#else

#include <Arduino.h>

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    Serial.println("The current arduino version of TFT_eSPI does not support arduino 3.0, please change the version to below 3.0");
    delay(1000);
}

#endif