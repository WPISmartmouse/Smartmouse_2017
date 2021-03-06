#pragma once

#include "Mouse.h"


class Solver {
public:
  enum class Goal {
    CENTER,
    START
  };

  Solver(Mouse *mouse);

  virtual void setup() = 0;

  virtual Direction planNextStep() = 0;

  virtual bool isFinished() = 0;

  virtual char *solve() = 0;

  virtual void teardown() = 0;

  virtual void setGoal(Goal goal) = 0;

  bool isSolvable();

  bool solvable;
  Mouse *mouse;
};
