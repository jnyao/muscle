//
// Created by 宋飞龙 on 2018-12-03.
//

#ifndef INTRODUCTION_MOVE_PATCH_H
#define INTRODUCTION_MOVE_PATCH_H

#include <Eigen/Core>
#include <igl/Hit.h>
#include <igl/ray_mesh_intersect.h>
#include <igl/boundary_loop.h>
#include <igl/map_vertices_to_circle.h>
#include <igl/harmonic.h>
#include <math.h>

Eigen::Vector3d calc_center(Eigen::MatrixXd & V) {
    Eigen::Vector3d sum(0, 0, 0);
    for (int i = 0; i < V.rows(); i++) {
        sum += V.row(i).transpose();
    }
    sum = sum / V.rows();
    return sum;
}

void translate_V(Eigen::MatrixXd & V, const Eigen::Vector3d & t) {
    for (int i = 0; i < V.rows(); i++) {
        V.row(i) += t.transpose();
    }
}

void origin_scaling(Eigen::MatrixXd & V, double s) {
    Eigen::Vector3d center = calc_center(V);
    translate_V(V, -center);
    V = V * s;
    translate_V(V, center);
}

int between_vertices(double query_rad, const Eigen::VectorXd & rads) {
    // All rads between 0 - 2 * PI
    int size = rads.size();
    for (int i = 0; i < size; i++) {
        int j = (i + 1) % size;
        int ti = (i + 1) / size;

        if (query_rad >= rads(i) && query_rad < rads(j) + ti * 2 * M_PI) {
            return i;
        }
    }

    std::cout << "return -1\n";

    return -1;
}

void translate_move_patch(
        Eigen::MatrixXd & musV,
        Eigen::MatrixXi & musF,
        Eigen::MatrixXd & bonV,
        Eigen::MatrixXi & bonF
) {
    Eigen::Vector3d musCenter = calc_center(musV);
    Eigen::Vector3d bonCenter = calc_center(bonV);

    Eigen::Vector3d direction = (bonCenter - musCenter).normalized();

    igl::Hit hit;
    bool result = igl::ray_mesh_intersect(musCenter, direction, bonV, bonF, hit);
    if (!result) exit(4);

    origin_scaling(musV, 1.2);
    translate_V(musV, hit.t * direction);
}

void map_move_patch(
        Eigen::MatrixXd & musV,
        Eigen::MatrixXi & musF,
        Eigen::MatrixXd & bonV,
        Eigen::MatrixXi & bonF,
        Eigen::MatrixXd & musVNew
) {
    Eigen::VectorXi bonBND;
    igl::boundary_loop(bonF, bonBND);
    Eigen::MatrixXd bon_uv;
    igl::map_vertices_to_circle(bonV, bonBND, bon_uv);
    Eigen::MatrixXd bonU, bonBND_U;
    igl::harmonic(bonV,bonF,bonBND,bon_uv,1,bonU);

    bonBND_U.resize(bonBND.size(), 2);
    for (int i = 0; i < bonBND.size(); i++) {
        bonBND_U.row(i) = bonU.row(bonBND(i));
    }

    Eigen::VectorXi musBND;
    igl::boundary_loop(musF, musBND);
    Eigen::MatrixXd mus_uv;
    igl::map_vertices_to_circle(musV, musBND, mus_uv);
    Eigen::MatrixXd musU, musBND_U;
    igl::harmonic(musV,musF,musBND,mus_uv,1,musU);

    musBND_U.resize(musBND.size(), 2);
    for (int i = 0; i < musBND.size(); i++) {
        musBND_U.row(i) = musU.row(musBND(i));
    }

    Eigen::VectorXd mus_bound_rad(musBND.size());
    for (int i = 0; i < musBND.size(); i++) {
        double rad = atan2(musBND_U(i,0), musBND_U(i, 1)) + M_PI;
        rad = rad < 0 ? rad + 2 * M_PI : rad;
        mus_bound_rad(i) = rad;
    }

    Eigen::VectorXd bon_bound_rad(bonBND.size());
    for (int i = 0; i < bonBND.size(); i++) {
        double rad = atan2(bonBND_U(i,0), bonBND_U(i, 1)) + M_PI;
        rad = rad < 0 ? rad + 2 * M_PI : rad;
        bon_bound_rad(i) = rad;
    }

    std::cout << "info loaded" << std::endl;

    musVNew = musV;
    for (int i = 0; i < mus_bound_rad.size(); i++) {
        double q = mus_bound_rad(i);
        int idx = between_vertices(q, bon_bound_rad);
        int idy = (idx + 1) % bon_bound_rad.size();

        double interval = bon_bound_rad(idy) - bon_bound_rad(idx);
        double query_interval = q - bon_bound_rad(idx);
        double percentage = query_interval / interval;

        Eigen::RowVector3d new_pos = percentage * bonV.row(idx) + (1 - percentage) * bonV.row(idy);
        musVNew.row(musBND(i)) = new_pos;
    }
    std::cout << "cal finished" << std::endl;

}

#endif //INTRODUCTION_MOVE_PATCH_H