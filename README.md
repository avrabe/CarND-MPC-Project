# CarND-Controls-MPC
[![Build Status](https://travis-ci.org/avrabe/CarND-MPC-Project.svg?branch=master)](https://travis-ci.org/avrabe/CarND-MPC-Project)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/bfd8d52eb2b5468d84e01c01a1a783c6)](https://www.codacy.com/app/avrabe/CarND-MPC-Project?utm_source=github.com&utm_medium=referral&utm_content=avrabe/CarND-MPC-Project&utm_campaign=badger)
[![Quality Gate](https://sonarqube.com/api/badges/gate?key=carnd-mpc-project)](https://sonarqube.com/dashboard/index/carnd-mpc-project)

Self-Driving Car Engineer Nanodegree Program

---
<img src="drive.gif" /></br>
---
## Model documentation

### Checklist
- [x] Your code should compile.
- [x] The Model description (incl. state, actuators and update equations)
- [X] Timestep Length and Elapsed Duration (N & dt)
- [x] Polynomial Fitting and MPC Preprocessing (show and describe)
- [x] Model Predictive Control with Latency
- [x] The vehicle must successfully drive a lap around the track.      

### onMessage
When receiving data from the simulator, below activities are run:

![alt text](onMessage.png "onMessage")<br>

Based on the received car data, an estimated data will be calculated to compensate latency.
It is assumed that the latency is 100ms and below formulas are used to get
the estimated position, psi (orientation) and v (vehicle velocity). 

![px += v * std::cos(psi) * dt](https://latex.codecogs.com/gif.latex?px_t_&plus;_1&space;=&space;px_t&space;&plus;&space;v_t&space;*&space;\cos(psi_t)&space;*&space;dt)<br>
![py += v * std::sin(psi) * dt](https://latex.codecogs.com/gif.latex?py_t_&plus;_1&space;=&space;py_t&space;&plus;&space;v_t&space;*&space;\sin(psi_t)&space;*&space;dt)<br>
![psi -= v * steer_value / params.Lf * dt](https://latex.codecogs.com/gif.latex?psi_{t&plus;1}&space;=&space;psi_t&space;-&space;v_t&space;\frac{steering_t}{Lf}dt)<br>
![v += throttle_value * dt](https://latex.codecogs.com/gif.latex?v_{t&plus;1}&space;=&space;v_t&space;&plus;throttle_t*dt)<br>

Using the predicted car position the received waypoints are converted from
global coordinates to coordinates relative to the car position and orientation. Below formulas are used to convert:

![ptsx[i] = (shift_x * cos(0 - psi) - shift_y * sin(0 - psi))](https://latex.codecogs.com/gif.latex?ptsx_i&space;=&space;(ptsx_i&space;-&space;px_{t&plus;1})&space;\cos(0&space;-&space;psi_{t&plus;1})&space;-&space;(ptsy_i&space;-&space;py_{t&plus;1})&space;\sin(0&space;-&space;psi))<br>
![ptsy[i] = (shift_x * sin(0 - psi) + shift_y * cos(0 - psi))](https://latex.codecogs.com/gif.latex?ptsy_i&space;=&space;(ptsx_i&space;-&space;px_{t&plus;1})&space;\sin(0&space;-&space;psi_{t&plus;1})&space;&plus;&space;(ptsy_i&space;-&space;py_{t&plus;1})&space;\cos(0&space;-&space;psi))<br>

The new waypoints are converted into a polynomial of 3rd order.
The cross track error and the orientation error are calculated based on the polynomial.
 
Afterwards it is checked if the car will soon enter a steep curve and based
 on the result the reference velocity is set.
 
 
Using the return values from the MPC solver, the new throttle and orientation is set and
 the predicted waypoints are returned.

### MPC solve 
The model predictive control solver is based on a kinematic model. The model with the given bounds and constraints is handed over to an optimizer. The optimzer tries to find the best solution for the given N prediction steps with dt time in between. 
If the optimizer returns with no error the first actuator values returned. In case of an error, the actual front wheel angle and a throttle of -1 is returned to bring the car into a safe state. The car model can be described as:

![px += v * std::cos(psi) * dt](https://latex.codecogs.com/gif.latex?px_t_&plus;_1&space;=&space;px_t&space;&plus;&space;v_t&space;*&space;\cos(psi_t)&space;*&space;dt)<br>
![py += v * std::sin(psi) * dt](https://latex.codecogs.com/gif.latex?py_t_&plus;_1&space;=&space;py_t&space;&plus;&space;v_t&space;*&space;\sin(psi_t)&space;*&space;dt)<br>
![psi -= v * steer_value / params.Lf * dt](https://latex.codecogs.com/gif.latex?psi_{t&plus;1}&space;=&space;psi_t&space;+&space;v_t&space;\frac{steering_t}{Lf}dt)<br>
![v += throttle_value * dt](https://latex.codecogs.com/gif.latex?v_{t&plus;1}&space;=&space;v_t&space;&plus;throttle_t*dt)<br>

With the car states: px, py, psi, v (x, y position, vehicle orientation, velocity) and the actuators: steering and throttle.
 
In additon, the cross track error (cte) and the orientation error (epsi) are calculated and added to the vehicle state with following formula:

![cte0 + (v0 * CppAD::sin(epsi0) * dt](https://latex.codecogs.com/gif.latex?cte_{t&plus;1}&space;=&space;cte_t&space;&plus;&space;v_t&space;\sin(epsi_t)&space;*&space;dt)<br>
![](https://latex.codecogs.com/gif.latex?epsi_{t&plus;1}&space;=&space;epsi_t&space;-&space;v_t&space;\frac{steering_t}{Lf}&space;dt)

These terms are added into a cost function which is used by the optimizer to minimize actuator usage. The cost factor are weighted to emphasize the importance especially of cte, epsi and especially the vehicle orientation. 

The prediction steps N and the time between the steps dt
are chosen based on a trade-off between the predicted resolution and length of the prediction.
Adding a larger N and small dt I could see a better model resolution but 
at higher computational costs which lead to worse predictions if the 
calculations are stopped after the defined time. Setting the estimates too small,
also did not work as the predictions did turn out not good enough. Finally I choose
N with 10 and dt as 0.1 ms.


## Dependencies

* cmake >= 3.5
 * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools]((https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)
* [uWebSockets](https://github.com/uWebSockets/uWebSockets)
  * Run either `install-mac.sh` or `install-ubuntu.sh`.
  * If you install from source, checkout to commit `e94b6e1`, i.e.
    ```
    git clone https://github.com/uWebSockets/uWebSockets 
    cd uWebSockets
    git checkout e94b6e1
    ```
    Some function signatures have changed in v0.14.x. See [this PR](https://github.com/udacity/CarND-MPC-Project/pull/3) for more details.
* Fortran Compiler
  * Mac: `brew install gcc` (might not be required)
  * Linux: `sudo apt-get install gfortran`. Additionall you have also have to install gcc and g++, `sudo apt-get install gcc g++`. Look in [this Dockerfile](https://github.com/udacity/CarND-MPC-Quizzes/blob/master/Dockerfile) for more info.
* [Ipopt](https://projects.coin-or.org/Ipopt)
  * Mac: `brew install ipopt`
  * Linux
    * You will need a version of Ipopt 3.12.1 or higher. The version available through `apt-get` is 3.11.x. If you can get that version to work great but if not there's a script `install_ipopt.sh` that will install Ipopt. You just need to download the source from the Ipopt [releases page](https://www.coin-or.org/download/source/Ipopt/) or the [Github releases](https://github.com/coin-or/Ipopt/releases) page.
    * Then call `install_ipopt.sh` with the source directory as the first argument, ex: `bash install_ipopt.sh Ipopt-3.12.1`. 
  * Windows: TODO. If you can use the Linux subsystem and follow the Linux instructions.
* [CppAD](https://www.coin-or.org/CppAD/)
  * Mac: `brew install cppad`
  * Linux `sudo apt-get install cppad` or equivalent.
  * Windows: TODO. If you can use the Linux subsystem and follow the Linux instructions.
* [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page). This is already part of the repo so you shouldn't have to worry about it.
* Simulator. You can download these from the [releases tab](https://github.com/udacity/self-driving-car-sim/releases).
* Not a dependency but read the [DATA.md](./DATA.md) for a description of the data sent back from the simulator.


## Basic Build Instructions


1. Clone this repo.
2. Make a build directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./mpc`.

## Tips

1. It's recommended to test the MPC on basic examples to see if your implementation behaves as desired. One possible example
is the vehicle starting offset of a straight line (reference). If the MPC implementation is correct, after some number of timesteps
(not too many) it should find and track the reference line.
2. The `lake_track_waypoints.csv` file has the waypoints of the lake track. You could use this to fit polynomials and points and see of how well your model tracks curve. NOTE: This file might be not completely in sync with the simulator so your solution should NOT depend on it.
3. For visualization this C++ [matplotlib wrapper](https://github.com/lava/matplotlib-cpp) could be helpful.

## Editor Settings

We've purposefully kept editor configuration files out of this repo in order to
keep it as simple and environment agnostic as possible. However, we recommend
using the following settings:

* indent using spaces
* set tab width to 2 spaces (keeps the matrices in source code aligned)

## Code Style

Please (do your best to) stick to [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html).

## Project Instructions and Rubric

Note: regardless of the changes you make, your project must be buildable using
cmake and make!

More information is only accessible by people who are already enrolled in Term 2
of CarND. If you are enrolled, see [the project page](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/f1820894-8322-4bb3-81aa-b26b3c6dcbaf/lessons/b1ff3be0-c904-438e-aad3-2b5379f0e0c3/concepts/1a2255a0-e23c-44cf-8d41-39b8a3c8264a)
for instructions and the project rubric.

## Hints!

* You don't have to follow this directory structure, but if you do, your work
  will span all of the .cpp files here. Keep an eye out for TODOs.

## Call for IDE Profiles Pull Requests

Help your fellow students!

We decided to create Makefiles with cmake to keep this project as platform
agnostic as possible. Similarly, we omitted IDE profiles in order to we ensure
that students don't feel pressured to use one IDE or another.

However! I'd love to help people get up and running with their IDEs of choice.
If you've created a profile for an IDE that you think other students would
appreciate, we'd love to have you add the requisite profile files and
instructions to ide_profiles/. For example if you wanted to add a VS Code
profile, you'd add:

* /ide_profiles/vscode/.vscode
* /ide_profiles/vscode/README.md

The README should explain what the profile does, how to take advantage of it,
and how to install it.

Frankly, I've never been involved in a project with multiple IDE profiles
before. I believe the best way to handle this would be to keep them out of the
repo root to avoid clutter. My expectation is that most profiles will include
instructions to copy files to a new location to get picked up by the IDE, but
that's just a guess.

One last note here: regardless of the IDE used, every submitted project must
still be compilable with cmake and make./
