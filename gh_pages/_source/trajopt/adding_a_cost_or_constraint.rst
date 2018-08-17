==================================
Adding a Custom Cost or Constraint
==================================

To add a custom cost or constraint, we first need to determine how widely your term will be used. It should effectively fall into one of two categories:

    A. Very application specific and not reusable
    B. General enough to have value in reuse
    
For both A and B, the first few step(s) are

    1. Create a function object to calculate the cost/constraint violations
    2. (Optionally) Create a function object to calculate the analytical Jacobian of your cost/constraint
    
If continuing with B, the final few steps are the following.

    3. Create a "TermInfo" object that holds necessary information for the term
    4. Complete the :code:`hatch` and :code:`fromJson` methods for your "TermInfo"
    
Background
----------

Types of Costs
~~~~~~~~~~~~~~

There are three different cost penalty types:

    SQUARED
        The resulting cost value is the sums of the squares of the values that the cost function returns.
    ABS
        The resulting cost value is the sum of the absolute value of the values the cost function returns.
    HINGE
        The resulting cost value is the sum of max(0, value) for all of the values that the cost function returns. Another way to think of this is that only positive values returned count towards the cost.

Types of Constraints
~~~~~~~~~~~~~~~~~~~~

There are two different types of constraints:

    EQ
        Equality. The cost value is the sum of the absolute values of constraint violations. Trajopt will push these constraints to return errors of as close to zero as possible.
    INEQ
        Inequality. The cost value is the sum of max(0, value) for all of the values the error function returns. Trajopt will push these constraints to the point where the return value to be negative.

A note on HINGE type costs and INEQ type constraints: these types allow for tolerances by simply subtracting the tolerance from the actual value. For example, if A < 5 is desired, the equivalent function to give Trajopt for these term types would be A - 5. This means that when A < 5, a negative value is returned and is not counted towards the cost or constraint violations.

Creating the Term's Function
----------------------------

To simplify things, the remainder of this page will refer to the value returned by the cost or constraint function as the *error*. This error is whatever value drives the cost larger in the optimization. There are two distinct ways to describe this error function for TrajOpt. The first is to directly use affine and quadratic expression objects defined in `trajopt_sco <../trajopt_sco/expression_objects.html>`_, while the second is to define your own function object  and have the optimizer convert your function into approximations of the former as it iterates and optimizes.

The previous statement makes it clear that, if possible, using the expression objects is preferable because it reduces the work the optimizer has to do, making the program more efficient and faster. That being said, relatively few error functions can be realistically described using such objects, so the latter method is more likely to be used in general applications.

So what are these expression objects that keep being mentioned? Simply put, they represent an affine or quadratic expression with variables and coefficients for each term. So any function that can be expressed as a linear combination of variables, linear combinations between multiplied pairs of variables, plus a constant, can be described using these expression objects.

By affine expression, it means any equation of the form

    Error = A + B*x1 + C*x2 + D*x3 + E*x4 + F*x5 + ... + Coeff*xn
    
By quadratic expression, it means any equation of the form

    Error = A + B*x1^2 + C*x1*x2 + ... Coeff1,n*x1*xn + ... Coeffn,n*xn^2
    
    TODO figure out rst math    