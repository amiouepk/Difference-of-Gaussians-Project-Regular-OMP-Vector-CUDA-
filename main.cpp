#include <cstdint>
#include <cmath>
#include <complex>
#include <iostream>
#include <fstream>
#include <webp/encode.h>


double difference_of_gaussians();

double laplacian_over_gaussian();

double gaussian_function();


int main(int argc, char argv[]){
    




    return 0;
}

double difference_of_gaussians(){

    return 0.0;
}

double gaussian_function(double a, double x, double b, double c){

    double f_x = a * exp(-1 * (((x - b) * (x - b)) / (2 * c)));

    return f_x;
}


