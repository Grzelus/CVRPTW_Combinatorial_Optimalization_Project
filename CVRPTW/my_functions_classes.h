#ifndef MY_FUNCTIONS_CLASSES_H
#define MY_FUNCTIONS_CLASSES_H

#include <iostream>
#include <vector>
#include <cmath>

struct Customer {
	int id;

	// Location
	double x_coord;
	double y_coord;

	// Demand
	int demand;

	// Time Window
	double ready_time;
	double due_time;
	double service_time;

	//Construntor
	Customer(int id, double x_coord, double y_coord, int demand, double ready_time, double due_time, double service_time) : id(id), x_coord(x_coord), y_coord(y_coord), demand(demand), ready_time(ready_time), due_time(due_time), service_time(service_time) {}
};

struct Vehicle {
	int amount;
	double capacity;

	//Constructor
	Vehicle(int amount, double capacity) : amount(amount), capacity(capacity) {}
};


void graphConstruction(double **Distances, std::vector<Customer> customers, int numberOfCustomers);

#endif 
// do dodania do maina póki co nie kompiluje
//	double **Distances=new double*[customers.size()];
//	graphConstruction(Distances, customers, customers.size());