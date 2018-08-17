# trajopt_ros
Integration of TrajOpt into ROS. For in depth information, refer to the sphinx documentation.

## Build Branch Sphinx Documentation

```
cd gh_pages
sphinx-build . output
```
Now open gh_pages/output/index.rst and remove *output* directory before commiting changes.

# Overview

TrajOpt is an optimization based path planner. It can be used to plan from scratch or to optimize starting from a seed plan. It takes a set of costs and constraints and approximates these functions as locally convex in an attempt to iteratively arrive at a minimal solution.

# Basic Usage

1. Load your robot model and initialize the KDL environment

2. Define the parameters for the planner either using a Json file or in code

    - Basic Info
    - Initialization Info
    - Optimization Info (optional)
    - Costs
    - Constraints

3. Optimize!

### Basic Info

Contains basic information about the path to be planned, such as the number of trajectory states (timesteps) to be used in the path planning, whether the start of the trajectory is fixed, and what manipulator to use.

### Initialization Info

Contains information telling the planner what initial values to use for the trajectory states.

### Optimization Info

Contains information relating to the optimization routine, such as maximum number of iterations and the tolerance on constraint violations.

### Costs and Constraints

There are many different costs and constraints to use such as joint acceleration, pose error, or cartesian velocity. For more information on what costs and constraints are available and how to use them, go to the sphinx page for the trajopt package.

