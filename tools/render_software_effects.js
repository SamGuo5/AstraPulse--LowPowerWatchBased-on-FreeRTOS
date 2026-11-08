const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const root = path.resolve(__dirname, '..');
const output = path.join(root, '软件效果图');
const iconOutput = path.join(output, '菜单图标');
const pageOutput = path.join(output, '功能页面');
fs.mkdirSync(iconOutput, {recursive: true});
fs.mkdirSync(pageOutput, {recursive: true});

const fontSource = fs.readFileSync(path.join(root, 'Assets/fonts/watch_chinese_font_data.cpp'), 'utf8');
const glyphs = new Map();
for (const match of fontSource.matchAll(/\{0x([0-9a-f]+),(\d+),(\d+),(\d+),(\d+),(\d+),(-?\d+),(-?\d+)\}/gi)) {
  const values = match.slice(1).map((value, index) => index === 0 ? parseInt(value, 16) : Number(value));
  glyphs.set(values[0], {offset: values[1], width: values[2], height: values[3], rowBytes: values[4], advance: values[5], xOffset: values[6], yOffset: values[7]});
}
const bitmapBody = fontSource.match(/const uint8_t bitmap\[\]=\{([\s\S]*?)\};/)[1];
const fontBitmap = Uint8Array.from([...bitmapBody.matchAll(/0x([0-9a-f]{2})/gi)].map(match => parseInt(match[1], 16)));

