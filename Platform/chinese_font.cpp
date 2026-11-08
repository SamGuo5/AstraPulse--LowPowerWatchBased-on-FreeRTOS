#include "watch_chinese_font.hpp"

namespace watch_font {
namespace {
uint32_t next_codepoint(const char *&cursor,const char *end) {
  if(cursor>=end)return 0;
  const uint8_t first=static_cast<uint8_t>(*cursor++);
  if(first<0x80)return first;
  uint32_t value=0;
  uint8_t remaining=0;
  if((first&0xe0)==0xc0){value=first&0x1f;remaining=1;}
  else if((first&0xf0)==0xe0){value=first&0x0f;remaining=2;}
  else if((first&0xf8)==0xf0){value=first&0x07;remaining=3;}
  else return 0xfffd;
  while(remaining--){
    if(cursor>=end)return 0xfffd;
    const uint8_t part=static_cast<uint8_t>(*cursor++);
    if((part&0xc0)!=0x80)return 0xfffd;
    value=(value<<6)|(part&0x3f);
  }
  return value;
}

const Glyph *find(uint32_t codepoint) {
  std::size_t low=0,high=glyph_count;
  while(low<high){
    const std::size_t middle=low+(high-low)/2;
    if(glyphs[middle].codepoint<codepoint)low=middle+1;
    else high=middle;
  }
  return low<glyph_count&&glyphs[low].codepoint==codepoint?&glyphs[low]:nullptr;
}
}

uint16_t utf8_width(const std::string &text) {
  uint16_t width=0;
  const char *cursor=text.data();
  const char *end=cursor+text.size();
  while(cursor<end){
    const Glyph *glyph=find(next_codepoint(cursor,end));
    width+=glyph?glyph->advance:12;
  }
  return width;
}

void draw_utf8(u8g2_t *canvas,int16_t x,int16_t baseline,const std::string &text) {
  const char *cursor=text.data();
  const char *end=cursor+text.size();
  while(cursor<end){
    const Glyph *glyph=find(next_codepoint(cursor,end));
    if(!glyph){x+=12;continue;}
    const int16_t top=baseline-glyph->height-glyph->y_offset;
    for(uint8_t row=0;row<glyph->height;++row){
      const uint8_t *source=bitmap+glyph->bitmap_offset+row*glyph->row_bytes;
      for(uint8_t column=0;column<glyph->width;++column){
        if(source[column/8]&(0x80u>>(column%8))){
          u8g2_DrawPixel(canvas,x+glyph->x_offset+column,top+row);
        }
      }
    }
    x+=glyph->advance;
  }
}
}
