========================
trajopt Package Overview
========================

The trajopt package has the files that a developer is most likely to desire modifying. It contains methods for creating and registering costs/constraints, calculating cost values, constructing the TrajOpt problem, as well as a few other utilities. It is also the location to place any callbacks created for use with TrajOpt, as well as tests. Furthermore, if you are experiencing an error and are diving into code to determine its cause, this will likely be the package to start.

The main things you may be interested in developing with this package (other than fixing a potential bug) are:

    #. `Adding a custom cost or constraint <adding_a_custom_cost_or_constraint.html>`_
    #. `Adding a callback <adding_a_callback.html>`_
    #. `Adding a test <adding_a_test.html>`_