class Canvas {
  constructor(width = 128, height = 64) {
    this.width = width; this.height = height; this.data = new Uint8Array(width * height); this.mode = 1;
  }
  clear() { this.data.fill(0); this.mode = 1; }
  setMode(mode) { this.mode = mode; }
  pixel(x, y, on = true) {
    x = Math.round(x); y = Math.round(y);
    if (x < 0 || y < 0 || x >= this.width || y >= this.height) return;
    const index = y * this.width + x;
    if (!on || this.mode === 0) this.data[index] = 0;
    else if (this.mode === 2) this.data[index] ^= 1;
    else this.data[index] = 1;
  }
  line(x0, y0, x1, y1, thickness = 1) {
    x0 = Math.round(x0); y0 = Math.round(y0); x1 = Math.round(x1); y1 = Math.round(y1);
    const dx = Math.abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const dy = -Math.abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    let error = dx + dy;
    for (;;) {
      const offset = Math.floor((thickness - 1) / 2);
      for (let yy = -offset; yy < thickness - offset; ++yy)
        for (let xx = -offset; xx < thickness - offset; ++xx) this.pixel(x0 + xx, y0 + yy);
      if (x0 === x1 && y0 === y1) break;
      const twice = 2 * error;
      if (twice >= dy) { error += dy; x0 += sx; }
      if (twice <= dx) { error += dx; y0 += sy; }
    }
  }
  hline(x, y, width, thickness = 1) { if (width > 0) this.line(x, y, x + width - 1, y, thickness); }
  vline(x, y, height, thickness = 1) { if (height > 0) this.line(x, y, x, y + height - 1, thickness); }
  box(x, y, width, height) {
    for (let yy = 0; yy < Math.round(height); ++yy)
      for (let xx = 0; xx < Math.round(width); ++xx) this.pixel(x + xx, y + yy);
  }
  frame(x, y, width, height, thickness = 1) {
    for (let i = 0; i < thickness; ++i) {
      this.hline(x + i, y + i, width - i * 2); this.hline(x + i, y + height - 1 - i, width - i * 2);
      this.vline(x + i, y + i, height - i * 2); this.vline(x + width - 1 - i, y + i, height - i * 2);
    }
  }
  fillCircle(cx, cy, radius, on = true) {
    const saved = this.mode; if (!on) this.mode = 0;
    for (let y = -radius; y <= radius; ++y)
      for (let x = -radius; x <= radius; ++x) if (x * x + y * y <= radius * radius) this.pixel(cx + x, cy + y);
    this.mode = saved;
  }
  circle(cx, cy, radius) {
    let x = radius, y = 0, error = 1 - radius;
    while (x >= y) {
      [[x,y],[y,x],[-y,x],[-x,y],[-x,-y],[-y,-x],[y,-x],[x,-y]].forEach(([xx,yy]) => this.pixel(cx + xx, cy + yy));
      ++y; if (error < 0) error += 2 * y + 1; else { --x; error += 2 * (y - x) + 1; }
    }
  }
  rbox(x, y, width, height, radius) {
    radius = Math.max(0, Math.min(Math.round(radius), Math.floor(Math.min(width, height) / 2)));
    this.box(x + radius, y, width - radius * 2, height); this.box(x, y + radius, width, height - radius * 2);
    this.fillCircle(x + radius, y + radius, radius); this.fillCircle(x + width - 1 - radius, y + radius, radius);
    this.fillCircle(x + radius, y + height - 1 - radius, radius); this.fillCircle(x + width - 1 - radius, y + height - 1 - radius, radius);
  }
  rframe(x, y, width, height, radius, thickness = 1) {
    const saved = this.mode; this.rbox(x, y, width, height, radius);
    this.mode = 0; if (width > thickness * 2 && height > thickness * 2) this.rbox(x + thickness, y + thickness, width - thickness * 2, height - thickness * 2, Math.max(0, radius - thickness));
    this.mode = saved;
  }
  dotted(x, y, width) { for (let xx = 0; xx < width; xx += 3) this.pixel(x + xx, y); }
  widthOf(text) { let width = 0; for (const char of text) width += (glyphs.get(char.codePointAt(0)) || {advance: 12}).advance; return width; }
  text(x, baseline, text) {
    x = Math.round(x); baseline = Math.round(baseline);
    for (const char of text) {
      const glyph = glyphs.get(char.codePointAt(0));
      if (!glyph) { x += 12; continue; }
      const top = baseline - glyph.height - glyph.yOffset;
      for (let row = 0; row < glyph.height; ++row) {
        const source = glyph.offset + row * glyph.rowBytes;
        for (let column = 0; column < glyph.width; ++column)
          if (fontBitmap[source + Math.floor(column / 8)] & (0x80 >> (column % 8))) this.pixel(x + glyph.xOffset + column, top + row);
      }
      x += glyph.advance;
    }
  }
  centered(baseline, text) { this.text((this.width - this.widthOf(text)) / 2, baseline, text); }
  blit(source, x, y) { for (let yy = 0; yy < source.height; ++yy) for (let xx = 0; xx < source.width; ++xx) if (source.data[yy * source.width + xx]) this.pixel(x + xx, y + yy); }
}

const segments = [0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f];
function sevenDigit(c, x, y, value, compact = false) {
  const hWidth = compact ? 6 : 8, xRight = compact ? 8 : 10, yMid = compact ? 7 : 10, yBottom = compact ? 14 : 20, vHeight = compact ? 5 : 8;
  const h = (xx, yy) => c.box(xx + (compact ? 1 : 2), yy, hWidth, 2);
  const v = (xx, yy) => c.box(xx, yy, 2, vHeight);
  const mask = segments[value % 10];
  if (mask & 1) h(x,y); if(mask & 2) v(x+xRight,y+2); if(mask & 4) v(x+xRight,y+yMid+2);
  if(mask & 8) h(x,y+yBottom); if(mask & 16) v(x,y+yMid+2); if(mask & 32) v(x,y+2); if(mask & 64) h(x,y+yMid);
}
function bigTime(c, hour, minute, x = 33, y = 18) {
  sevenDigit(c,x,y,Math.floor(hour/10)); sevenDigit(c,x+14,y,hour%10); c.box(x+29,y+6,2,2); c.box(x+29,y+14,2,2);
  sevenDigit(c,x+34,y,Math.floor(minute/10)); sevenDigit(c,x+48,y,minute%10);
}
function title(c, text, right = '') { c.centered(11,text); if(right) c.text(103,11,right); c.dotted(0,14,128); }
function switchControl(c, x, y, enabled, width = 28, height = 12) {
  c.rframe(x,y,width,height,5); if(enabled){c.rbox(x,y,width,height,5);c.setMode(0);c.rbox(x+width-10,y+2,8,height-4,4);c.setMode(1);} else c.rbox(x+2,y+2,8,height-4,4);
}

