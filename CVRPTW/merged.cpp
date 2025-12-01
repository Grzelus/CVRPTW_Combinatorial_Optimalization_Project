#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <utility>
#include <algorithm>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <limits>
#include <chrono>

struct Customer {
    int id;
    double x, y;
    int demand;
    double ready, due, service;
    Customer(int id, double x, double y, int demand, double ready, double due, double service)
        : id(id), x(x), y(y), demand(demand), ready(ready), due(due), service(service) {
    }
};

struct Route {
    std::vector<int> sequence;
    int load = 0;
    double cost = 0.0;
};

// do tabu search klasa i funckje pomocnicze
class Move {
public:
    std::string type;
    int a, b, route1, route2;
    double cost;
    // move constuctors
    Move() = default;
    Move(std::string o_type, int x, int routex, int y, int routey, double o_cost)
        : type(o_type), a(x), b(y), route1(routex), route2(routey), cost(o_cost) {};

    // for comparing moves
    bool operator==(const Move& other)const {
        return type == other.type &&
            a == other.a &&
            b == other.b &&
            route1 == other.route1 &&
            route2 == other.route2;
    }

    // metoda haszująca ruchy do ich szybszego znajdywania
    size_t hash() const {
        return std::hash<std::string>()(type) ^ std::hash<int>()(a) ^ std::hash<int>()(b) ^ std::hash<int>()(route1) ^ std::hash<int>()(route2);
    }
};

struct MoveHasher {
    size_t operator()(const Move& m) const { return m.hash(); }
};

// pod sortowanie najlepszych ruchów
bool comparing_moves(Move a, Move b) {
    return a.cost < b.cost;
}

double euclidean_distance(const Customer& a, const Customer& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
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

        // pracownik był za wcześnie
        double wait_time = 0.0;
        if (time < customer.ready) {
            wait_time = customer.ready - time;
            time = customer.ready;
        }

        // pracownik nie zdążył
        if (time > customer.due) {
            return { false, 0.0 };
        }

        // Obliczanie czasu i kosztu po wykonanej usłudze
        cost += wait_time;
        cost += customer.service;
        time += customer.service;

        prev = index;
    }

    // powrót do depotu
    double travel_to_depot = distance[prev][depot_index];
    cost += travel_to_depot;
    time += travel_to_depot;

    if (time > depot.due) {
        return { false, 0.0 };
    }

    return { true, cost };
}

// Remove any routes that have empty sequence from a solution
void remove_empty_routes(std::vector<Route>& solution) {
    solution.erase(std::remove_if(solution.begin(), solution.end(), [](const Route& r) {
        return r.sequence.empty();
    }), solution.end());
}

// counting cost of specyfic solution
double totalCostCount(std::vector<Route>routes, std::vector<Customer>customers, std::vector<std::vector<double>> distances) {
    double total_cost = 0.0;
    for (auto& r : routes) {
        auto fc = route_feasible_and_cost(customers, 0, distances, r.sequence);
        total_cost += fc.second;
    }
    return total_cost;
}

// creating a key for a route - string from sequance indexes 
std::string get_key_route(const std::vector<int>& seq) {
    std::string key;
    for (int x : seq) { key += std::to_string(x) + ","; }
    return key;
}

// caching function
std::pair<bool, double> route_feasible_and_cost_cached(
    const std::vector<Customer>& customers, int depot_index,
    const std::vector<std::vector<double>>& distance, const std::vector<int>& route_indexes, std::unordered_map<std::string, std::pair<bool, double>>& cost_cache) {
    std::string key = get_key_route(route_indexes);
    if (cost_cache.count(key)) return cost_cache[key];
    auto result = route_feasible_and_cost(customers, depot_index, distance, route_indexes);
    cost_cache[key] = result;
    return result;
}

