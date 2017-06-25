#ifndef MPC_H
#define MPC_H

#include <vector>
#include "Eigen-3.3/Eigen/Core"

class Paramaters {
public:
    //-c 1000 -e 1000 -v 10 -d 10000 -a 1 -i 100  -j 1
    uint32_t factor_cte = 1000;
    uint32_t factor_epsi = 1000;
    uint32_t factor_v = 10;
    uint32_t factor_delta = 10000;
    uint32_t factor_a = 1;
    uint32_t factor_delta_delta = 100;
    uint32_t factor_a_delta = 1;

    double ref_v = 40;
    double factor_ref_v = 1;
// Lf was tuned until the the radius formed by the simulating the model
// presented in the classroom matched the previous radius.
//
// This is the length from front to CoG that has a similar radius.
    double Lf = 2.67;

    Paramaters();

    virtual ~Paramaters();

    double getV();
};

class MPC {
 public:
  MPC();

  virtual ~MPC();

  // Solve the model given an initial state and polynomial coefficients.
  // Return the first actuatotions.
  std::vector<double> Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs, Paramaters params);
};

#endif /* MPC_H */
