#include "correlate.h"
#include <cmath>
#include <vector>
#include <omp.h>

#define BLOCK 32

void correlate(int ny, int nx, const float* data, float* result)
{
    std::vector<double> mean(ny,0.0), norm(ny,0.0);
    std::vector<double> normalized(ny*nx);

    #pragma omp parallel for
    for(int i=0;i<ny;i++){
        double sum=0;
        for(int j=0;j<nx;j++)
            sum+=data[j+i*nx];
        mean[i]=sum/nx;
    }

    #pragma omp parallel for
    for(int i=0;i<ny;i++){
        double local=0;
        for(int j=0;j<nx;j++){
            double val=data[j+i*nx]-mean[i];
            normalized[j+i*nx]=val;
            local+=val*val;
        }
        norm[i]=sqrt(local);
    }

    #pragma omp parallel for schedule(dynamic)
    for(int ii=0;ii<ny;ii+=BLOCK)
        for(int jj=0;jj<=ii;jj+=BLOCK)
            for(int i=ii;i<std::min(ii+BLOCK,ny);i++)
                for(int j=jj;j<=std::min(jj+BLOCK-1,i);j++){
                    double sum=0;
                    for(int k=0;k<nx;k++)
                        sum+=normalized[k+i*nx]*normalized[k+j*nx];
                    result[i+j*ny]=sum/(norm[i]*norm[j]);
                }
}
