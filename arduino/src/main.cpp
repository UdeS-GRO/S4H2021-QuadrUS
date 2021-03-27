#include "qd_hw_control.h"
#include "qd_hw_config.h"

Adafruit_PWMServoDriver driver = Adafruit_PWMServoDriver();
ros::NodeHandle nh;
std_msgs::Float64MultiArray cmd_msg;
std_msgs::Float64MultiArray feedback_msg;
ros::Publisher feedback_pub(feedback_topic, &feedback_msg);
ros::Subscriber<std_msgs::Float64MultiArray> cmd_sub(cmd_topic, &subscriberCallback);

void setup() {
  servoInit();
  computeLimits();
  moveMotor(initPositions);
  rosInit();
}

void loop() {
  readAngles(feedback_msg);
  feedback_pub.publish(&feedback_msg);
  nh.spinOnce();
  delay(1.00/loop_hz);
}