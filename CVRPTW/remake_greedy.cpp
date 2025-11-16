#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <utility>
#include <algorithm>
#include <iomanip>
#include <limits>

struct Customer {
	int id;
	double x, y;
	int demand;
	double ready, due, service;
	Customer(int id, double x, double y, int demand, double ready, double due, double service) : id(id), x(x), y(y), demand(demand), ready(ready), due(due), service(service) {}
};
struct Route {
	std::vector<int> sequence;
	int load = 0;
	double cost = 0.0;
};
double euclidean_distance(const Customer& a, const Customer& b) {
	return std::sqrt((a.x - b.x)* (a.x - b.x)+ (a.y - b.y)* (a.y - b.y));
}

std::pair<bool, double> route_feasible_and_cost(const std::vector<Customer>& customers, int depot_index, const std::vector<std::vector<double>>& distance, const std::vector<int>& route_indexes) {
	double time = 0.0;
	double cost = 0.0;
	int prev = depot_index;
	const Customer& depot = customers[depot_index];

	for (int index : route_indexes) {
		const Customer& customer = customers[index];
		double travel = distance[prev][index];

		cost += travel;
		time = time + travel;

		//pracownik był za wcześnie
		double wait_time = 0.0;
		if (time < customer.ready) {
			wait_time = customer.ready - time;
			time = customer.ready;
		}
		
		//pracownik nie zdążył
		if (time > customer.due) {
			return { false , 0.0 };
		}

		//Obliczanie czasu i kosztu po wykonanej usłudze
		cost += wait_time;
		cost += customer.service;
		time += customer.service;

		prev = index;
	}

	//powrót do depotu
	double travel_to_depot = distance[prev][depot_index];
	cost += travel_to_depot;
	time += travel_to_depot;

	if (time > depot.due) {
		return { false, 0.0 };
	}

	return { true, cost };
}

int main(int argc, char** argv) {
	std::string file_name = (argc > 1) ? argv[1] : "index2.txt";
	std::ifstream file(file_name);
	if (!file) {
		std::cerr << "Error opening file.\n";
		return 1;
	}
	// getting data
	double amount, capacity;
	std::string skip_line;
	for (int i = 0; i < 2; i++) std::getline(file, skip_line);
	file >> amount >> capacity;
	for (int i = 0; i < 5; i++) std::getline(file, skip_line);
	//matrix of customers
	std::vector<Customer> customers;
	int id, demand;
	double x, y, ready, due, service;
	while (file >> id >> x >> y >> demand >> ready >> due >> service) {
		customers.emplace_back(id, x, y, demand, ready, due, service);
	}

	int n = customers.size();
	if (n == 0) {
		std::cerr << "No customers in file.\n";
		return 1;
	}

	std::cout << n << " customers loaded.\n";
	int depot_index = 0;

	// Demand feasibility
	for (int i = 1; i < n; i++) {
		if (customers[i].demand > capacity) {
			std::ofstream out("wynik.txt");
			out << "-1\n";
			return 0;
		}
	}

	// Distance matrix
	std::vector<std::vector<double>> distances(n, std::vector<double>(n, 0.0));
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			distances[i][j] = euclidean_distance(customers[i], customers[j]);


	//Heurystyka zachłanna

	std::vector<Route> routes;
	std::vector<bool> visited(n, false);
	visited[depot_index] = true;
	int unvisited_count = n - 1;

	while (unvisited_count > 0) {

		//nowa trasa
		Route current_route;
		current_route.load = 0;
		int current_location_index = depot_index;
		double current_time = 0.0;
		bool can_add_more_to_this_route = true;
		while (can_add_more_to_this_route){
			int best_customer_index = -1;
			double best_start_time = std::numeric_limits<double>::infinity();

			//szukanie najlepszego następnego klineta
			//zaczynamy od 1 bo index 0 to depot
			for (int i = 1; i < n; i++) {
				if (visited[i]) {
					continue;
				}
				const Customer& customer = customers[i];

				// Czy zapotrzebowanie danego klienta mieści się w obecnej trasie
				if (current_route.load + customer.demand > capacity) {
					continue;
				}

				double travel_time = distances[current_location_index][i];
				double arrival_time = current_time + travel_time;
				double start_service_time = std::max(arrival_time, customer.ready);

				//przybycie poza oknem czasowym
				if (start_service_time > customer.due) {
					continue;
				}

				//sprawdzanie powrotu
				double departure_time = start_service_time + customer.service;
				double return_travel_time = distances[i][depot_index];
				double return_to_deport_time = departure_time + return_travel_time;

				if (return_to_deport_time > customers[depot_index].due) {
					continue;
				}

				//Ten klient pasuje do rozwiązania
				if (start_service_time < best_start_time) {
					best_start_time = start_service_time;
					best_customer_index = i;
				}
			}
			if (best_customer_index != -1) {
				const Customer& best_customer = customers[best_customer_index];

				//aktualizowanie ścieżki i czasów
				current_route.sequence.push_back(best_customer_index);
				current_route.load += best_customer.demand;
				current_time = best_start_time + best_customer.service;
				current_location_index = best_customer_index;

				visited[best_customer_index] = true;
				unvisited_count--;
			}
			else {
				can_add_more_to_this_route = false;
			}
		}

		if (!current_route.sequence.empty()) {
			routes.push_back(current_route);
		}
		else if (unvisited_count > 0) {
			break;
		}
	}
	double total_cost = 0.0;
	bool all_feasible = true;

	if (unvisited_count > 0) {
		all_feasible = false;
	}
	else {
		for (auto& route : routes) {
			auto feasible_and_cost = route_feasible_and_cost(customers, depot_index, distances, route.sequence);
			if (!feasible_and_cost.first) {
				all_feasible = false;
				break;
			}
			route.cost = feasible_and_cost.second;
			total_cost += route.cost;
		}
	}
	std::ofstream out("wynik.txt");

	if (!all_feasible) {
		out << "-1\n";
	}
	else {
		out.setf(std::ios::fixed);
		out << std::setprecision(5);
		out << routes.size() << " " << total_cost << "\n";

		for (auto& r : routes) {
			for (size_t k = 0; k < r.sequence.size(); k++) {
				if (k > 0) out << " ";
				// Zapisujemy ID klienta (z pliku), a nie jego indeks (z wektora)
				out << customers[r.sequence[k]].id;
			}
			out << "\n";
		}
	}
	out.close();

	return 0;
}