function iconCanvas(draw) { const c = new Canvas(30,30); draw(c); return c; }
function capsule(c,x0,y0,x1,y1,t=2){c.line(x0,y0,x1,y1,t);c.fillCircle(x0,y0,Math.floor(t/2));c.fillCircle(x1,y1,Math.floor(t/2));}
const iconDrawers = {
  '系统设置': c=>{c.circle(15,15,8);c.circle(15,15,3);[[15,3,15,7],[15,23,15,27],[3,15,7,15],[23,15,27,15],[7,7,10,10],[20,20,23,23],[23,7,20,10],[10,20,7,23]].forEach(v=>capsule(c,...v));},
  '个人信息': c=>{c.circle(15,8,5);[[11,15,8,18],[19,15,22,18],[8,18,5,26],[22,18,25,26],[11,16,9,25],[19,16,21,25]].forEach(v=>capsule(c,...v));c.hline(5,27,21,2);},
  '温湿度': c=>{c.rframe(3,3,9,24,4,2);capsule(c,7,10,7,22);c.fillCircle(7,23,3);c.line(21,4,15,16,2);c.line(21,4,27,16,2);c.line(15,16,15,20,2);c.line(27,16,27,20,2);c.line(15,20,18,24,2);c.line(27,20,24,24,2);capsule(c,18,24,24,24);},
  '心率血氧': c=>{[[3,10,4,7],[4,7,7,6],[7,6,11,9],[11,9,15,6],[15,6,18,7],[18,7,20,10],[20,10,19,13],[19,13,11,23],[11,23,3,13],[3,13,3,10]].forEach(v=>c.line(...v,2));c.circle(24,17,4);c.hline(24,23,4);c.pixel(27,24);c.hline(24,26,4);},
  '运动监测': c=>{[[3,22,7,16],[7,16,12,19],[12,19,17,20],[17,20,21,16],[21,16,24,20],[24,20,27,21],[27,21,26,26]].forEach(v=>c.line(...v,2));c.hline(5,27,21,2);c.line(3,22,5,27,2);c.hline(9,22,4);c.hline(14,23,4);},
  '恐龙游戏': c=>{[[3,17,9,17],[3,17,7,12],[9,17,10,12],[10,12,16,10],[16,10,16,5],[16,5,27,5],[27,5,27,14],[21,14,20,21],[20,21,24,26],[16,20,14,26],[14,26,9,22],[9,22,9,17]].forEach(v=>c.line(...v,2));c.hline(21,14,7,2);c.hline(20,27,6,2);c.hline(11,27,5,2);c.fillCircle(24,8,1);},
  '日期时间': c=>{c.rframe(2,5,26,23,4,2);c.hline(3,11,24,2);capsule(c,9,3,9,8);capsule(c,21,3,21,8);c.circle(20,20,7);capsule(c,20,20,20,16);capsule(c,20,20,24,22);c.fillCircle(20,20,1);},
  '闹钟设置': c=>{c.circle(15,16,10);capsule(c,15,16,15,10);capsule(c,15,16,20,18);c.fillCircle(15,16,1);[[5,7,9,3],[21,3,25,7],[8,24,5,27],[22,24,25,27]].forEach(v=>capsule(c,...v));},
  '秒表': c=>{c.circle(15,17,10);c.rframe(11,2,9,5,2,2);capsule(c,15,5,15,7);capsule(c,22,9,25,6);capsule(c,15,17,15,11);capsule(c,15,17,20,20);c.fillCircle(15,17,1);},
  '声音设置': c=>{c.frame(3,12,6,7,2);c.line(9,12,15,7,2);c.line(9,18,15,23,2);c.vline(15,7,17,2);[[19,11,21,14],[21,14,21,17],[21,17,19,20],[23,7,27,12],[27,12,27,19],[27,19,23,24]].forEach(v=>capsule(c,...v));},
  '屏幕亮度': c=>{c.circle(15,15,6);[[15,2,15,6],[15,24,15,28],[2,15,6,15],[24,15,28,15],[6,6,9,9],[21,21,24,24],[24,6,21,9],[9,21,6,24]].forEach(v=>capsule(c,...v));}
};
const menuNames = Object.keys(iconDrawers);

