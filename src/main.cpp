#include <math.h>
#include <uWS/uWS.h>
#include <thread>
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "MPC.h"
#include "json.hpp"

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }

double deg2rad(double x) { return x * pi() / 180; }

double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
    auto found_null = s.find("null");
    auto b1 = s.find_first_of("[");
    auto b2 = s.rfind("}]");
    if (found_null != std::string::npos) {
        return "";
    } else if (b1 != std::string::npos && b2 != std::string::npos) {
        return s.substr(b1, b2 - b1 + 2);
    }
    return "";
}

// Evaluate a polynomial.
double polyeval(Eigen::VectorXd coeffs, double x) {
    double result = 0.0;
    for (int i = 0; i < coeffs.size(); i++) {
        result += coeffs[i] * pow(x, i);
    }
    return result;
}

// Fit a polynomial.
// Adapted from
// https://github.com/JuliaMath/Polynomials.jl/blob/master/src/Polynomials.jl#L676-L716
Eigen::VectorXd polyfit(Eigen::VectorXd xvals, Eigen::VectorXd yvals,
                        int order) {
    assert(xvals.size() == yvals.size());
    assert(order >= 1 && order <= xvals.size() - 1);
    Eigen::MatrixXd A(xvals.size(), order + 1);

    for (int i = 0; i < xvals.size(); i++) {
        A(i, 0) = 1.0;
    }

    for (int j = 0; j < xvals.size(); j++) {
        for (int i = 0; i < order; i++) {
            A(j, i + 1) = A(j, i) * xvals(j);
        }
    }

    auto Q = A.householderQr();
    auto result = Q.solve(yvals);
    return result;
}

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<float> fsec;
auto t0 = Time::now();
uint64_t time_s = 0;
Paramaters params;

