/**
 * Servo Controller Implementation
 */

#include "servo_controller.h"

ServoController::ServoController()
  : panAngle(PAN_CENTER), tiltAngle(TILT_CENTER),
    kp(0.5), ki(0.0), kd(0.1),
    panErrorIntegral(0), tiltErrorIntegral(0),
    panErrorPrevious(0), tiltErrorPrevious(0) {
}

void ServoController::begin(int panGPIO, int tiltGPIO) {
  panPin = panGPIO;
  tiltPin = tiltGPIO;

  // Allow allocation of all timers for servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  panServo.setPeriodHertz(50);    // Standard 50Hz servo
  tiltServo.setPeriodHertz(50);

  panServo.attach(panPin, 500, 2400);   // Min/max pulse width in microseconds
  tiltServo.attach(tiltPin, 500, 2400);

  setCenter();

  Serial.printf("Servos initialized - Pan: GPIO%d, Tilt: GPIO%d\n", panPin, tiltPin);
}

void ServoController::updateTracking(int errorX, int errorY, int speed) {
  // Convert pixel error to angle adjustment using PID
  // Positive errorX means object is to the right, pan left (decrease angle)
  // Positive errorY means object is below center, tilt down (decrease angle)

  // Calculate PID for pan
  float panError = -errorX / 10.0; // Scale pixel error to reasonable angle
  panErrorIntegral += panError;
  panErrorIntegral = constrain(panErrorIntegral, -100, 100); // Prevent windup
  float panDerivative = panError - panErrorPrevious;
  float panAdjustment = (kp * panError) + (ki * panErrorIntegral) + (kd * panDerivative);
  panErrorPrevious = panError;

  // Calculate PID for tilt
  float tiltError = -errorY / 10.0;
  tiltErrorIntegral += tiltError;
  tiltErrorIntegral = constrain(tiltErrorIntegral, -100, 100);
  float tiltDerivative = tiltError - tiltErrorPrevious;
  float tiltAdjustment = (kp * tiltError) + (ki * tiltErrorIntegral) + (kd * tiltDerivative);
  tiltErrorPrevious = tiltError;

  // Apply speed factor
  panAdjustment *= (speed / 10.0);
  tiltAdjustment *= (speed / 10.0);

  // Limit maximum adjustment per step for smooth movement
  panAdjustment = constrain(panAdjustment, -MAX_STEP, MAX_STEP);
  tiltAdjustment = constrain(tiltAdjustment, -MAX_STEP, MAX_STEP);

  // Update angles
  float newPan = panAngle + panAdjustment;
  float newTilt = tiltAngle + tiltAdjustment;

  updateServo(panServo, panAngle, newPan, PAN_MIN, PAN_MAX);
  updateServo(tiltServo, tiltAngle, newTilt, TILT_MIN, TILT_MAX);
}

void ServoController::setPan(int angle) {
  updateServo(panServo, panAngle, angle, PAN_MIN, PAN_MAX);
}

void ServoController::setTilt(int angle) {
  updateServo(tiltServo, tiltAngle, angle, TILT_MIN, TILT_MAX);
}

void ServoController::setCenter() {
  setPan(PAN_CENTER);
  setTilt(TILT_CENTER);

  // Reset PID state
  panErrorIntegral = 0;
  tiltErrorIntegral = 0;
  panErrorPrevious = 0;
  tiltErrorPrevious = 0;

  Serial.println("Servos centered");
}

void ServoController::setPIDGains(float p, float i, float d) {
  kp = p;
  ki = i;
  kd = d;
  Serial.printf("PID gains updated - P: %.2f, I: %.2f, D: %.2f\n", kp, ki, kd);
}

float ServoController::constrainAngle(float angle, float minAngle, float maxAngle) {
  return constrain(angle, minAngle, maxAngle);
}

void ServoController::updateServo(Servo &servo, float &currentAngle, float targetAngle, float minAngle, float maxAngle) {
  float newAngle = constrainAngle(targetAngle, minAngle, maxAngle);

  if (abs(newAngle - currentAngle) > 0.5) { // Dead zone to prevent jitter
    currentAngle = newAngle;
    servo.write(currentAngle);
  }
}