function menuScreen(name, index) {
  const c = new Canvas(); const icon = iconCanvas(iconDrawers[name]);
  c.box(0,0,128*(index+1)/menuNames.length,2); c.blit(icon,49,8);
  c.setMode(2); const x=46,y=5,w=36,h=36,l=6; c.hline(x,y,l);c.vline(x,y,l);c.hline(x,y+h-1,l);c.vline(x,y+h-l,l);c.hline(x+w-l,y,l);c.vline(x+w-1,y,l);c.hline(x+w-l,y+h-1,l);c.vline(x+w-1,y+h-l,l);c.setMode(1);
  c.dotted(0,46,128); c.hline(4,56,6);c.pixel(5,55);c.pixel(6,54);c.pixel(5,57);c.pixel(6,58);c.hline(118,56,6);c.pixel(122,55);c.pixel(121,54);c.pixel(122,57);c.pixel(121,58);
  c.hline(16,58,9);c.box(18,54,5,4);c.hline(103,58,9);c.box(105,54,5,4); c.centered(60,name); return c;
}

function systemPage(){const c=new Canvas();title(c,'系统设置');[['指示灯','ON'],['心率 LOW','40'],['心率 HIGH','120']].forEach((r,i)=>{const y=16+i*13;if(i===0)c.rframe(2,y,124,13,4);c.text(9,y+10,r[0]);c.text(119-c.widthOf(r[1]),y+10,r[1]);});c.text(2,63,'WDG OK S:224');return c;}
function personalPage(){const c=new Canvas();title(c,'个人信息','MEM');[[3,'身高','175','cm'],[67,'体重','65','kg']].forEach((v,i)=>{if(i===0)c.rframe(v[0],18,58,34,5);c.text(v[0]+6,30,v[1]);c.text(v[0]+8,46,v[2]);c.text(v[0]+38,46,v[3]);});c.text(2,63,'UP/DN SELECT');c.text(104,63,'OK');return c;}
function climatePage(){const c=new Canvas();title(c,'温湿度','C');c.rframe(1,17,62,33,5);c.rframe(65,17,62,33,5);c.text(6,27,'TEMP');c.text(70,27,'RH');sevenDigit(c,9,30,2,true);sevenDigit(c,20,30,5,true);c.text(32,45,'.6');c.text(51,45,'C');sevenDigit(c,73,30,5,true);sevenDigit(c,84,30,8,true);c.text(96,45,'.2');c.text(116,45,'%');c.centered(62,'COMFORT');return c;}
function smallDigit(c,x,y,n){const rows=[[7,5,5,5,7],[2,6,2,2,7],[7,1,7,4,7],[7,1,7,1,7],[5,5,7,1,1],[7,4,7,1,7],[7,4,7,5,7],[7,1,2,2,2],[7,5,7,5,7],[7,5,7,1,7]][n%10];for(let r=0;r<5;r++)for(let col=0;col<3;col++)if(rows[r]&(4>>col))c.box(x+col*2,y+r*2,2,2);}
function healthPage(){const c=new Canvas();title(c,'心率血氧');c.text(5,25,'HR');c.text(69,25,'SPO2');smallDigit(c,24,27,7);smallDigit(c,31,27,2);smallDigit(c,82,27,9);smallDigit(c,89,27,8);c.text(47,37,'BPM');c.text(116,37,'%');let last=50;for(let x=0;x<128;x+=2){const y=50-Math.round(7*Math.sin(x/7)+3*Math.sin(x/3));c.vline(x,Math.min(last,y),Math.abs(last-y)+1);c.hline(x,y,2);last=y;}c.text(108,63,'Q92');return c;}
function motionPage(){const c=new Canvas();title(c,'运动监测','DEMO');c.text(5,27,'STEPS');c.text(79,27,'ACTIVITY');c.text(8,45,'1286');c.text(88,45,'64%');c.frame(4,50,120,6);c.box(6,52,75,2);c.text(3,63,'WALK');c.text(78,63,'DEMO DATA');return c;}
function drawXbm(c,x,y,width,height,bytes){const stride=Math.ceil(width/8);for(let row=0;row<height;row++)for(let column=0;column<width;column++)if(bytes[row*stride+Math.floor(column/8)]&(1<<(column&7)))c.pixel(x+column,y+row);}
const dinoRunA=[0x00,0x7e,0x00,0x00,0xff,0x01,0x00,0xfb,0x01,0x00,0xff,0x01,0x00,0xdf,0x01,0x00,0x3f,0x00,0x84,0x3f,0x00,0xce,0x3f,0x00,0xff,0x1f,0x00,0xfe,0xdf,0x00,0xfc,0x67,0x00,0xf8,0x03,0x00,0xb8,0x03,0x00,0x18,0x03,0x00,0x1c,0x07,0x00,0x3c,0x0f,0x00];
function dinoPage(){const c=new Canvas();c.text(3,10,'00000  HI 00356');c.dotted(0,13,128);c.hline(0,54,128);for(let i=0;i<10;i++){const x=(i*17+128)%136-4;c.hline(x,58,6);c.pixel(x+8,56);}c.hline(119,19,8);c.hline(116,21,17);c.pixel(118,20);c.pixel(128,20);c.pixel(132,20);drawXbm(c,12,38,18,16,dinoRunA);c.rframe(37,21,77,21,5);c.text(51,30,'DINO RUN');c.text(49,40,'OK START');return c;}
function datePage(){const c=new Canvas();title(c,'日期时间');bigTime(c,10,28,25,18);c.text(101,29,'36S');c.text(105,40,'RTC');c.dotted(0,44,128);c.centered(59,'2026-11-08 SUN');return c;}
function bell(c,x,y){c.rframe(x+2,y+2,10,9,4);c.box(x+1,y+10,12,2);c.box(x+6,y,2,2);c.box(x+6,y+12,2,2);}
function alarmPage(){const c=new Canvas();title(c,'闹钟设置');[[17,'A1','07:30',true],[36,'A2','18:00',false]].forEach((r,i)=>{if(i===0)c.rframe(2,r[0],124,17,4);bell(c,7,r[0]+2);c.text(24,r[0]+12,r[1]);c.text(43,r[0]+12,r[2]);switchControl(c,99,r[0]+3,r[3],24,10);});c.text(2,62,'UP/DN SELECT');c.text(101,62,'OK');return c;}
function stopwatchPage(){const c=new Canvas();title(c,'秒表','RUN');const x=20,y=20;sevenDigit(c,x,y,0,true);sevenDigit(c,x+12,y,1,true);c.box(x+25,y+5,2,2);c.box(x+25,y+12,2,2);sevenDigit(c,x+30,y,2,true);sevenDigit(c,x+42,y,8,true);c.text(87,38,'.42');c.frame(3,42,122,5);c.box(5,44,48,1);c.text(3,61,'OK');c.text(18,61,'暂停');c.vline(75,52,8);c.hline(72,57,7);c.text(86,61,'复位');return c;}
function soundPage(){const c=new Canvas();title(c,'声音设置');[['总声音',true],['按键音',true]].forEach((r,i)=>{const y=18+i*20;if(i===0)c.rframe(2,y,124,19,5);c.text(10,y+13,r[0]);switchControl(c,94,y+3,r[1]);});c.text(2,63,'UP/DN SELECT');c.text(104,63,'OK');return c;}
function displayPage(){const c=new Canvas();title(c,'屏幕亮度');c.rframe(10,26,10,10,5);c.box(13,29,4,4);c.hline(5,30,4);c.hline(21,30,4);c.vline(14,21,4);c.vline(14,37,4);c.frame(5,44,20,5);c.box(7,46,13,1);[['亮度','L6'],['休眠','15S']].forEach((r,i)=>{const y=18+i*20;if(i===0)c.rframe(30,y,96,18,5);c.text(38,y+12,r[0]);c.text(118-c.widthOf(r[1]),y+12,r[1]);});c.text(2,63,'UP/DN SELECT');c.text(104,63,'OK');return c;}
const pageRenderers=[systemPage,personalPage,climatePage,healthPage,motionPage,dinoPage,datePage,alarmPage,stopwatchPage,soundPage,displayPage];

