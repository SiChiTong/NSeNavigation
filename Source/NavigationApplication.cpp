/*
 * NavigationApplication.cpp
 *
 *  Created on: Jan 10, 2018
 *      Author: pengjiawei
 */

#include "NavigationApplication.h"
#include <Parameter/Parameter.h>
#include "planner/implements/GlobalPlanner/GlobalPlanner.h"
#include "planner/implements/TrajectoryLocalPlanner/TrajectoryLocalPlanner.h"
#include <Service/ServiceType/ServiceMap.h>
namespace NS_Navigation {

NavigationApplication::NavigationApplication() :
		new_global_plan_(false), runPlanner_(false) {
	// TODO Auto-generated constructor stub
	   goal_sub = new NS_DataSet::Subscriber< sgbot::Pose2D >(
	        "GOAL", boost::bind(&NavigationApplication::goalFromAPP, this, _1));
}

NavigationApplication::~NavigationApplication() {
	// TODO Auto-generated destructor stub
	delete goal_sub;
}
void NavigationApplication::loadParameters() {
	NS_NaviCommon::Parameter parameter;

	parameter.loadConfigurationFile("navigation.xml");
	global_planner_type_ = parameter.getParameter("global_planner_type",
			"global_planner");
	local_planner_type_ = parameter.getParameter("local_planner_type",
			"trajectory_local_planner");
	planner_frequency_ = parameter.getParameter("planner_frequency", 0.0f);
	controller_frequency_ = parameter.getParameter("controller_frequency",
			5.0f);
}
void NavigationApplication::runRecovery() {
	logInfo<<"run Recovery()";
}

void NavigationApplication::resetState() {
	state = PLANNING;
	publishZeroVelocity();
}
void NavigationApplication::publishZeroVelocity() {
	logInfo<<"--------------->publishZeroVelocity------------------------>";
	publishVelocity(0, 0, 0);
}
void NavigationApplication::publishVelocity(double linear_x, double linear_y,
		double angular_z) {
	Velocity2D vel;
	vel.linear = linear_x;
//	vel.linear.y = linear_y;
	vel.angular = angular_z;
	twist_pub->publish(vel);
}
bool NavigationApplication::goalFromAPP(sgbot::Pose2D& goal_from_app){
	planner_mutex.lock();
	    goal = goalToGlobalFrame(goal_from_app);
		goal = goal_from_app;
	    printf("goal_callback x = %.4f,y = %.4f, theta = %.4f\n", goal.x,
	           goal.y, goal.theta);
	    new_goal_trigger = true;
	    state = PLANNING;
	    planner_cond.notify_one();
	    planner_mutex.unlock();

	    controlLoop();
	    return true;
}
void NavigationApplication::controlLoop() {
	while (running) {
		NS_NaviCommon::Rate rate(controller_frequency_);
		//TODO maybe controller_frequency should be used
//		controller_mutex.lock();
//		while ((state != CONTROLLING || global_planner_plan->size() == 0)
//				&& running) {
//			controller_cond.timed_wait(controller_mutex,
//					(boost::get_system_time() + boost::posix_time::milliseconds(
//					PLANNER_LOOP_TIMEOUT)));
//		}
//		controller_mutex.unlock();

		if (!running) {
			console.message("Quit local planning loop...");
			break;
		}
		if (new_global_plan_) {
			if (!local_planner->setPlan(*global_planner_plan)) {
				console.error("Set plan to local planner failure!");
				resetState();
				logInfo<< "local planner failed tp set plan,so disable plan thread ";
				planner_mutex.lock();
				runPlanner_ = false;
				planner_mutex.unlock();
				continue;
			}
			new_global_plan_ = false;
		} else {
			logInfo<< "no new global plan do not set plan";
		}

		Velocity2D cmd_vel;
		NS_NaviCommon::Time last_valid_control;

		//update feedback to correspond to our curent position
		sgbot::Pose2D global_pose;
		global_costmap->getRobotPose(global_pose);

		printf("global_pose x = %.4f,y = %.4f, w = %.4f ,state = %d\n",
				global_pose.x, global_pose.y,
				global_pose.theta, state);

		sgbot::Pose2D current_position;
//		NS_Transform::poseStampedTFToMsg(global_pose, current_position);
		current_position = global_pose;

		if (distance(current_position, oscillation_pose_)
				>= oscillation_distance_) {
			//TODO: oscillation
			logInfo<< "oscillation is triggered , but do nothing now";
			oscillation_pose_ = current_position;
		}

		logInfo << "control loop state ="<<state;
		switch (state) {
		case PLANNING:
			logInfo<< "control loop state is planning";
			planner_mutex.lock();
			planner_cond.notify_one();
			runPlanner_ = true;
			planner_mutex.unlock();
			break;
			case CONTROLLING:
			if(local_planner->isGoalReached())
			{
				console.message("The goal has reached!");
				goalCallbackExecutor->done();
//             printf("continue exploring? = %d\n", isExploring);
//             publishIsExploring();
				resetState();
				planner_mutex.lock();
				runPlanner_ = false;
				planner_mutex.unlock();
				break;
			}

			//TODO : check oscillation and clear
			clock_t start, end;
			start = clock();

			if(local_planner->computeVelocityCommands(cmd_vel))
			{
				console.debug(
						"Got velocity data : l_x=%.3lf, l_y=0.0f, a_z=%.3lf!",
						cmd_vel.linear, cmd_vel.angular);
				last_valid_control = NS_NaviCommon::Time::now();
				publishVelocity(cmd_vel.linear,0.0,
						cmd_vel.angular);
				end = clock();
			}
			else
			{
				console.warning("The planner can not got a valid velocity data!");
				NS_NaviCommon::Time next_control = last_valid_control + NS_NaviCommon::Duration(
						controller_patience_);

				if(NS_NaviCommon::Time::now() > next_control)
				{
					publishZeroVelocity();
					state = CLEARING;
					logInfo << "can't get valid vel and out of control patience , recovery behavior should be triggered";
				}
				else
				{
					//TODO: re-plan

					logInfo << "go back to planning";
					publishZeroVelocity();
					state = PLANNING;

					planner_mutex.lock();
					runPlanner_ = true;
					planner_cond.notify_one();
					planner_mutex.unlock();
				}
			}

			break;
			case CLEARING:
			runRecovery();
			state = PLANNING;
			break;
		}
		rate.sleep();
	}
}

void NavigationApplication::planLoop() {
	NS_NaviCommon::Rate rate(planner_frequency_);

	while (running) {
		planner_mutex.lock();
		while (!new_goal_trigger && running && !runPlanner_) {
//			planner_cond.timed_wait(planner_mutex,
//					(boost::get_system_time() + boost::posix_time::milliseconds(
//					PLANNER_LOOP_TIMEOUT)));
			logInfo << "planner_condition waiting all long ";
			planner_cond.wait(planner_mutex);
		}
		planner_mutex.unlock();

		if (!running) {
			console.message("Quit global planning loop...");
			break;
		}

		new_goal_trigger = false;

		if (!makePlan(goal, *latest_plan) && state != PLANNING) {
			console.error("Make plan failure!");
			goalCallbackExecutor->abort();
			logInfo << "global planner failed to make plan , maybe recovery should be triggered";
			continue;
		}

//		controller_mutex.lock();
		state = CONTROLLING;
		new_global_plan_ = true;
		global_planner_plan->clear();
		global_planner_plan->assign(latest_plan->begin(), latest_plan->end());
		if(runPlanner_)
			state = CONTROLLING;
//		controller_cond.notify_one();
//		controller_mutex.unlock();

		if (planner_frequency_ != 0.0f)
			rate.sleep();
	}
}
sgbot::Pose2D NavigationApplication::goalToGlobalFrame(
		sgbot::Pose2D& goal) {
	//map_to_odom * odom_to_base;
	NS_Service::Client <Transform2D> odom_tf_cli("BASE_ODOM_TF");
	NS_Service::Client <Transform2D> map_tf_cli("ODOM_MAP_TF");
	Transform2D odom_transform,map_transform;
	if(odom_tf_cli.call(odom_transform) == false){
		logError<<"get odometry transform failed";
	}
	if(map_tf_cli.call(map_transform) == false){
		logError<<"get map transform failed";
	}
	sgbot::Pose2D pose2d_result = map_transform.transform(odom_transform.transform(goal));
	logInfo<<"pose 2d in global frame is "<<pose2d_result.x<<" "<<pose2d_result.y<<" "<<pose2d_result.theta;
}
bool NavigationApplication::makePlan(const sgbot::Pose2D& goal,
		std::vector<sgbot::Pose2D>& plan) {

	boost::unique_lock<NS_CostMap::Costmap2D::mutex_t> lock(
			*(global_costmap->getLayeredCostmap()->getCostmap()->getMutex()));

	plan.clear();

	//get the starting pose of the robot
	sgbot::Pose2D global_pose;
	if (!global_costmap->getRobotPose(global_pose)) {
		console.error(
				"Unable to get starting pose of robot, unable to create global plan");
		return false;
	}

	sgbot::Pose2D start = global_pose;

	//if the planner fails or returns a zero length plan, planning failed
	if (!global_planner->makePlan(start, goal, plan) || plan.empty()) {
		console.warning("Failed to find a  plan to point (%.2f, %.2f)",
				goal.x, goal.y);
		return false;
	}

	console.debug("Plans computed, %d points to go...", plan.size());
	global_plan.resize(plan.size());
	for (size_t i = 0; i < plan.size(); i++) {
		console.debug("[%d] x = %lf, y = %lf", (i + 1), plan[i].x,
				plan[i].y);
		global_plan[i] = plan[i];
		printf("%lf,%lf,\n", plan[i].x, plan[i].y);
	}
	printf("global_plan is assigned and size = %d\n", global_plan.size());
	return true;

}
//TODO change implement ways
double NavigationApplication::distance(const sgbot::Pose2D& p1,
		const sgbot::Pose2D& p2) {
	return hypot(p1.x - p2.x,
			p1.y - p2.y);
}
void NavigationApplication::run() {
	loadParameters();

	//set up plan triple buffer
	global_planner_plan = new std::vector<sgbot::Pose2D>();
	latest_plan = new std::vector<sgbot::Pose2D>();

	/*
	 * make global planner and global costmap
	 */
	global_costmap = new NS_CostMap::CostmapWrapper();
	global_costmap->initialize();

	//load global planner
	if (global_planner_type_ == "global_planner") {
		global_planner = new NS_Planner::GlobalPlanner();
	} else {
		global_planner = new NS_Planner::GlobalPlanner();
	}

	global_planner->initialize(global_costmap);

	//load local planner

	local_planner = new NS_Planner::TrajectoryLocalPlanner();

	local_planner->initialize(global_costmap);

	state = PLANNING;

	new_goal_trigger = false;

	NS_Service::Client<NS_ServiceType::ServiceMap> map_cli("MAP");
	for (int i = 0; i < 10; i++) {
		NS_ServiceType::ServiceMap map;
		if (map_cli.call(map)) {
			if (map.result) {
				break;
			}
		}
	}

	running = true;

	plan_thread = boost::thread(
			boost::bind(&NavigationApplication::planLoop, this));

//	control_thread = boost::thread(
//			boost::bind(&NavigationApplication::controlLoop, this));

	global_costmap->start();
	sleep(2);
//    isExploring = true;
}

void NavigationApplication::quit() {
	console.message("navigation is quitting!");

	running = false;
	plan_thread.join();
}
} /* namespace NS_Navigation */
