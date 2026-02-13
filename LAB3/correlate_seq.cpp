#include "correlate.h"
#include <cmath>
#include <vector>

void correlate(int ny, int nx, const float* data, float* result)
{
    std::vector<double> mean(ny,0.0), norm(ny,0.0);
    std::vector<double> normalized(ny*nx);

    for(int i=0;i<ny;i++){
        for(int j=0;j<nx;j++)
            mean[i]+=data[j+i*nx];
        mean[i]/=nx;
    }

    for(int i=0;i<ny;i++){
        for(int j=0;j<nx;j++){
            double val=data[j+i*nx]-mean[i];
            normalized[j+i*nx]=val;
            norm[i]+=val*val;
        }
        norm[i]=sqrt(norm[i]);
    }

    for(int i=0;i<ny;i++)
        for(int j=0;j<=i;j++){
            double sum=0.0;
            for(int k=0;k<nx;k++)
                sum+=normalized[k+i*nx]*normalized[k+j*nx];

            result[i+j*ny]=sum/(norm[i]*norm[j]);
        }
}