function bootFrame(elapsed) {
  const c=new Canvas(); const clamp=v=>Math.max(0,Math.min(1,v)); const phase=(s,d)=>elapsed<=s?0:clamp((elapsed-s)/d); const ease=v=>{v=clamp(v);return v*v*(3-2*v)}; const mix=(a,b,t)=>a+(b-a)*t;
  const eye=(cx,cy,w,h,px=0,pupil=false)=>{c.rbox(cx-w/2,cy-h/2,w,h,Math.max(1,Math.min(w,h)*.28));if(pupil&&w>=14&&h>=10){c.setMode(0);c.rbox(cx-2+px,cy-3,4,6,2);c.setMode(1);}};
  const smile=a=>{if(a<=0)return;const w=24*a,left=64-w/2;c.hline(left,51,w);if(a>.45){c.pixel(left-1,50);c.pixel(left+w,50);}};
  if(elapsed<150){eye(40,31,30,3);eye(88,31,30,3);c.hline(98,13,6);c.pixel(103,14);c.pixel(102,15);c.pixel(101,16);c.hline(98,17,6);c.hline(108,7,4);c.pixel(111,8);c.pixel(110,9);c.hline(108,10,4);}
  else if(elapsed<520){const o=ease(phase(150,370));eye(40,31,mix(30,34,o),mix(3,25,o),0,o>.68);eye(88,31,mix(30,34,o),mix(3,25,o),0,o>.68);}
  else if(elapsed<760){const l=phase(520,240);let g=l<.3?mix(0,-6,ease(l/.3)):l<.68?mix(-6,6,ease((l-.3)/.38)):mix(6,0,ease((l-.68)/.32));eye(40,31,34,25,g,true);eye(88,31,34,25,g,true);}
  else if(elapsed<980){const w=phase(760,220),close=1-Math.abs(2*w-1);eye(40,31,34,mix(25,3,ease(close)),0,close<.55);eye(88,31,34,25,0,true);smile(ease(1-close*.35));}
  else if(elapsed<1240){const m=ease(phase(980,260));eye(mix(40,63,m),mix(31,25,m),mix(34,3,m),mix(25,3,m));eye(mix(88,63,m),mix(31,33,m),mix(34,3,m),mix(25,3,m));smile(1-m);}
  else {const t=ease(phase(1240,260));sevenDigit(c,mix(-28,33,t),18,1);sevenDigit(c,mix(-14,47,t),18,0);sevenDigit(c,mix(128,67,t),18,2);sevenDigit(c,mix(142,81,t),18,8);c.box(62,24,2,2);c.box(62,32,2,2);const divider=128*t;c.dotted(64-divider/2,14,divider);c.dotted(64-divider/2,44,divider);}
  return c;
}
const stripIcons=[
  [0x00,0x66,0xff,0xff,0x7e,0x3c,0x18,0x00],
  [0x70,0x88,0x88,0x88,0x76,0x02,0x04,0x06],
  [0x18,0x24,0x24,0x24,0x24,0x5a,0x7e,0x3c],
  [0x10,0x18,0x3c,0x3c,0x7e,0x7e,0x3c,0x00]
];
function drawMsbIcon(c,x,y,rows){for(let row=0;row<8;row++)for(let column=0;column<8;column++)if(rows[row]&(0x80>>column))c.pixel(x+column,y+row);}
function stripValue(c,cell,value){const digits=String(value);const width=digits.length*6+(digits.length-1);let x=cell+10+(22-width)/2;for(const char of digits){smallDigit(c,x,49,Number(char));x+=7;}}
function watchFace(){const c=new Canvas();c.text(0,11,'2026/11/08');c.rframe(68,3,8,6,3);c.hline(67,9,10);c.pixel(71,2);c.pixel(72,2);c.pixel(71,10);c.text(84,11,'86%');c.frame(104,2,19,9);c.box(123,4,2,5);c.box(106,4,13,5);c.dotted(0,14,128);bigTime(c,10,28);c.dotted(0,44,128);[72,98,26,58].forEach((value,i)=>{const cell=i*32;drawMsbIcon(c,cell+1,50,stripIcons[i]);stripValue(c,cell,value);if(i<3){c.pixel(cell+31,50);c.pixel(cell+31,54);c.pixel(cell+31,58);}});return c;}

