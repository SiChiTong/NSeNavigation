#ifndef _BASE_LOCAL_PLANNER_
#define _BASE_LOCAL_PLANNER_



#include "Algorithm/WorldModel.h"
#include "Algorithm/CostmapModel.h"
#include "Algorithm/TrajectoryPlanner.h"
#include "Algorithm/OdometryHelper.h"

#include <transform/transform2d.h>
#include <boost/thread.hpp>
#include <string>
#include "../../base/LocalPlannerBase.h"



namespace NS_Planner
{
  /**
   * @class TrajectoryLocalPlanner
   * @brief A ROS wrapper for the trajectory controller that queries the param server to construct a controller
   */
  class TrajectoryLocalPlanner: public LocalPlannerBase
  {
  public:
    /**
     * @brief  Default constructor for the ros wrapper
     */
    TrajectoryLocalPlanner();

    /**
     * @brief  Constructs the ros wrapper
     * @param name The name to give this instance of the trajectory planner
     * @param tf A pointer to a transform listener
     * @param costmap The cost map to use for assigning costs to trajectories
     */
    virtual void
    onInitialize();

    /**
     * @brief  Destructor for the wrapper
     */
    ~TrajectoryLocalPlanner();

    /**
     * @brief  Given the current position, orientation, and velocity of the robot,
     * compute velocity commands to send to the base
     * @param cmd_vel Will be filled with the velocity command to be passed to the robot base
     * @return True if a valid trajectory was found, false otherwise
     */
    virtual bool
    computeVelocityCommands(Velocity2D& cmd_vel);

    /**
     * @brief  Set the plan that the controller is following
     * @param orig_global_plan The plan to pass to the controller
     * @return True if the plan was updated successfully, false otherwise
     */
    virtual bool
    setPlan(const std::vector< Pose2D >& orig_global_plan);

    /**
     * @brief  Check if the goal pose has been achieved
     * @return True if achieved, false otherwise
     */
    virtual bool
    isGoalReached();

    /**
     * @brief  Generate and score a single trajectory
     * @param vx_samp The x velocity used to seed the trajectory
     * @param vy_samp The y velocity used to seed the trajectory
     * @param vtheta_samp The theta velocity used to seed the trajectory
     * @param update_map Whether or not to update the map for the planner
     * when computing the legality of the trajectory, this is useful to set
     * to false if you're going to be doing a lot of trajectory checking over
     * a short period of time
     * @return True if the trajectory is legal, false otherwise
     */
    bool
    checkTrajectory(double vx_samp, double vy_samp, double vtheta_samp,
                    bool update_map = true);

    /**
     * @brief  Generate and score a single trajectory
     * @param vx_samp The x velocity used to seed the trajectory
     * @param vy_samp The y velocity used to seed the trajectory
     * @param vtheta_samp The theta velocity used to seed the trajectory
     * @param update_map Whether or not to update the map for the planner
     * when computing the legality of the trajectory, this is useful to set
     * to false if you're going to be doing a lot of trajectory checking over
     * a short period of time
     * @return score of trajectory (double)
     */
    double
    scoreTrajectory(double vx_samp, double vy_samp, double vtheta_samp,
                    bool update_map = true);

    bool isInitialized()
    {
      return initialized_;
    }

    /** @brief Return the inner TrajectoryPlanner object.  Only valid after initialize(). */
    TrajectoryPlanner*
    getPlanner() const
    {
      return tc_;
    }

  private:



    /**
     * @brief Once a goal position is reached... rotate to the goal orientation
     * @param  global_pose The pose of the robot in the global frame
     * @param  robot_vel The velocity of the robot
     * @param  goal_th The desired th value for the goal
     * @param  cmd_vel The velocity commands to be filled
     * @return  True if a valid trajectory was found, false otherwise
     */
    bool
    rotateToGoal(const Pose2D& global_pose,
                 const Velocity2D& robot_vel,
                 double goal_th, Velocity2D& cmd_vel);

    /**
     * @brief Stop the robot taking into account acceleration limits
     * @param  global_pose The pose of the robot in the global frame
     * @param  robot_vel The velocity of the robot
     * @param  cmd_vel The velocity commands to be filled
     * @return  True if a valid trajectory was found, false otherwise
     */
    bool
    stopWithAccLimits(
        const Pose2D& global_pose,
        const Velocity2D& robot_vel,
        Velocity2D& cmd_vel);

    double sign(double x)
    {
      return x < 0.0 ? -1.0 : 1.0;
    }

    WorldModel* world_model_; ///< @brief The world model that the controller will use
    TrajectoryPlanner* tc_; ///< @brief The trajectory controller

    OdometryHelper* odom_helper_;

    NS_CostMap::Costmap2D* costmap_; ///< @brief The costmap the controller will use

    double max_sensor_range_; ///< @brief Keep track of the effective maximum range of our sensors
    Odometry base_odom_; ///< @brief Used to get the velocity of the robot

    double rot_stopped_velocity_, trans_stopped_velocity_;
    double xy_goal_tolerance_, yaw_goal_tolerance_, min_in_place_vel_th_;
    std::vector< Pose2D > global_plan_;
    bool prune_plan_;
    boost::recursive_mutex odom_lock_;

    double max_vel_th_, min_vel_th_;
    double acc_lim_x_, acc_lim_y_, acc_lim_theta_;
    double sim_period_;
    bool rotating_to_goal_;
    bool reached_goal_;
    bool latch_xy_goal_tolerance_, xy_tolerance_latch_;

    bool setup_;

    bool initialized_;

    int request_times;
    std::vector< Point2D > footprint_spec_;

  };
}
;
#endif
