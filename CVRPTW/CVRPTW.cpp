#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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

//Main Function
int main(void) {
	// reading from file
	std::ifstream file("index.txt");
	if (!file) {
		std::cerr << "Error opening file.\n";
		return 1;
	}
	// temporary initialization
	Vehicle vehicle(0,0);

	//skiping blank and header lines
	std::string skip_line;
	for (int i = 0; i < 2; i++) {
		std::getline(file, skip_line);
	}

	// storing data in vehicle object
	file >> vehicle.amount >> vehicle.capacity;


	//skiping blank and header lines
	for (int i = 0; i < 5; i++) {
		std::getline(file, skip_line);
	}
	
	std::vector<Customer> customers;

	int id, demand;
	double x, y, ready, due, service;

	//filling vector with customers
	while (file >> id >> x >> y >> demand >> ready >> due >> service) {
		customers.emplace_back(id, x, y, demand, ready, due, service);
	}

	std::cout << customers.size() << "customers has been loaded" << std::endl;
	
	return 0;
}