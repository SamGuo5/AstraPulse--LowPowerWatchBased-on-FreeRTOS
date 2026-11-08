#include "feature_icons.hpp"

#include <algorithm>
#include <cstdlib>

namespace {
class IconCanvas final {
 public:
  static constexpr int width=30;
  static constexpr int height=30;
  static constexpr int stride=4;

  void pixel(int x,int y,bool on=true){
    if(x<0 || x>=width || y<0 || y>=height)return;
    const auto mask=static_cast<uint8_t>(1u<<(x&7));
    auto &value=data_[static_cast<size_t>(y*stride+x/8)];
    if(on)value|=mask;
    else value&=static_cast<uint8_t>(~mask);
  }

  void line(int x0,int y0,int x1,int y1,int thickness=1){
    const int dx=std::abs(x1-x0),step_x=x0<x1?1:-1;
    const int dy=-std::abs(y1-y0),step_y=y0<y1?1:-1;
    int error=dx+dy;
    for(;;){
      const int offset=(thickness-1)/2;
      for(int y=-offset;y<thickness-offset;++y)
        for(int x=-offset;x<thickness-offset;++x)pixel(x0+x,y0+y);
      if(x0==x1 && y0==y1)break;
      const int twice=2*error;
      if(twice>=dy){error+=dy;x0+=step_x;}
      if(twice<=dx){error+=dx;y0+=step_y;}
    }
  }

  void hline(int x,int y,int length,int thickness=1){line(x,y,x+length-1,y,thickness);}
  void vline(int x,int y,int length,int thickness=1){line(x,y,x,y+length-1,thickness);}

  void rect(int x,int y,int w,int h,int thickness=1){
    for(int index=0;index<thickness;++index){
      hline(x+index,y+index,w-index*2);
      hline(x+index,y+h-1-index,w-index*2);
      vline(x+index,y+index,h-index*2);
      vline(x+w-1-index,y+index,h-index*2);
    }
  }

  void fill_rect(int x,int y,int w,int h,bool on=true){
    for(int row=0;row<h;++row)
      for(int column=0;column<w;++column)pixel(x+column,y+row,on);
  }

  void circle(int center_x,int center_y,int radius){
    int x=radius,y=0,error=1-radius;
    while(x>=y){
      pixel(center_x+x,center_y+y);pixel(center_x+y,center_y+x);
      pixel(center_x-y,center_y+x);pixel(center_x-x,center_y+y);
      pixel(center_x-x,center_y-y);pixel(center_x-y,center_y-x);
      pixel(center_x+y,center_y-x);pixel(center_x+x,center_y-y);
      ++y;
      if(error<0)error+=2*y+1;
      else{--x;error+=2*(y-x)+1;}
    }
  }

  void fill_circle(int center_x,int center_y,int radius,bool on=true){
    for(int y=-radius;y<=radius;++y)
      for(int x=-radius;x<=radius;++x)
        if(x*x+y*y<=radius*radius)pixel(center_x+x,center_y+y,on);
  }

  void capsule(int x0,int y0,int x1,int y1,int thickness,bool on=true){
    if(!on){
      line(x0,y0,x1,y1,thickness);
      return;
    }
    line(x0,y0,x1,y1,thickness);
    const int radius=thickness/2;
    fill_circle(x0,y0,radius);fill_circle(x1,y1,radius);
  }

  void fill_round_rect(int x,int y,int w,int h,int radius,bool on=true){
    radius=std::max(0,std::min(radius,std::min(w,h)/2));
    fill_rect(x+radius,y,w-2*radius,h,on);
    fill_rect(x,y+radius,w,h-2*radius,on);
    fill_circle(x+radius,y+radius,radius,on);
    fill_circle(x+w-1-radius,y+radius,radius,on);
    fill_circle(x+radius,y+h-1-radius,radius,on);
    fill_circle(x+w-1-radius,y+h-1-radius,radius,on);
  }

  void round_frame(int x,int y,int w,int h,int radius,int thickness=1){
    fill_round_rect(x,y,w,h,radius);
    if(w>2*thickness && h>2*thickness)
      fill_round_rect(x+thickness,y+thickness,w-2*thickness,h-2*thickness,
                      std::max(0,radius-thickness),false);
  }

  void clear_rect(int x,int y,int w,int h){fill_rect(x,y,w,h,false);}
  std::vector<uint8_t> take(){return std::move(data_);}