int main(int argc, char** argv) {
    int start_time = time(NULL);

    std::string file_name = (argc > 1) ? argv[1] : "m2kvrptw-0.txt";
    std::ifstream file(file_name);
    if (!file) {
        std::cerr << "Error opening file.\n";
        return 1;
    }
    // getting data
    double amount, capacity;
    std::string skip_line;
    for (int i = 0; i < 4; i++) std::getline(file, skip_line);
    file >> amount >> capacity;
    for (int i = 0; i < 5; i++) std::getline(file, skip_line);
    
    // matrix of customers
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

    std::cout << n - 1 << " customers loaded.\n"; // n-1 adjustment for output consistency
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


    // --- POCZĄTEK HEURYSTYKI ZACHŁANNEJ (GREEDY) ---
    // Zastępuje algorytm Savings
    
    // Heurystyka zachłanna

    std::cout << "Starting Greedy Heuristic construction..." << std::endl;
    std::vector<Route> routes;
    std::vector<bool> visited(n, false);
    visited[depot_index] = true;
    int unvisited_count = n - 1;

    while (unvisited_count > 0)
    {
        // nowa trasa
        Route current_route;
        current_route.load = 0;
        int current_location_index = depot_index;
        double current_time = 0.0;
        bool can_add_more_to_this_route = true;
        while (can_add_more_to_this_route)
        {
            int best_customer_index = -1;
            double best_start_time = std::numeric_limits<double>::infinity();

            // szukanie najlepszego następnego klineta
            // zaczynamy od 1 bo index 0 to depot
            for (int i = 1; i < n; i++)
            {
                if (visited[i])
                {
                    continue;
                }
                const Customer& customer = customers[i];

                // Czy zapotrzebowanie danego klienta mieści się w obecnej trasie
                if (current_route.load + customer.demand > capacity)
                {
                    continue;
                }

                double travel_time = distances[current_location_index][i];
                double arrival_time = current_time + travel_time;
                double start_service_time = std::max(arrival_time, customer.ready);

                // przybycie poza oknem czasowym
                if (start_service_time > customer.due)
                {
                    continue;
                }

                // sprawdzanie powrotu
                double departure_time = start_service_time + customer.service;
                double return_travel_time = distances[i][depot_index];
                double return_to_deport_time = departure_time + return_travel_time;

                if (return_to_deport_time > customers[depot_index].due)
                {
                    continue;
                }

                // Ten klient pasuje do rozwiązania
                if (start_service_time < best_start_time)
                {
                    best_start_time = start_service_time;
                    best_customer_index = i;
                }
            }
            if (best_customer_index != -1)
            {
                const Customer& best_customer = customers[best_customer_index];

                // aktualizowanie ścieżki i czasów
                current_route.sequence.push_back(best_customer_index);
                current_route.load += best_customer.demand;
                current_time = best_start_time + best_customer.service;
                current_location_index = best_customer_index;

                visited[best_customer_index] = true;
                unvisited_count--;
            }
            else
            {
                can_add_more_to_this_route = false;
            }
        }

        if (!current_route.sequence.empty())
        {
            routes.push_back(current_route);
        }
        else if (unvisited_count > 0)
        {
            // Fallback for safety
            std::ofstream out("wynik.txt");
            out << "-1\n";
            return 0;
        }
    }

    double total_cost = 0.0;
    for (auto& route : routes) {
        auto feasible_and_cost = route_feasible_and_cost(customers, depot_index, distances, route.sequence);
        if (!feasible_and_cost.first) {
             std::ofstream out("wynik.txt");
             out << "-1\n";
             return 0;
        }
        route.cost = feasible_and_cost.second;
        total_cost += route.cost;
    }

    std::cout << "Greedy initial solution found. Routes: " << routes.size() << ", Cost: " << total_cost << std::endl;

    // --- KONIEC HEURYSTYKI ZACHŁANNEJ ---


    // --- POCZĄTEK TABU SEARCH ---
    
    // tabu search
    // zmienne do kontrolowania tabu search
    constexpr int TABU_LIMIT = 50;
    constexpr int MAX_MOVES = 2000;
    constexpr int MAX_REPEAT = 50;
    constexpr double REPEAT_EPS = 1e-6;


    std::unordered_set<Move, MoveHasher> Tabu;
    std::vector<Route> best_solution = routes;
    double best_cost = total_cost;
    std::vector<Route> actual_solution = routes;
    std::unordered_map<std::string, std::pair<bool, double>> cost_cache;


    // zmienne do kontrolowania powtórzeń
    double act_cost = best_cost;
    int repeat_counter = 0;
    double previous_cost = act_cost;

    std::cout << "Starting Tabu Search..." << std::endl;

    // maksymalnie 5 min wykonywania
    while ((time(NULL) - start_time) < 299) {
        // std::cout<<time(NULL)-start_time<<std::endl;
        std::vector<Route>routes_for_tests = actual_solution;
        std::vector<Move>list_of_moves;
        // oganicznie długości listy ruchów
        int moves_counter = 0;
        bool moves_limit = false;

        // generating list of possible moves
        for (int route1 = 0; route1 < actual_solution.size() && !moves_limit; route1++)
        {
            for (int route2 = 0; route2 < actual_solution.size() && !moves_limit; route2++)
            {
                if (route1 == route2) { continue; }


                for (int i = 0; i < actual_solution[route1].sequence.size() && !moves_limit; i++)
                {

                    for (int j = 0; j < actual_solution[route2].sequence.size() && !moves_limit; j++)
                    {
                        //   printIntVector(routes_for_tests[route1].sequence);
                        //   printIntVector(routes_for_tests[route2].sequence);
                        //   std::cout<<i<<" "<<j<<std::endl;
                        //   std::cout<<"gora"<<std::endl;

                        // swap move
                        std::swap(routes_for_tests[route1].sequence[i], routes_for_tests[route2].sequence[j]);

                        //   printIntVector(routes_for_tests[route1].sequence);
                         // printIntVector(routes_for_tests[route2].sequence);
                        //   std::cout<<i<<" "<<j<<std::endl;

                        auto change_effect1 = route_feasible_and_cost_cached(customers, 0, distances, routes_for_tests[route1].sequence, cost_cache);   //za mało argumentów
                        auto change_effect2 = route_feasible_and_cost_cached(customers, 0, distances, routes_for_tests[route2].sequence, cost_cache);
                        auto original_cost1 = route_feasible_and_cost_cached(customers, 0, distances, actual_solution[route1].sequence, cost_cache);
                        auto original_cost2 = route_feasible_and_cost_cached(customers, 0, distances, actual_solution[route2].sequence, cost_cache);

                        //std::cout<<routes_for_tests[route1].load<<" "<<routes_for_tests[route2].load<<std::endl;

                       // std::cout << change_effect1.second<<" "<<change_effect2.second<<" "<<original_cost1.second<<" "<<original_cost2.second<<std::endl;

                        // whether move possible
                        if (change_effect1.first && change_effect2.first) {
                            // checking the load
                            int client_index1 = actual_solution[route1].sequence[i];
                            int client_index2 = actual_solution[route2].sequence[j];

                            int newload1 = actual_solution[route1].load - customers[client_index1].demand + customers[client_index2].demand;
                            int newload2 = actual_solution[route2].load + customers[client_index1].demand - customers[client_index2].demand;
                            // counting cost delta and adding move to the list
                            if (newload1 >= 0 && newload1 <= capacity && newload2 >= 0 && newload2 <= capacity) {
                                double cost = (change_effect1.second + change_effect2.second) - (original_cost1.second + original_cost2.second);;
                                list_of_moves.push_back(Move("swap", i, route1, j, route2, cost));
                                moves_counter++;
                                if (moves_counter >= MAX_MOVES) { moves_limit = true; break; }
                            }
                        }
                        std::swap(routes_for_tests[route1].sequence[i], routes_for_tests[route2].sequence[j]);
                        // insertion move
                        int client_removed = actual_solution[route1].sequence[i];
                        routes_for_tests[route2].sequence.insert(routes_for_tests[route2].sequence.begin() + j, client_removed);
                        routes_for_tests[route1].sequence.erase(routes_for_tests[route1].sequence.begin() + i);


                        change_effect1 = route_feasible_and_cost_cached(customers, 0, distances, routes_for_tests[route1].sequence, cost_cache);
                        change_effect2 = route_feasible_and_cost_cached(customers, 0, distances, routes_for_tests[route2].sequence, cost_cache);

                        original_cost1 = route_feasible_and_cost_cached(customers, 0, distances, actual_solution[route1].sequence, cost_cache);
                        original_cost2 = route_feasible_and_cost_cached(customers, 0, distances, actual_solution[route2].sequence, cost_cache);
                        //   printIntVector(routes_for_tests[route1].sequence);
                        //   printIntVector(routes_for_tests[route2].sequence);

                        // std::cout << change_effect1.second<<" "<<change_effect2.second<<" "<<original_cost1.second<<" "<<original_cost2.second<<std::endl;

                        // whether move possible

                        if (change_effect1.first && change_effect2.first) {
                            int client_index = actual_solution[route1].sequence[i];

                            int newload1 = actual_solution[route1].load - customers[client_index].demand;
                            int newload2 = actual_solution[route2].load + customers[client_index].demand;
                            if (newload1 >= 0 && newload2 <= capacity) {
                                double cost = (change_effect1.second + change_effect2.second) - (original_cost1.second + original_cost2.second);
                                list_of_moves.push_back(Move("insert", i, route1, j, route2, cost));
                                //   std::cout<<cost<<" "<< act_cost<< " " << totalCostCount(routes_for_tests,customers,distances)<<std::endl;
                                moves_counter++;
                                if (moves_counter >= MAX_MOVES) { moves_limit = true; break; }
                            }
                        }

                        routes_for_tests[route1].sequence.insert(routes_for_tests[route1].sequence.begin() + i, client_removed);
                        // std::cout<<"sekwencja"<<std::endl;

                        routes_for_tests[route2].sequence.erase(routes_for_tests[route2].sequence.begin() + j);
                        // std::cout<<"sekwencja"<<std::endl;

                    }
                }
            }
        }


        // chosing the best move 
        std::sort(list_of_moves.begin(), list_of_moves.end(), comparing_moves);
        if (list_of_moves.size() > 1000) {
            list_of_moves.resize(1000);
        }
        //   printListOfMoves(list_of_moves);
          // if move wasn't found 
        if (list_of_moves.empty()) {
            break;
        }
        Move chosen;
        bool found = false;
        for (auto i : list_of_moves) {
            if (!Tabu.count(i)) {
                found = true;
                Tabu.insert(i);
                chosen = i;
                if (Tabu.size() > TABU_LIMIT) Tabu.erase(Tabu.begin());
                break;
            }
        }
        // gdy nie ma ruchow z poza tabu
        if (!found) {
            if (!list_of_moves.empty()) {
                chosen = list_of_moves[0];
                Tabu.insert(chosen);
                if (Tabu.size() > TABU_LIMIT) Tabu.erase(Tabu.begin());
            }
            else {
                break;
            }
        }


        //   std::cout<< chosen.route1<<" "<<chosen.a<<" "<<chosen.route2<<" "<<chosen.b<<std::endl;
         // printSolution(actual_solution);
          // creating actual solution
        if (chosen.type == "swap") {

            // if load is right
            int client_index1 = actual_solution[chosen.route1].sequence[chosen.a];
            int client_index2 = actual_solution[chosen.route2].sequence[chosen.b];
            actual_solution[chosen.route1].load = actual_solution[chosen.route1].load - customers[client_index1].demand + customers[client_index2].demand;
            actual_solution[chosen.route2].load = actual_solution[chosen.route2].load + customers[client_index1].demand - customers[client_index2].demand;

            // swaping move
            std::swap(actual_solution[chosen.route1].sequence[chosen.a], actual_solution[chosen.route2].sequence[chosen.b]);

        }
        else {
            int client_id_to_move = actual_solution[chosen.route1].sequence[chosen.a];
            int client_demand = customers[client_id_to_move].demand;
            // update of routes
            actual_solution[chosen.route2].sequence.insert(actual_solution[chosen.route2].sequence.begin() + chosen.b, actual_solution[chosen.route1].sequence[chosen.a]);
            actual_solution[chosen.route1].sequence.erase(actual_solution[chosen.route1].sequence.begin() + chosen.a);
            // update of loads
            actual_solution[chosen.route1].load = actual_solution[chosen.route1].load - client_demand;
            actual_solution[chosen.route2].load = actual_solution[chosen.route2].load + client_demand;
        }
        remove_empty_routes(actual_solution);


        act_cost = totalCostCount(actual_solution, customers, distances);
        if (act_cost < best_cost) {
            best_solution = actual_solution;
            best_cost = act_cost;
        }
        //   printSolution(actual_solution);
         // printSolution(best_solution);
       //   std::cout<<totalCostCount(routes,customers,distances)<<std::endl;
        // std::cout << totalCostCount(actual_solution, customers, distances) << " " << totalCostCount(best_solution, customers, distances) << std::endl;
        list_of_moves.clear();
        routes_for_tests.clear();
        cost_cache.clear();
        if (std::fabs(previous_cost - best_cost) < REPEAT_EPS) {
            repeat_counter++;
            if (repeat_counter >= MAX_REPEAT) {
                break;
            }
        }
        else {
            repeat_counter = 0;
            previous_cost = best_cost;
        }
    }
    // tabu search end
    // remove any empty routes
    remove_empty_routes(best_solution);

    std::ofstream out("wynik.txt");
    out.setf(std::ios::fixed);
    out << std::setprecision(5);
    out << best_solution.size() << " " << totalCostCount(best_solution, customers, distances) << "\n";
    for (auto& r : best_solution) {
        for (size_t k = 0; k < r.sequence.size(); k++) {
            if (k) out << " ";
            out << customers[r.sequence[k]].id;
        }
        out << "\n";
    }
    out.close();
    std::cout << "Koszt całkowity najlepszego rozwiązania: " << best_cost << std::endl;
    std::cout << "Czas wykonania algorytmu: " << (time(NULL) - start_time) << std::endl;

    return 0;
}