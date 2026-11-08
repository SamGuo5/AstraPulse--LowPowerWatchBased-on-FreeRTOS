//
// Created by Fir on 2024/2/2.
//

#include "launcher.h"

namespace astra {

void Launcher::popInfo(std::string _info, uint16_t _time) {
  static const uint64_t beginTime = this->time;
  static bool onRender = true;

  while (onRender) {
    time++;

    static float wPop = HAL::getFontWidth(_info) + 2 * getUIConfig().popMargin;  //宽度
    static float hPop = HAL::getFontHeight() + 2 * getUIConfig().popMargin;  //高度

    static float yPop = 0 - hPop - 8; //从屏幕上方滑入
    static float yPopTrg = 0;
    if (time - beginTime < _time) yPopTrg = (HAL::getSystemConfig().screenHeight - hPop) / 3;  //目标位置 中间偏上
    else yPopTrg = 0 - hPop - 8;  //滑出

    static float xPop = (HAL::getSystemConfig().screenWeight - wPop) / 2;  //居中

    HAL::canvasClear();
    /*渲染一帧*/
    currentPage->render(camera.getPosition());
    selector.render(camera.getPosition());
    camera.update(currentPage, &selector);
    /*渲染一帧*/

    HAL::setDrawType(0);
    HAL::drawRBox(xPop - 4, yPop - 4, wPop + 8, hPop + 8, getUIConfig().popRadius + 2);
    HAL::setDrawType(1);  //反色显示
    HAL::drawRFrame(xPop - 1, yPop - 1, wPop + 2, hPop + 2, getUIConfig().popRadius);  //绘制一个圆角矩形
    HAL::drawEnglish(xPop + getUIConfig().popMargin, yPop + getUIConfig().popMargin + HAL::getFontHeight(), _info);  //绘制文字

    HAL::canvasUpdate();

    animation(&yPop, yPopTrg, getUIConfig().popSpeed);  //动画

    //todo 这里条件可以加上一个如果按键按下 就退出
    if (time - beginTime >= _time && yPop == 0 - hPop - 8) onRender = false;  //退出条件
  }
}

void Launcher::init(Menu *_rootPage) {
  currentPage = _rootPage;
  camera = Camera(0, 0);
  _rootPage->init(camera.getPosition());
  selector.inject(_rootPage);
  selector.go(_rootPage->selectIndex);
}

/**
 * @brief 打开选中的页面
 *
 * @return 是否成功打开
 * @warning 仅可调用一次
 */
bool Launcher::open() {
  //todo 打开和关闭都还没写完 应该还漏掉了一部分内容

  //如果当前页面指向的当前item没有后继 那就返回false
  if (currentPage == nullptr || currentPage->getItemNum() == 0) return false;
  if (currentPage->getNext() == nullptr || currentPage->getNext()->getItemNum() == 0) return false;

  currentPage->deInit();  //先析构（退场动画）再挪动指针

  currentPage = currentPage->getNext();
  camera.reset();
  currentPage->init(camera.getPosition());

  selector.inject(currentPage);

  return true;
}

/**
 * @brief 关闭选中的页面
 *
 * @return 是否成功关闭
 * @warning 仅可调用一次
 */
bool Launcher::close() {
  if (currentPage == nullptr || currentPage->getPreview() == nullptr) return false;

  currentPage->deInit();  //先析构（退场动画）再挪动指针

  currentPage = currentPage->getPreview();
  camera.reset();
  currentPage->init(camera.getPosition());

  selector.inject(currentPage);

  return true;
}

void Launcher::update() {
  HAL::canvasClear();

  currentPage->render(camera.getPosition());
  selector.render(camera.getPosition());
  camera.update(currentPage, &selector);

  HAL::canvasUpdate();

  time++;
}

void Launcher::previous() {
  if (currentPage == nullptr || currentPage->getItemNum() == 0) return;
  const uint8_t count = currentPage->getItemNum();
  currentPage->selectIndex = currentPage->selectIndex == 0 ? count - 1 : currentPage->selectIndex - 1;
  selector.go(currentPage->selectIndex);
}

void Launcher::next() {
  if (currentPage == nullptr || currentPage->getItemNum() == 0) return;
  currentPage->selectIndex = (currentPage->selectIndex + 1) % currentPage->getItemNum();
  selector.go(currentPage->selectIndex);
}

Menu* Launcher::selected() {
  if (currentPage == nullptr || currentPage->getItemNum() == 0) return nullptr;
  return currentPage->getNext();
}
}
