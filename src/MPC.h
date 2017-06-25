#ifndef MPC_H
#define MPC_H

#include <vector>
#include "Eigen-3.3/Eigen/Core"

class Paramaters {
public:

    uint32_t factor_cte = 1000;
    uint32_t factor_epsi = 100;
    uint32_t factor_v = 1;
    uint32_t factor_delta = 1000;
    uint32_t factor_a = 1;
    uint32_t factor_delta_delta = 50;
    uint32_t factor_a_delta = 1;

    Paramaters();

    virtual ~Paramaters();
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
