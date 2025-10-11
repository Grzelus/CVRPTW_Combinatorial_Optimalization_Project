#include <iostream>
#include <vector>
#include <cmath>
#include "my_functions_classes.h"

using namespace std;

double countDistance(Customer A, Customer B){
    return sqrt(pow((A.x_coord-B.x_coord),2)+ pow((A.y_coord-B.y_coord),2));
}
void graphConstruction(double **Distances,std::vector<Customer> customers, int numberOfCustomers){
    for (int i=0;i<=numberOfCustomers;i++)
    {
        for(int j=0;i<=numberOfCustomers;j++)
        {
            int dist=countDistance(customers[i],customers[j]);
            Distances[i][j]=dist;
        }
    }

    for (int i=0;i<=numberOfCustomers;i++)
    {
        for(int j=0;i<=numberOfCustomers;j++)
        {
            cout<<Distances[i][j]<<" ";
        }
        cout<<endl;
    }

} 
int main(){
    Customer A(2,0,0,10,20,120,10), B(3,50,3,10,30,180,10);
    cout<<countDistance(A,B);
}