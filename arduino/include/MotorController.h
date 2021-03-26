#ifndef MotorController_h
#define MotorController_h

#include <Adafruit_PWMServoDriver.h>
#include <ros.h>
#include <std_msgs/String.h>
#include <std_msgs/MultiArrayLayout.h>
#include <std_msgs/MultiArrayDimension.h>
#include <std_msgs/Float64MultiArray.h>

#define PI 3.1415926535897932384626433832795
#define PULSEMIN 555
#define PULSEMAX 2395
#define SERVO_FREQ 60

//Global variables
float initPositions[] = {0,90,0,
                        0,90,0,
                        0,90,0,
                        0,90,0};

float compensationArrayMec[12] = { 0.0,  -1.0,  -7.0,
                                   7.5,   7.0,  -7.0,
                                  -5.0,   0.0,   0.0,
                                  -5.0,  -4.0,  -8.0};
                               

float compensationArrayROS[12] = { 135.0,  225.0,  135.0,
                                   135.0,   45.0,  135.0,
                                   135.0,  225.0,  135.0,
                                   135.0,   45.0,  135.0}; 

int jointLimit[12][2] = { {-30, 45}, {-110, 125}, {-75, 135},
                          {-45, 30}, {-125, 110}, {-135, 75},
                          {-30, 45}, {-110, 125}, {-75, 135},
                          {-45, 30}, {-125, 110}, {-135, 75} };


//Function prototypes
Adafruit_PWMServoDriver driver = Adafruit_PWMServoDriver();
void computeLimits();
int degToPulse(float ang);
float radToDeg(float angleRad);
void subscriberCallback(const std_msgs::Float64MultiArray& motor_msg);
void moveMotor(float cmdAngle[12]);
float compensateCommand(float rawAngle, int index);
void motorController(int pulseCommand[12]);

//ROS config
ros::NodeHandle node_handle;
std_msgs::Float64MultiArray motor_msg;

/**
 * Compute the joint limits with the base angle of 135 degree
 * and the mechanical compensations
 */
void computeLimits() {
  for (int i = 0; i < 12; i++)
  {
    for (int j = 0; j < 2; j++)
    {
      jointLimit[i][j] =  degToPulse(135 + compensationArrayMec[i] + jointLimit[i][j]);
    }
    
  }
  
}

/**
 * Map an angle in degrees to the corresponding pulse
 *
 * @param ang Angle in degrees
 * @return Corresponding pulse
 */
int degToPulse(float ang) {
  int pulse = map(ang, 0, 270, PULSEMIN, PULSEMAX);
  return pulse;
}

/**
 * Convert an angle in rad to degrees
 *
 * @param angleRad Angle in rad
 * @return angle in degrees
 */
float radToDeg(float angleRad) {
  return angleRad*180.0/PI;
}

/**
 * Receive angle commands from ROS and convert them to degrees
 *
 * @param motor_msg ROS controller message specifying joint angle in radian
 */
void subscriberCallback(const std_msgs::Float64MultiArray& motor_msg) {

  float ros_motor_commands[12];
  
  for (int i = 0; i < 12; i++)
  {
    ros_motor_commands[i] = radToDeg(motor_msg.data[i]);
  }
  
  moveMotor(ros_motor_commands);
}

/**
 * Receive angle commands, add the compensations,
 * convert them to pulse and move the motors
 *
 * @param cmdAngle Array of commands in degrees for the 12 motors
 */
void moveMotor(float cmdAngle[12]) {
  float compensatedCmd[12];
  int pulseCmd[12];

  for (int i = 0; i < 12; i++) {
    compensatedCmd[i] = compensateCommand(cmdAngle[i], i);
    pulseCmd[i] = degToPulse(compensatedCmd[i]);
  }
  motorController(pulseCmd);
}

/**
 * Add the mechanical and the ROS compensations to the angle command passed
 *
 * @param rawAngle Angle in degrees before adding compensations
 * @param index Index of the motor corresponding to the rawAngle passed
 * @return The angle command with the compensations
 */
float compensateCommand(float rawAngle, int index) {
  return rawAngle + compensationArrayMec[index] + compensationArrayROS[index];
}

/**
 * Check if the pulse commands are within the joint limits then write the commands
 * to the driver to move the motors one after the other
 *
 * @param pulseCommand Array of commands in pulse for the 12 motors
 */
void motorController(int pulseCommand[12]) {

    for (int i = 0; i < 12; i++){
        if(pulseCommand[i] > jointLimit[i][1]){
            pulseCommand[i] = jointLimit[i][1];
          }
        else if(pulseCommand[i] < jointLimit[i][0]){
            pulseCommand[i] = jointLimit[i][0];
          }
      }
    
    for (int i=0; i < 12; i++){
        driver.writeMicroseconds(i, pulseCommand[i]);
    }
}

#endif
