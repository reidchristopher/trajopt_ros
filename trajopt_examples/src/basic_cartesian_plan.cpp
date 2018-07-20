/**
 * @file basic_cartesian_plan.cpp
 * @brief Example using Trajopt for cartesian planning
 *
 * @author Levi Armstrong
 * @date Dec 18, 2017
 * @version TODO
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2017, Southwest Research Institute
 *
 * @par License
 * Software License Agreement (Apache License)
 * @par
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * @par
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <jsoncpp/json/json.h>
#include <ros/ros.h>
#include <srdfdom/model.h>
#include <tesseract_ros/kdl/kdl_chain_kin.h>
#include <tesseract_ros/kdl/kdl_env.h>
#include <tesseract_ros/ros_basic_plotting.h>
#include <trajopt/plot_callback.hpp>
#include <trajopt/problem_description.hpp>
#include <trajopt_utils/config.hpp>
#include <trajopt_utils/logging.hpp>
#include <urdf_parser/urdf_parser.h>

#include <octomap_ros/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud_conversion.h>

using namespace trajopt;
using namespace tesseract;

const std::string ROBOT_DESCRIPTION_PARAM = "robot_description"; /**< Default ROS parameter for robot description */
const std::string ROBOT_SEMANTIC_PARAM = "robot_description_semantic"; /**< Default ROS parameter for robot
                                                                          description */
const std::string TRAJOPT_DESCRIPTION_PARAM =
    "trajopt_description"; /**< Default ROS parameter for trajopt description */

bool plotting_ = false;
int steps_ = 5;
std::string method_ = "json";
urdf::ModelInterfaceSharedPtr urdf_model_; /**< URDF Model */
srdf::ModelSharedPtr srdf_model_;          /**< SRDF Model */
tesseract_ros::KDLEnvPtr env_;             /**< Trajopt Basic Environment */

TrajOptProbPtr jsonMethod()
{
  ros::NodeHandle nh;
  std::string trajopt_config;

  nh.getParam(TRAJOPT_DESCRIPTION_PARAM, trajopt_config);

  Json::Value root;
  Json::Reader reader;
  bool parse_success = reader.parse(trajopt_config.c_str(), root);
  if (!parse_success)
  {
    ROS_FATAL("Failed to load trajopt json file from ros parameter");
    exit(-1);
  }

  return ConstructProblem(root, env_);
}

