#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <utility>
#include <algorithm>
#include <iomanip>

struct Customer {
    int id;
    double x, y;
    int demand;
    double ready, due, service;
    Customer(int id, double x, double y, int demand, double ready, double due, double service)
        : id(id), x(x), y(y), demand(demand), ready(ready), due(due), service(service) {
    }
};

struct Saving {
    int i_index, j_index;
    double value;
    Saving(int i, int j, double v) : i_index(i), j_index(j), value(v) {}
};



double euclidean_distance(const Customer& a, const Customer& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

std::pair<bool, double> route_feasible_and_cost(
    const std::vector<Customer>& customers,
    int depot_index,
    const std::vector<std::vector<double>>& distance,
    const std::vector<int>& route_indexes) {

    double time = 0.0;
    double cost = 0.0;
    int prev = depot_index;

    for (int index : route_indexes) {
        double travel = distance[prev][index];
        cost += travel;
        time += travel;
        double ready = customers[index].ready;
        double due = customers[index].due;
        double service = customers[index].service;

        if (time < ready) {
            cost += (ready - time);
            time = ready;
        }
        if (time > due) return { false, 0.0 };
        cost += service;
        time += service;
        prev = index;
    }
    cost += distance[prev][depot_index];
    return { true, cost };
}


class Move{
    public:
        std::string type;
        int a,b, route1 ,route2;
        Move(std::string o_type, int x, int routex, int y,int routey): type(o_type), a(x),b(y), route1(routex),route2(routex) {};
};


int main(int argc, char** argv) {
    std::string file_name = (argc > 1) ? argv[1] : "index.txt";
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

    // Savings list
    std::vector<Saving> savings;
    for (int i = 1; i < n; i++)
        for (int j = i + 1; j < n; j++)
            savings.push_back(Saving(i, j, distances[0][i] + distances[0][j] - distances[i][j]));

    std::sort(savings.begin(), savings.end(),
        [](const Saving& a, const Saving& b) { return a.value > b.value; });

    struct Route {
        std::vector<int> sequence;
        int load = 0;
    };

    std::vector<Route> routes;
    routes.reserve(n);
    for (int i = 1; i < n; i++) {
        routes.push_back({ {i}, customers[i].demand });
    }

    auto find_route_of = [&](int customer_index)->int {
        for (int r = 0; r < (int)routes.size(); r++)
            for (int x : routes[r].sequence)
                if (x == customer_index) return r;
        return -1;
        };

    for (auto& saving : savings) {
        int i_index = saving.i_index;
        int j_index = saving.j_index;
        int i_route = find_route_of(i_index);
        int j_route = find_route_of(j_index);
        if (i_route == -1 || j_route == -1 || i_route == j_route) continue;

        bool i_is_back = (routes[i_route].sequence.back() == i_index);
        bool j_is_front = (routes[j_route].sequence.front() == j_index);
        if (!i_is_back || !j_is_front) continue;

        int total_load = routes[i_route].load + routes[j_route].load;
        if (total_load > capacity) continue;

        std::vector<int> merged = routes[i_route].sequence;
        merged.insert(merged.end(), routes[j_route].sequence.begin(), routes[j_route].sequence.end());

        auto feasible_and_cost = route_feasible_and_cost(customers, 0, distances, merged);
        if (!feasible_and_cost.first) continue;

        routes[i_route].sequence = std::move(merged);
        routes[i_route].load = total_load;
        routes.erase(routes.begin() + j_route);
    }

    double total_cost = 0.0;
    for (auto& r : routes) {
        auto fc = route_feasible_and_cost(customers, 0, distances, r.sequence);
        if (!fc.first) {
            std::ofstream out("wynik.txt");
            out << "-1\n";
            return 0;
        }
        total_cost += fc.second;
    }

    //tabu search
    int start_time=time(NULL);
    std::vector<Move> Tabu;
    std::vector<Route> best_solution = routes;
    std::vector<Route> actual_solution = routes;
    //maksymalnie 5 min wykonywania
    while(time(NULL)-start_time<300){
        std::vector<Move>list_of_moves;

        //generating list of possible moves
        for (int route1=0;route1<actual_solution.size();route1++){ 
            for (int i=0;i<actual_solution[route1].load;i++)
            {
                for (int route2=0;route2<actual_solution.size();route2++){ 
                    for (int j=0;j<actual_solution[route2].load;j++)
                    {   
                        //swap move
                        std::vector<Route>routes_for_tests = actual_solution;
                        std::swap(routes_for_tests[route1].sequence[i],routes_for_tests[route2].sequence[j]);
                        bool x, int cost = route_feasible_and_cost(routes_for_tests );// co dokladnie ta funckja robi????
                        if(x){
                            list_of_moves.push_back(Move("swap",i, route1,j,route2, cost));
                        }
                        routes_for_tests=actual_solution;
                            //insertion
                            routes_for_tests[route1].sequence.erase(routes_for_tests.begin() + i);
                            routes_for_tests[route1].sequence.insert(routes_for_tests.begin() + i);

                    }
                }
            }        
        }


    }


    //tabu search
    std::ofstream out("wynik.txt");
    out.setf(std::ios::fixed);
    out << std::setprecision(5);
    out << routes.size() << " " << total_cost << "\n";
    for (auto& r : routes) {
        for (size_t k = 0; k < r.sequence.size(); k++) {
            if (k) out << " ";
            out << customers[r.sequence[k]].id;
        }
        out << "\n";
    }
    out.close();

    return 0;
}