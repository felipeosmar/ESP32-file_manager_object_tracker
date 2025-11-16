/**
 * Motion Detection and Object Tracking
 *
 * Uses frame differencing to detect motion and calculate object center
 */

#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

#include <Arduino.h>

struct Point {
  int x;
  int y;

  Point() : x(-1), y(-1) {}
  Point(int _x, int _y) : x(_x), y(_y) {}
};

class MotionDetector {
private:
  uint8_t *previousFrame;
  size_t frameSize;
  int threshold;
  bool initialized;

  // Downsampling for performance
  static const int SAMPLE_WIDTH = 80;
  static const int SAMPLE_HEIGHT = 60;
  uint8_t sampledCurrent[SAMPLE_WIDTH * SAMPLE_HEIGHT];
  uint8_t sampledPrevious[SAMPLE_WIDTH * SAMPLE_HEIGHT];

public:
  MotionDetector();
  ~MotionDetector();

  void begin(int motionThreshold = 30);
  Point detectMotion(uint8_t *currentFrame, int width, int height);
  void setThreshold(int newThreshold);
  int getThreshold() const { return threshold; }

private:
  void downsampleFrame(uint8_t *frame, int width, int height, uint8_t *output);
  Point calculateCentroid(uint8_t *diffFrame);
  uint8_t getGrayscaleFromJPEG(uint8_t *jpegData, int x, int y, int width, int height);
};

#endif // MOTION_DETECTOR_H
