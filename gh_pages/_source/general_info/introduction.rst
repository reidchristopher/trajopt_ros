=======================
Introduction to Trajopt
=======================

What is Trajopt?
----------------

Trajopt is an optimization based path planner that utilizes Sequential Quadratic Programming. It models the path planning problem as an optimization problem of the costs and constraints provided by the user, then iteratively attempts to converge to a global minimum (though with no guarantee).

Why use Trajopt?
----------------

1. Speed

    Trajopt solves many problems very quickly.

2. Reliability

    Trajopt can reliably solve a large number of planning problems.
    
3. Flexibility
    
    Trajopt provides the ability for a user to define their own costs and constraints. This means that you can define a unique optimization objection specific to any application.
    
