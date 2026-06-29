#pragma once

enum class DefenseGameAction {
  None,
  Home,
};

namespace DefenseGame {

void start();
DefenseGameAction update();

}  // namespace DefenseGame
