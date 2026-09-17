#pragma once

#include <vector>
#include "util/Random.h"
#include "util/Util.hpp"
#include <cstdint>
#include <cmath>
#include <unordered_set>


using namespace std;

enum class CastShape {
    LINE,
    CURVE
};

inline string cast_shape_to_name(CastShape s) {
    switch(s) {
        case CastShape::LINE: return "LINE";
        case CastShape::CURVE: return "CURVE";
        default: return "Unknown";
    }
}


/**
 * For determining what shape a cast was done in
 */
struct CastAnalysis {


    /*
        find max and min box and determine if it's more narrow or wide

        circle and curves
        - wide box
        
        straight line
        - box skinny along one direction


    */
    static CastShape determine_shape(vector<pair<double, double>> xy_points) {
        
        

        // remove duplicates
        vector<pair<double, double>> samples;
        unordered_set<string> point_tags;
        for (pair<double, double> point : xy_points) {
            
            string tag = to_string(point.first) + " " + to_string(point.second);
            if (point_tags.find(tag) == point_tags.end()) {
                point_tags.insert(tag);
                samples.push_back(
                    pair<double, double>(point.first, point.second)
                );
            }
        }


        // find line of best fit
        double slope;
        double intercept;
        line_of_best_fit(samples, slope, intercept);

        // get slope of orthagonal line
        double orthagonal_slope = -1/slope;

        // determine average distance to the line of best fit using the orthagonal line
        double sum_dist = 0;
        for (pair<double, double> point : samples) {
            double x = point.first;
            double y = point.second;
            
            /*
            
                find where orthagonal line drawn from point hits line of best fit

                

                orth intercept:
                    orth_intercept = point.y - orthagonal_slope*point.x 

                orth line:
                    y = orthagonal_slope*x + orth_intercept

                best fit line:
                    y = slope*x + intercept

                set equal and solve for x:
                    orthagonal_slope*x + orth_intercept = slope*x + intercept
                    orthagonal_slope*x - slope*x = intercept - orth_intercept
                    x*(orthagonal_slope - slope) = intercept - orth_intercept
                    x = (intercept - orth_intercept) / (orthagonal_slope - slope)


                use one of the lines to find the y value
            */

            double orth_intercept = y - orthagonal_slope * x;

            double x_intersection = (intercept - orth_intercept) / (orthagonal_slope - slope);
            double y_intersection = slope * x_intersection + intercept;

            double dist = eu_dist(x, y, x_intersection, y_intersection);
            sum_dist += dist;

        }
        double avg_dist = sum_dist / samples.size();
        Util::print("avg_dist", avg_dist);

        // check if less than n pixels to determine if it's a line
        if (avg_dist < 50) {
            return CastShape::LINE;
        }
        
        return CastShape::CURVE;
    }


    /*
        circle and curves
        - rise and run between points varies
        
        straight line
        - rise and run between points varies little


    */
    static CastShape cos_sim_method (vector<pair<double, double>> xy_points) {


        // remove duplicates
        vector<pair<double, double>> samples;
        unordered_set<string> point_tags;
        for (pair<double, double> point : xy_points) {
            
            string tag = to_string(point.first) + " " + to_string(point.second);
            if (point_tags.find(tag) == point_tags.end()) {
                point_tags.insert(tag);
                samples.push_back(
                    pair<double, double>(point.first, point.second)
                );
            }
        }
        int SAMPLES = samples.size();


        // Grapher::graph_points(samples, "samples.bmp");
        // Util::print_vector_pairs_copiable(samples);

        // for each of those get a slope (and get sums for average at the same time)
        double sum_x = 0;
        double sum_y = 0;
        double sum_rise = 0;
        double sum_run = 0;
        vector<pair<double, double>> rise_runs;
        for (int i = 0; i < samples.size() - 1; ++i) {
            // add to sums
            sum_x += samples[i].first;
            sum_y += samples[i].second;

            // get slope with a random point
            int r = Random::randInt((int64_t) Util::time()+i, i+1, samples.size());
            
            double rise;
            double run;
            rise_and_run(samples[i], samples[r], run, rise);
            rise_runs.push_back(
                {rise, run}
            );
            sum_rise += rise;
            sum_run += run;
        }

        // cout << "rise,  run" << endl;
        // Util::vector_pairs_to_csv(rise_runs);

        // get mean and variance of slopes
        double avg_rise = sum_rise / SAMPLES;
        double avg_run = sum_run / SAMPLES;
        CastAnalysis::add_small_if_zero(avg_rise);
        CastAnalysis::add_small_if_zero(avg_run);
        double abs_variance_rise_run = 0;
        double sum_squared_avg = pow(avg_rise, 2) + pow(avg_run, 2);
        for (pair<double, double> rise_run : rise_runs) {

            double rise = rise_run.first;
            double run = rise_run.second;
 
            /*
                cosine simalarity
                sum(a_i*b_i) / sqrt( sum(a_i^2) * sum(b_i^2) )
            */
            double numer = (rise * avg_rise + run * avg_run);
            double denom = sqrt(
                (pow(rise, 2) + pow(run, 2)) * (sum_squared_avg)
            );
            CastAnalysis::add_small_if_zero(denom);
            double cos_sim = numer / denom;
            abs_variance_rise_run += cos_sim;
        }
        abs_variance_rise_run /= SAMPLES;
        // Util::print("avg_rise", avg_rise);
        // Util::print("avg_run", avg_run);
        // Util::print("abs_variance_rise_run", abs_variance_rise_run);

        
        if (abs_variance_rise_run >= 0.88) {
            return CastShape::LINE;
        }

        return CastShape::CURVE;

    }

        

    static void line_of_best_fit (vector<pair<double, double>> xy_points, double& slope, double& intercept) {


        // get a few samples from points


        // determine line of best fit
        /*
        
            slope = (SAMPLES*sum(x_i*y_i) - sum(x_i)*sum(y_i)) / (SAMPLES*sum(x_i^2) - sum(x_i)^2)

            intercept = (sum(y_i) - slope*sum(x_i)) / SAMPLES

            residual = y_i - y^_I

        */
        double sum_xy = 0;
        double sum_x = 0;
        double sum_y = 0;
        double sum_x2 = 0;
        for (pair<double, double> point : xy_points) {
            double x = point.first;
            double y = point.second;
            sum_xy += x * y;
            sum_x += x;
            sum_y += y;
            sum_x2 += x * x;
        }

        int SAMPLES = xy_points.size();
        slope = (SAMPLES*sum_xy - sum_x*sum_y) / (SAMPLES*sum_x2 - pow(sum_x, 2));
        intercept = (sum_y - slope * sum_x) / SAMPLES;
    }

    static void rise_and_run(const pair<double, double>& point, const pair<double, double>& other_point, double& run , double& rise) {
        // bool reverse_order = point.first > other_point.first;

        // pair<double, double> first = reverse_order? other_point : point;
        // pair<double, double> second = reverse_order? point : other_point;

        rise = other_point.second - point.second;
        run = other_point.first - point.first;
        double dist = sqrt(
            pow(
                rise,
                2
            ) +
            pow(
                run,
                2
            )
        );
        CastAnalysis::add_small_if_zero(dist);
        rise /= dist;
        run /= dist;
    }

    static double eu_dist(double x1, double y1, double x2, double y2) {
        return sqrt(
            pow(
                x1-x2,
                2
            ) +
            pow(
                y1-y2,
                2
            )
        );
    }

    static void add_small_if_zero(double& value) {
        if (value == 0) value += 0.00001;
    }





};