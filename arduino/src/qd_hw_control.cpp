#include "qd_hw_control.h"

/**
* Initialise node handle of ROS
*/
void rosInit(){
  nh.getHardware()->setBaud(115200);
  nh.initNode();
  nh.subscribe(cmd_sub);
  nh.advertise(feedback_pub);
  //nh.advertise(imu_pub);
  feedback_msg.data_length = 12;
  feedback_msg.data = (float *)malloc(sizeof(float) *12);
}


/**
 * Initialise the driver object to control the servos
 * from the PCA9685 board
 */
void servoInit(){
  driver.begin();
  driver.setOscillatorFrequency(27000000);
  driver.setPWMFreq(servo_freq);
}


/**
 * Compute the joint limits in pulses with the base angle of 135 degree
 * and the mechanical compensations
 */
void computeLimits() {
  for (int i = 0; i < 12; i++){
    for (int j = 0; j < 2; j++){
      if (i == 3)
      {
        jointLimitPulse[i][j] =  degToPulse(90 + compensationArrayMec[i] + jointLimit[i][j], i);
      }
      else
      {
        jointLimitPulse[i][j] =  degToPulse(135 + compensationArrayMec[i] + jointLimit[i][j], i);
      }
    }
  }
}


/**
 * Map an angle in degrees to the corresponding pulse
 *
 * @param ang Angle in degrees
 * @param motorIndex Index of the motor to map the pulse
 * @return Corresponding pulse
 */
int degToPulse(float ang, int motorIndex) {
  if (motorIndex == 3)
  {
    return ang*(pulsemax[motorIndex]-pulsemin)/180.0 + pulsemin;
  } else {
    return ang*(pulsemax[motorIndex]-pulsemin)/270.0 + pulsemin;
  }
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
 * Convert an angle in degrees to radians
 *
 * @param angleDeg Angle in deg
 * @return angle in rad
 */
float degToRad(float angleDeg) {
  return angleDeg*PI/180.0;
}

/**
 * Convert the analog output of a servo potentiometer to an angle in degrees
 *
 * @param analog_value Analog reading from Arduino
 * @return angle in degrees
 */
float analogToDeg(int analog_value, int motorIndex){
  if (motorIndex == 3)
  {
    return (analog_value - 153.0)*(180.0/(558.0-153.0));
  }
  else
  {
    return (analog_value - 67.0)*(270.0/(646.0-67.0));
  }
}


/**
 * Read the analog inputs on the Arduino and store them
 * in angle in the feedback_data to send back to ROS
 *
 * @param feedback_data Current angle of the servos to feedback to ROS
 */
void readAngles(std_msgs::Float64MultiArray * feedback_data){  
  float feedbackAngle[12];
  float compensatedAngle[12];
  for(int i=0;i<12;i++){
     feedbackAngle[i] = analogToDeg(analogRead(analog_pins[i]), i);
     compensatedAngle[i] = compensateFeedback(feedbackAngle[i], i);
     feedback_data->data[i] = degToRad(compensatedAngle[i]);
  }
}


/**
 * Receive angle commands from ROS and convert them to degrees
 *
 * @param cmd_msg ROS controller message specifying joint angle in radians
 */
void subscriberCallback(const std_msgs::Float64MultiArray& cmd_msg) {

  float ros_motor_commands[12];

  for (int i = 0; i < 12; i++){
    ros_motor_commands[i] = radToDeg(cmd_msg.data[i]);

    if(i==4 || i==5 || i==10 || i==11){
      ros_motor_commands[i] = -ros_motor_commands[i];
    }
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
    pulseCmd[i] = degToPulse(compensatedCmd[i], i);
  }
  motorController(pulseCmd);
}


/**
 * Add the mechanical and the ROS compensations to the angle command passed
 *
 * @param rawCommand Angle in degrees before adding compensations
 * @param index Index of the motor corresponding to the rawCommand passed
 * @return The angle command with the compensations
 */
float compensateCommand(float rawCommand, int index) {
  return rawCommand + compensationArrayMec[index] + compensationArrayROS[index];
}


/**
 * Add the mechanical and the ROS compensations to the angle command passed
 *
 * @param rawAngle Angle in degrees before substracting compensations
 * @param index Index of the motor corresponding to the rawAngle passed
 * @return The angle feedback without the compensations
 */
float compensateFeedback(float rawAngle, int index) {
  return rawAngle - compensationArrayMec[index] - compensationArrayROS[index];
}


/**
 * Check if the pulse commands are within the joint limits then write the commands
 * to the driver to move the motors one after the other
 *
 * @param pulseCommand Array of commands in pulse for the 12 motors
 */
void motorController(int pulseCommand[12]) {

    for (int i = 0; i < 12; i++){
        if(pulseCommand[i] > jointLimitPulse[i][1]){
            pulseCommand[i] = jointLimitPulse[i][1];
          }
        else if(pulseCommand[i] < jointLimitPulse[i][0]){
            pulseCommand[i] = jointLimitPulse[i][0];
          }
      }
    
    for (int i=0; i < 12; i++){
        driver.writeMicroseconds(i, pulseCommand[i]);
    }
}

void motorControllerBadOne(int pulseCommand[12]) {
    bool stop = false;
    int lastStep = 0;
    int pulseInterval = 5;

    for (int i = 0; i < 12; i++){
        if(pulseCommand[i] > jointLimitPulse[i][1]){
            pulseCommand[i] = jointLimitPulse[i][1];
          }
        else if(pulseCommand[i] < jointLimitPulse[i][0]){
            pulseCommand[i] = jointLimitPulse[i][0];
          }
      }
    
    while(stop == false) {
        stop = true;

        for (int i = 0; i < 12 ; i++){
          if (pulseCommand[i] > currentPulse[i]){
              lastStep = pulseCommand[i] - currentPulse[i];
              
              if ( lastStep < pulseInterval) {
                currentPulse[i] += lastStep;
              } 
              else {
                currentPulse[i] += pulseInterval;
              }
                
              driver.writeMicroseconds(i,currentPulse[i]);
            }
          else if (pulseCommand[i] < currentPulse[i]){
              lastStep = currentPulse[i] - pulseCommand[i];
              
              if ( lastStep < pulseInterval) {
                currentPulse[i] -= lastStep;
              } 
              else {
                currentPulse[i] -= pulseInterval;
              }
              
              driver.writeMicroseconds(i,currentPulse[i]);
            }
          else {
              delayMicroseconds(1000);
            }

          if (currentPulse[i] != pulseCommand[i]) {
              stop = false;
            }
        }
      } 
  }


/**
 * Read the analog inputs on the Arduino and store them
 * in acceleration(X,Y,Z); gyro(X,Y,Z); angles(X,Y,Z) in the imu_data to send back to ROS
 *
 * @param imu_data Current angular positionning of the robot to feedback to ROS
 */
/*void readIMU(IMUdata imu_data){

  double accX = 0;
  double accY = 0;
  double accZ = 0;
  double gyroX = 0;
  double gyroY = 0;
  double gyroZ = 0;
  double roll = 0;
  double pitch = 0;

  imu.Acc_getValues(&accX, &accY, &accZ);
  imu.Gyro_getValues(&gyroX, &gyroY, &gyroZ);
  imu.getAngles(&roll, &pitch, 2); 

  imu_data.roll = roll;
  imu_data.pitch = pitch;
  imu_data.acc_x = accX;
  imu_data.acc_y = accY;
  imu_data.acc_z = accZ;
  imu_data.gyro_x = gyroX;
  imu_data.gyro_y = gyroY;
  imu_data.gyro_z = gyroZ;
}*/