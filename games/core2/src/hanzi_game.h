#pragma once

enum class HanziGameAction {
  None,
  Home,
};

namespace HanziGame {

void start();
HanziGameAction update();

}  // namespace HanziGame
