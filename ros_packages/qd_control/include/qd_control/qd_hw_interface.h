#include <hardware_interface/joint_state_interface.h>
#include <hardware_interface/joint_command_interface.h>
#include <hardware_interface/robot_hw.h>
#include <joint_limits_interface/joint_limits.h>
#include <joint_limits_interface/joint_limits_interface.h>
#include <joint_limits_interface/joint_limits_rosparam.h>
#include <controller_manager/controller_manager.h>
#include <boost/scoped_ptr.hpp>
#include <ros/ros.h>
#include <std_msgs/String.h>
#include <std_msgs/Float64MultiArray.h>
#include <string.h>

#define NB_JOINTS 12
#define LOOP_REFRESH_RATE 10

class Quadrus : public hardware_interface::RobotHW 
{
    public:
        Quadrus(ros::NodeHandle& nh);                                                                   //Constructor
        ~Quadrus();                                                                                     //Destructor
        void init();                                                                                    //define all joint handle, joint's interfaces and joint limits interfaces
        void update(const ros::TimerEvent& e);                                                          //control loop()
        void read();                                                                                    //reading joint sensor data
        void write(ros::Duration elapsed_time);                                                         //sending command to motors
        
    protected:
        //Declare the type of joint interfaces and joint limit interfaces your robot actuators/motors are using.
        
        hardware_interface::JointStateInterface joint_state_interface_;
        hardware_interface::PositionJointInterface position_joint_interface_;
        joint_limits_interface::PositionJointSaturationInterface position_joint_sat_interface;

        joint_limits_interface::JointLimits *jlimits[NB_JOINTS];

        hardware_interface::JointStateHandle *jsHandle[NB_JOINTS];
        hardware_interface::JointHandle *jpHandle[NB_JOINTS];
        joint_limits_interface::PositionJointSaturationHandle *jlHandle[NB_JOINTS];

        double pos[NB_JOINTS];
        double vel[NB_JOINTS];
        double eff[NB_JOINTS];
        double cmd[NB_JOINTS];

        ros::Publisher position_pub;
        std_msgs::Float64MultiArray pos_array;
        
        ros::NodeHandle nh_;
        ros::Timer qd_control_loop_;
        ros::Duration elapsed_time_;
        double loop_hz_;
        boost::shared_ptr<controller_manager::ControllerManager> controller_manager_;
};