TrajOptProbPtr cppMethod()
{
  ProblemConstructionInfo pci(env_);

  // Populate Basic Info
  pci.basic_info.n_steps = steps_;
  pci.basic_info.manip = "manipulator";
  pci.basic_info.start_fixed = false;
  pci.basic_info.dt_lower_lim = 0.1;
  pci.basic_info.dt_upper_lim = 5.0;
  //  pci.basic_info.dofs_fixed

  // Create Kinematic Object
  pci.kin = pci.env->getManipulator(pci.basic_info.manip);

  // Populate Init Info
  pci.init_info.type = InitInfo::STATIONARY;
  pci.init_info.data = pci.env->getCurrentJointValues(pci.kin->getName());
  pci.init_info.has_time = false;

  // Populate Cost Info
  std::vector<std::string> joint_names = pci.kin->getJointNames();
  for (std::size_t i = 0; i < joint_names.size(); i++)
  {
    std::shared_ptr<JointVelTermInfo> jv(new JointVelTermInfo);
    jv->coeffs = std::vector<double>(1, 2.5);
    jv->name = joint_names[i] + "_vel";
    jv->term_type = TT_COST;
    jv->first_step = 0;
    jv->last_step = steps_ - 1;
    jv->joint_name = joint_names[i];
    jv->penalty_type = sco::SQUARED;
    pci.cost_infos.push_back(jv);
  }

  std::shared_ptr<TotalTimeTermInfo> time_cost(new TotalTimeTermInfo);
  time_cost->name = "time_cost";
  time_cost->penalty_type = sco::SQUARED;
  time_cost->weight = 1.0;
  time_cost->term_type = TT_COST;
  pci.cost_infos.push_back(time_cost);

  std::shared_ptr<CollisionCostInfo> collision(new CollisionCostInfo);
  collision->name = "collision";
  collision->term_type = TT_COST;
  collision->continuous = false;
  collision->first_step = 0;
  collision->last_step = pci.basic_info.n_steps - 1;
  collision->gap = 1;
  collision->info = createSafetyMarginDataVector(pci.basic_info.n_steps, 0.025, 20);
  pci.cost_infos.push_back(collision);

  // Populate Constraints
  double delta = 0.5 / pci.basic_info.n_steps;
  for (auto i = 0; i < pci.basic_info.n_steps; ++i)
  {
    std::shared_ptr<StaticPoseCostInfo> pose(new StaticPoseCostInfo);
    pose->term_type = TT_CNT;
    pose->name = "waypoint_cart_" + std::to_string(i);
    pose->link = "tool0";
    pose->timestep = i;
    pose->xyz = Eigen::Vector3d(0.5, -0.2 + delta * i, 0.62);
    pose->wxyz = Eigen::Vector4d(0.0, 0.0, 1.0, 0.0);
    pose->pos_coeffs = Eigen::Vector3d(10, 10, 10);
    pose->rot_coeffs = Eigen::Vector3d(10, 10, 10);
    pci.cnt_infos.push_back(pose);
  }

  return ConstructProblem(pci);
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "basic_cartesian_plan");
  ros::NodeHandle pnh("~");
  ros::NodeHandle nh;

  // Initial setup
  std::string urdf_xml_string, srdf_xml_string;
  nh.getParam(ROBOT_DESCRIPTION_PARAM, urdf_xml_string);
  nh.getParam(ROBOT_SEMANTIC_PARAM, srdf_xml_string);
  urdf_model_ = urdf::parseURDF(urdf_xml_string);

  srdf_model_ = srdf::ModelSharedPtr(new srdf::Model);
  srdf_model_->initString(*urdf_model_, srdf_xml_string);
  env_ = tesseract_ros::KDLEnvPtr(new tesseract_ros::KDLEnv);
  assert(urdf_model_ != nullptr);
  assert(env_ != nullptr);

  bool success = env_->init(urdf_model_, srdf_model_);
  assert(success);

  pcl::PointCloud<pcl::PointXYZ> full_cloud;
  double delta = 0.05;
  int length = (1 / delta);

  for (int x = 0; x < length; ++x)
    for (int y = 0; y < length; ++y)
      for (int z = 0; z < length; ++z)
        full_cloud.push_back(pcl::PointXYZ(-0.5 + x * delta, -0.5 + y * delta, -0.5 + z * delta));

  sensor_msgs::PointCloud2 pointcloud_msg;
  pcl::toROSMsg(full_cloud, pointcloud_msg);

  octomap::Pointcloud octomap_data;
  octomap::pointCloud2ToOctomap(pointcloud_msg, octomap_data);
  octomap::OcTree* octree = new octomap::OcTree(2 * delta);
  octree->insertPointCloud(octomap_data, octomap::point3d(0, 0, 0));

  AttachableObjectPtr obj(new AttachableObject());
  shapes::OcTree* octomap_world = new shapes::OcTree(std::shared_ptr<const octomap::OcTree>(octree));
  Eigen::Affine3d octomap_pose;

  octomap_pose.setIdentity();
  octomap_pose.translation() = Eigen::Vector3d(1, 0, 0);

  obj->name = "octomap_attached";
  obj->collision.shapes.push_back(shapes::ShapeConstPtr(octomap_world));
  obj->collision.shape_poses.push_back(octomap_pose);
  obj->collision.collision_object_types.push_back(CollisionObjectType::UseShapeType);
  env_->addAttachableObject(obj);

  AttachedBodyInfo attached_body;
  attached_body.object_name = "octomap_attached";
  attached_body.parent_link_name = "base_link";
  attached_body.transform = Eigen::Affine3d::Identity();
  env_->attachBody(attached_body);

  // Create plotting tool
  tesseract_ros::ROSBasicPlottingPtr plotter(new tesseract_ros::ROSBasicPlotting(env_));

  // Get ROS Parameters
  pnh.param("plotting", plotting_, plotting_);
  pnh.param<std::string>("method", method_, method_);
  pnh.param<int>("steps", steps_, steps_);

  // Set the robot initial state
  std::unordered_map<std::string, double> ipos;
  ipos["joint_a1"] = -0.4;
  ipos["joint_a2"] = 0.2762;
  ipos["joint_a3"] = 0.0;
  ipos["joint_a4"] = -1.3348;
  ipos["joint_a5"] = 0.0;
  ipos["joint_a6"] = 1.4959;
  ipos["joint_a7"] = 0.0;

  env_->setState(ipos);

  plotter->plotScene();

  // Set Log Level
  gLogLevel = util::LevelInfo;

  ROS_INFO("Setting up the problem");
  // Setup Problem
  TrajOptProbPtr prob;
  if (method_ == "cpp")
  {
    ROS_INFO("Using C++ method");
    prob = cppMethod();
  }
  else
  {
    ROS_INFO("Using Json method");
    prob = jsonMethod();
  }

  // Solve Trajectory
  ROS_INFO("basic cartesian plan example");

  tesseract::ContactResultMap collisions;
  const std::vector<std::string>& joint_names = prob->GetKin()->getJointNames();
  const std::vector<std::string>& link_names = prob->GetKin()->getLinkNames();

  env_->continuousCollisionCheckTrajectory(joint_names, link_names, prob->GetInitTraj(), collisions);

  tesseract::ContactResultVector collision_vector;
  tesseract::moveContactResultsMapToContactResultsVector(collisions, collision_vector);
  ROS_INFO("Initial trajector number of continuous collisions: %lu\n", collision_vector.size());

  BasicTrustRegionSQP opt(prob);
  if (plotting_)
  {
    opt.addCallback(PlotCallback(*prob, plotter));
  }

  ROS_INFO("Initializing");
  opt.initialize(trajToDblVec(prob->GetInitTraj()));
  ros::Time tStart = ros::Time::now();
  ROS_INFO("Optimizing");
  opt.optimize();
  ROS_INFO("planning time: %.3f", (ros::Time::now() - tStart).toSec());

  if (plotting_)
  {
    plotter->clear();
  }

  collisions.clear();
  env_->continuousCollisionCheckTrajectory(joint_names, link_names, getTraj(opt.x(), prob->GetVars()), collisions);
  ROS_INFO("Final trajectory number of continuous collisions: %lu\n", collisions.size());
  ROS_INFO_STREAM("Final trajectory\n" << getTraj(opt.x(), prob->GetVars()));
}