 private:
  std::vector<uint8_t> data_=std::vector<uint8_t>(height*stride,0);
};

void settings(IconCanvas &canvas){
  canvas.circle(15,15,8);canvas.circle(15,15,3);
  canvas.capsule(15,3,15,7,2);canvas.capsule(15,23,15,27,2);
  canvas.capsule(3,15,7,15,2);canvas.capsule(23,15,27,15,2);
  canvas.capsule(7,7,10,10,2);canvas.capsule(20,20,23,23,2);
  canvas.capsule(23,7,20,10,2);canvas.capsule(10,20,7,23,2);
}

void personal(IconCanvas &canvas){
  canvas.circle(15,8,5);canvas.capsule(11,15,8,18,2);
  canvas.capsule(19,15,22,18,2);canvas.capsule(8,18,5,26,2);
  canvas.capsule(22,18,25,26,2);canvas.hline(5,27,21,2);
  canvas.capsule(11,16,9,25,2);canvas.capsule(19,16,21,25,2);
}

void climate(IconCanvas &canvas){
  canvas.round_frame(3,3,9,24,4,2);canvas.capsule(7,10,7,22,2);
  canvas.fill_circle(7,23,3);
  canvas.line(21,4,15,16,2);canvas.line(21,4,27,16,2);
  canvas.line(15,16,15,20,2);canvas.line(27,16,27,20,2);
  canvas.line(15,20,18,24,2);canvas.line(27,20,24,24,2);
  canvas.capsule(18,24,24,24,2);
}

void heart_oxygen(IconCanvas &canvas){
  canvas.line(3,10,4,7,2);canvas.line(4,7,7,6,2);
  canvas.line(7,6,11,9,2);canvas.line(11,9,15,6,2);
  canvas.line(15,6,18,7,2);canvas.line(18,7,20,10,2);
  canvas.line(20,10,19,13,2);canvas.line(19,13,11,23,2);
  canvas.line(11,23,3,13,2);canvas.line(3,13,3,10,2);
  canvas.circle(24,17,4);canvas.hline(24,23,4);canvas.pixel(27,24);canvas.hline(24,26,4);
}

void motion(IconCanvas &canvas){
  canvas.line(3,22,7,16,2);canvas.line(7,16,12,19,2);
  canvas.line(12,19,17,20,2);canvas.line(17,20,21,16,2);
  canvas.line(21,16,24,20,2);canvas.line(24,20,27,21,2);
  canvas.line(27,21,26,26,2);canvas.hline(5,27,21,2);
  canvas.line(3,22,5,27,2);canvas.hline(9,22,4);canvas.hline(14,23,4);
}

void dino(IconCanvas &canvas){
  canvas.line(3,17,9,17,2);canvas.line(3,17,7,12,2);
  canvas.line(9,17,10,12,2);canvas.line(10,12,16,10,2);
  canvas.line(16,10,16,5,2);canvas.line(16,5,27,5,2);
  canvas.line(27,5,27,14,2);canvas.hline(21,14,7,2);
  canvas.line(21,14,20,21,2);canvas.line(20,21,24,26,2);
  canvas.hline(20,27,6,2);canvas.line(16,20,14,26,2);canvas.hline(11,27,5,2);
  canvas.line(14,26,9,22,2);canvas.line(9,22,9,17,2);canvas.fill_circle(24,8,1);
}

void date_time(IconCanvas &canvas){
  canvas.round_frame(2,5,26,23,4,2);canvas.hline(3,11,24,2);
  canvas.capsule(9,3,9,8,2);canvas.capsule(21,3,21,8,2);
  canvas.circle(20,20,7);canvas.capsule(20,20,20,16,2);
  canvas.capsule(20,20,24,22,2);canvas.fill_circle(20,20,1);
}

void alarm(IconCanvas &canvas){
  canvas.circle(15,16,10);canvas.capsule(15,16,15,10,2);
  canvas.capsule(15,16,20,18,2);canvas.fill_circle(15,16,1);
  canvas.capsule(5,7,9,3,2);canvas.capsule(21,3,25,7,2);
  canvas.capsule(8,24,5,27,2);canvas.capsule(22,24,25,27,2);
}

void stopwatch(IconCanvas &canvas){
  canvas.circle(15,17,10);canvas.round_frame(11,2,9,5,2,2);
  canvas.capsule(15,5,15,7,2);canvas.capsule(22,9,25,6,2);
  canvas.capsule(15,17,15,11,2);canvas.capsule(15,17,20,20,2);
  canvas.fill_circle(15,17,1);
}

void sound(IconCanvas &canvas){
  canvas.rect(3,12,6,7,2);canvas.line(9,12,15,7,2);
  canvas.line(9,18,15,23,2);canvas.vline(15,7,17,2);
  canvas.capsule(19,11,21,14,2);canvas.capsule(21,14,21,17,2);
  canvas.capsule(21,17,19,20,2);canvas.capsule(23,7,27,12,2);
  canvas.capsule(27,12,27,19,2);canvas.capsule(27,19,23,24,2);
}

void brightness(IconCanvas &canvas){
  canvas.circle(15,15,6);
  canvas.capsule(15,2,15,6,2);canvas.capsule(15,24,15,28,2);
  canvas.capsule(2,15,6,15,2);canvas.capsule(24,15,28,15,2);
  canvas.capsule(6,6,9,9,2);canvas.capsule(21,21,24,24,2);
  canvas.capsule(24,6,21,9,2);canvas.capsule(9,21,6,24,2);
}
}

std::vector<uint8_t> make_feature_icon(FeatureIcon icon){
  IconCanvas canvas;
  switch(icon){
    case FeatureIcon::SystemSettings:settings(canvas);break;
    case FeatureIcon::PersonalInfo:personal(canvas);break;
    case FeatureIcon::Climate:climate(canvas);break;
    case FeatureIcon::HeartOxygen:heart_oxygen(canvas);break;
    case FeatureIcon::Motion:motion(canvas);break;
    case FeatureIcon::DinoGame:dino(canvas);break;
    case FeatureIcon::DateTime:date_time(canvas);break;
    case FeatureIcon::Alarm:alarm(canvas);break;
    case FeatureIcon::Stopwatch:stopwatch(canvas);break;
    case FeatureIcon::Sound:sound(canvas);break;
    case FeatureIcon::Brightness:brightness(canvas);break;
  }
  return canvas.take();
}