function crc32(buffer){let crc=0xffffffff;for(const byte of buffer){crc^=byte;for(let i=0;i<8;i++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}return (crc^0xffffffff)>>>0;}
function chunk(type,data){const typeBuffer=Buffer.from(type);const out=Buffer.alloc(12+data.length);out.writeUInt32BE(data.length,0);typeBuffer.copy(out,4);data.copy(out,8);out.writeUInt32BE(crc32(Buffer.concat([typeBuffer,data])),8+data.length);return out;}
function rgbFromCanvas(canvas,scale=4,border=0){const width=canvas.width*scale+border*2,height=canvas.height*scale+border*2;const rgb=Buffer.alloc(width*height*3);const off=[3,8,12],on=[234,251,255],edge=[35,78,90];for(let y=0;y<height;y++)for(let x=0;x<width;x++){let color=off;if(border&&((x<border||x>=width-border)||(y<border||y>=height-border)))color=edge;else if(x>=border&&y>=border&&canvas.data[Math.floor((y-border)/scale)*canvas.width+Math.floor((x-border)/scale)])color=on;const i=(y*width+x)*3;rgb[i]=color[0];rgb[i+1]=color[1];rgb[i+2]=color[2];}return{width,height,rgb};}
function writePng(file,width,height,rgb){const scan=Buffer.alloc((width*3+1)*height);for(let y=0;y<height;y++){scan[y*(width*3+1)]=0;rgb.copy(scan,y*(width*3+1)+1,y*width*3,(y+1)*width*3);}const ihdr=Buffer.alloc(13);ihdr.writeUInt32BE(width,0);ihdr.writeUInt32BE(height,4);ihdr[8]=8;ihdr[9]=2;fs.writeFileSync(file,Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',ihdr),chunk('IDAT',zlib.deflateSync(scan,{level:9})),chunk('IEND',Buffer.alloc(0))]));}
function saveCanvas(file,canvas,scale=4){const image=rgbFromCanvas(canvas,scale,2);writePng(file,image.width,image.height,image.rgb);}
function contactSheet(file,canvases,columns=3,scale=3){const cellW=128*scale+20,cellH=64*scale+20,rows=Math.ceil(canvases.length/columns),width=columns*cellW+20,height=rows*cellH+20,rgb=Buffer.alloc(width*height*3,5);for(let y=0;y<height;y++)for(let x=0;x<width;x++){const i=(y*width+x)*3;rgb[i]=3;rgb[i+1]=8;rgb[i+2]=12;}canvases.forEach((canvas,index)=>{const col=index%columns,row=Math.floor(index/columns),ox=20+col*cellW,oy=20+row*cellH;for(let y=0;y<64*scale+4;y++)for(let x=0;x<128*scale+4;x++){const border=x<2||y<2||x>=128*scale+2||y>=64*scale+2;const on=!border&&canvas.data[Math.floor((y-2)/scale)*128+Math.floor((x-2)/scale)];const color=border?[35,78,90]:on?[234,251,255]:[3,8,12],i=((oy+y)*width+ox+x)*3;rgb[i]=color[0];rgb[i+1]=color[1];rgb[i+2]=color[2];}});writePng(file,width,height,rgb);}
function gifCodes(pixels){const bytes=[];let current=0,bits=0;const push=code=>{current|=code<<bits;bits+=3;while(bits>=8){bytes.push(current&255);current>>>=8;bits-=8;}};for(const pixel of pixels){push(4);push(pixel?1:0);}push(5);if(bits)bytes.push(current&255);return Buffer.from(bytes);}
function writeGif(file,frames,scale=4,delays=[]){const width=128*scale,height=64*scale,parts=[Buffer.from('GIF89a')],header=Buffer.alloc(7);header.writeUInt16LE(width,0);header.writeUInt16LE(height,2);header[4]=0xf0;parts.push(header,Buffer.from([3,8,12,234,251,255]),Buffer.from([0x21,0xff,0x0b]),Buffer.from('NETSCAPE2.0'),Buffer.from([3,1,0,0,0]));frames.forEach((frame,index)=>{const pixels=Buffer.alloc(width*height);for(let y=0;y<height;y++)for(let x=0;x<width;x++)pixels[y*width+x]=frame.data[Math.floor(y/scale)*128+Math.floor(x/scale)];const delay=delays[index]||10;parts.push(Buffer.from([0x21,0xf9,4,0,delay&255,(delay>>8)&255,0,0,0x2c,0,0,0,0,width&255,(width>>8)&255,height&255,(height>>8)&255,0,2]));const data=gifCodes(pixels);for(let at=0;at<data.length;at+=255){const size=Math.min(255,data.length-at);parts.push(Buffer.from([size]),data.subarray(at,at+size));}parts.push(Buffer.from([0]));});parts.push(Buffer.from([0x3b]));fs.writeFileSync(file,Buffer.concat(parts));}

const menus=menuNames.map((name,index)=>menuScreen(name,index));
const pages=pageRenderers.map(render=>render());
menuNames.forEach((name,index)=>saveCanvas(path.join(iconOutput,`${String(index+1).padStart(2,'0')}-${name}.png`),menus[index]));
menuNames.forEach((name,index)=>saveCanvas(path.join(pageOutput,`${String(index+1).padStart(2,'0')}-${name}.png`),pages[index]));
contactSheet(path.join(output,'二级菜单图标总览.png'),menus);
contactSheet(path.join(output,'功能页面总览.png'),pages);
const bootTimes=[0,180,360,540,660,800,900,1030,1160,1260,1360,1460];
const bootFrames=bootTimes.map(bootFrame).concat([watchFace()]);
writeGif(path.join(output,'开机动画-主表盘.gif'),bootFrames,4,[12,10,10,10,10,10,10,10,10,10,10,12,90]);
saveCanvas(path.join(output,'主表盘.png'),watchFace());
console.log(`Rendered ${menus.length} menu thumbnails, ${pages.length} feature pages and ${bootFrames.length} GIF frames to ${output}`);
