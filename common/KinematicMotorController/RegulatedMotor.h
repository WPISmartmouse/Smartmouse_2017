#pragma once

class RegulatedMotor {
public:
  RegulatedMotor(unsigned long period_ms);

  double run_pid(unsigned long time_ms, double angle_rad);

  void set_setpoint(double setpoint_rps);

  static const double kP;
  static const double kI;
  static const double kD;
  static const double kFF;
  static const double INTEGRAL_CAP;

  double abstract_force;
  double derivative;
  double error;
  double integral;
  double last_angle_rad;
  double last_error;
  double setpoint_rps;
  double smoothed_velocity_rps;
  double velocity_rps;
  unsigned long period_ms;
  unsigned long last_update_time_ms;

};