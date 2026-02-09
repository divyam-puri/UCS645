#include <omp.h>
#include <iostream>
#include <vector>
#include <cmath>

#define N 1000

int main() {

    std::vector<double> x(N), y(N), z(N);
    std::vector<double> fx(N,0), fy(N,0), fz(N,0);

    for(int i=0;i<N;i++) {
        x[i] = rand()%100;
        y[i] = rand()%100;
        z[i] = rand()%100;
    }

    double start = omp_get_wtime();

    #pragma omp parallel for
    for(int i=0;i<N;i++) {

        double fix=0, fiy=0, fiz=0;

        for(int j=0;j<N;j++) {
            if(i==j) continue;

            double dx = x[i]-x[j];
            double dy = y[i]-y[j];
            double dz = z[i]-z[j];

            double r = sqrt(dx*dx+dy*dy+dz*dz)+0.0001;

            double f = 1/(r*r);

            fix += f*dx;
            fiy += f*dy;
            fiz += f*dz;
        }

        fx[i]=fix;
        fy[i]=fiy;
        fz[i]=fiz;
    }

    double end = omp_get_wtime();

    std::cout << "Time: " << end-start << " seconds\n";
}
