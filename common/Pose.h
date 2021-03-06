#pragma once

class LocalPose {
public:
  LocalPose();

  LocalPose(double to_left, double to_back);

  LocalPose(double to_left, double to_back, double yaw_from_straight);

  double to_left, to_back;
  double yaw_from_straight;
};

class GlobalPose {
public:
  GlobalPose();

  GlobalPose(double x, double y);

  GlobalPose(double x, double y, double yaw);

  double x, y;
  double yaw;
};
