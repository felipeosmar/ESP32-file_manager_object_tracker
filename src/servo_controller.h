/**
 * Servo Controller with PID Control
 *
 * Controls Pan and Tilt servos to track objects
 */

#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class ServoController {
private:
  Servo panServo;
  Servo tiltServo;

  int panPin;
  int tiltPin;

  float panAngle;
  float tiltAngle;

  // Servo limits (degrees)
  static constexpr float PAN_MIN = 0;
  static constexpr float PAN_MAX = 180;
  static constexpr float TILT_MIN = 0;
  static constexpr float TILT_MAX = 180;

  static constexpr float PAN_CENTER = 90;
  static constexpr float TILT_CENTER = 90;

  // PID coefficients
  float kp;  // Proportional gain
  float ki;  // Integral gain
  float kd;  // Derivative gain

  // PID state
  float panErrorIntegral;
  float tiltErrorIntegral;
  float panErrorPrevious;
  float tiltErrorPrevious;

  // Smoothing
  static constexpr float MAX_STEP = 5.0;  // Max degrees per update

public:
  ServoController();

  void begin(int panGPIO, int tiltGPIO);
  void updateTracking(int errorX, int errorY, int speed);
  void setPan(int angle);
  void setTilt(int angle);
  void setCenter();

  float getPanAngle() const { return panAngle; }
  float getTiltAngle() const { return tiltAngle; }

  void setPIDGains(float p, float i, float d);

private:
  float constrainAngle(float angle, float minAngle, float maxAngle);
  void updateServo(Servo &servo, float &currentAngle, float targetAngle, float minAngle, float maxAngle);
};

#endif // SERVO_CONTROLLER_H