int main(int argc, char **argv) {
    uWS::Hub h;

    int opt;

    while ((opt = getopt(argc, argv, "c:e:v:d:a:i:j:r:t:")) != EOF) {
        switch (opt) {
            case 'c':
                params.factor_cte = atoi(optarg);
                break;
            case 'e':
                params.factor_epsi = atoi(optarg);
                break;
            case 'v':
                params.factor_v = atoi(optarg);
                break;
            case 'd':
                params.factor_delta = atoi(optarg);
                break;
            case 'a':
                params.factor_a = atoi(optarg);
                break;
            case 'i':
                params.factor_delta_delta = atoi(optarg);
                break;
            case 'j':
                params.factor_a_delta = atoi(optarg);
                break;
            case 'r':
                params.ref_v = atof(optarg);
                break;
            case 't':
                time_s = atoi(optarg);
                break;
            default:
                abort();
        }
    }
    if (time_s == 0) {
        std::cout << "not timeboxed" << std::endl;
    } else {
        std::cout << "limit to " << time_s << " seconds." << std::endl;
    }

    // MPC is initialized here!
    MPC mpc;

    h.onMessage([&mpc](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
                       uWS::OpCode opCode) {
        // "42" at the start of the message means there's a websocket message event.
        // The 4 signifies a websocket message
        // The 2 signifies a websocket event
        std::string sdata = std::string(data).substr(0, length);
        std::cout << sdata << std::endl;
        if (sdata.size() > 2 && sdata[0] == '4' && sdata[1] == '2') {
            std::string s = hasData(sdata);
            if (s != "") {
                auto j = json::parse(s);
                std::string event = j[0].get<std::string>();
                if (event == "telemetry") {
                    // j[1] is the data JSON object
                    std::vector<double> ptsx = j[1]["ptsx"];
                    std::vector<double> ptsy = j[1]["ptsy"];
                    double px = j[1]["x"];
                    double py = j[1]["y"];
                    double psi = j[1]["psi"];
                    double v = j[1]["speed"];
                    double steer_value = j[1]["steering_angle"];
                    double throttle_value = j[1]["throttle"];


                    /*
                     * Take the current vehicle information and perform
                     * a very simple prediction where the car will be
                     * within the estimated latency for the algorithm.
                     *
                     * Use this as base to calculate the next steps.
                     */
                    double dt = 0.1;
                    px += v * std::cos(psi) * dt;
                    py += v * std::sin(psi) * dt;
                    psi -= v * steer_value / params.Lf * dt;
                    v += throttle_value * dt;



                    for (uint32_t i = 0; i < ptsx.size(); i++) {
                        double shift_x = ptsx[i] - px;
                        double shift_y = ptsy[i] - py;

                        ptsx[i] = (shift_x * cos(0 - psi) - shift_y * sin(0 - psi));
                        ptsy[i] = (shift_x * sin(0 - psi) + shift_y * cos(0 - psi));

                    }

                    Eigen::Map<Eigen::VectorXd> ptsx_tranform(&ptsx[0], 6);
                    Eigen::Map<Eigen::VectorXd> ptsy_tranform(&ptsy[0], 6);

                    auto coeffs = polyfit(ptsx_tranform, ptsy_tranform, 3);
                    // should be the shorttest distance of the car(px,py) to the polynomial
                    double cte = polyeval(coeffs, 0);
                    // should be: psi - atan(coeffs[1]+2*px*coeffs[2]+3*coeffs[3]*pow(px,2))
                    //            with px now == 0 it would result into
                    double epsi = -atan(coeffs[1]);

                    /*
                    * DONE: Calculate steering angle and throttle using MPC.
                    *
                    * Both are in between [-1, 1].
                    *
                    */

                    Eigen::VectorXd state(6);
                    // MiSRA does not like comma operators.
                    state.fill(0);
                    state(3) = v;
                    state(4) = cte;
                    state(5) = epsi;

                    //Display the waypoints/reference line
                    std::vector<double> next_x_vals;
                    std::vector<double> next_y_vals;

                    /*
                     * Calculate the reference points based on the car position.
                     * In addition perform a very basic test if a steep curve
                     * is comming up and adjust the reference speed accordingly.
                     *
                     */
                    double curve = 0;
                    uint32_t count = 0;
                    for (uint32_t i = 0; i < 80; i = i + 2) {
                        double x = (double) i;
                        double y = polyeval(coeffs, i);
                        if (x >= 0 && count < 10) {
                            curve = std::max(curve, std::fabs(y));
                            count++;
                        }
                        next_x_vals.push_back(x);
                        next_y_vals.push_back(y);
                    }
                    if (curve < 2) {
                        params.factor_ref_v = 2;
                    } else {
                        params.factor_ref_v = 1;
                    }
                    std::cout << "curve " << curve << std::endl;


                    std::vector<double> ret = mpc.Solve(state, coeffs, params);
                    steer_value = ret[0];
                    throttle_value = ret[1];

                    json msgJson;
                    // NOTE: Remember to divide by deg2rad(25) before you send the steering value back.
                    // Otherwise the values will be in between [-deg2rad(25), deg2rad(25] instead of [-1, 1].
                    msgJson["steering_angle"] = -1 * steer_value / 0.46332;
                    msgJson["throttle"] = throttle_value;

                    //Display the MPC predicted trajectory
                    std::vector<double> mpc_x_vals;
                    std::vector<double> mpc_y_vals;
                    for (uint32_t i = 2; i < ret.size(); i++) {
                        if (i % 2) {
                            mpc_y_vals.push_back(ret[i]);
                        } else {
                            mpc_x_vals.push_back(ret[i]);
                        }
                    }

                    //.. add (x,y) points to list here, points are in reference to the vehicle's coordinate system
                    // the points in the simulator are connected by a Green line

                    msgJson["mpc_x"] = mpc_x_vals;
                    msgJson["mpc_y"] = mpc_y_vals;

                    //.. add (x,y) points to list here, points are in reference to the vehicle's coordinate system
                    // the points in the simulator are connected by a Yellow line

                    msgJson["next_x"] = next_x_vals;
                    msgJson["next_y"] = next_y_vals;


                    auto msg = "42[\"steer\"," + msgJson.dump() + "]";
                    std::cout << msg << std::endl;
                    // Latency
                    // The purpose is to mimic real driving conditions where
                    // the car does actuate the commands instantly.
                    //
                    // Feel free to play around with this value but should be to drive
                    // around the track with 100ms latency.
                    //
                    // NOTE: REMEMBER TO SET THIS TO 100 MILLISECONDS BEFORE
                    // SUBMITTING.
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

                    auto t1 = Time::now();
                    fsec fs = t1 - t0;
                    std::cout << fs.count() << "s\n";
                    if (time_s > 0 && fs.count() > time_s) {
                        exit(0);
                    }
                }
            } else {
                // Manual driving
                std::string msg = "42[\"manual\",{}]";
                ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
            }
        }
    });

    // We don't need this since we're not using HTTP but if it's removed the
    // program
    // doesn't compile :-(
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data,
                       size_t, size_t) {
        const std::string s = "<h1>Hello world!</h1>";
        if (req.getUrl().valueLength == 1) {
            res->end(s.data(), s.length());
        } else {
            // i guess this should be done more gracefully?
            res->end(nullptr, 0);
        }
    });

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
        std::cout << "Connected!!!" << std::endl;
    });

    h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
                           char *message, size_t length) {
        ws.close();
        std::cout << "Disconnected" << std::endl;
    });

    int port = 4567;
    if (h.listen(port)) {
        std::cout << "Listening to port " << port << std::endl;
    } else {
        std::cerr << "Failed to listen to port" << std::endl;
        return -1;
    }
    h.run();
}
