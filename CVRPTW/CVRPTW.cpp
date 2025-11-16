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

struct Route {
    std::vector<int> sequence;
    int load = 0;
};

//do tabu search klasa i funckje pomocnicze
class Move{
    public:
        std::string type;
        int a,b, route1 ,route2;
        double cost;
        Move() = default;
        Move(std::string o_type, int x, int routex, int y,int routey, double o_cost): type(o_type), a(x),b(y), route1(routex),route2(routey), cost(o_cost) {};

        //for comparing moves
        bool operator==(const Move& other)const{
            return type==other.type &&
            a==other.a &&
            b==other.b &&
            route1==other.route1 &&
            route2==other.route2;
        }

        size_t hash() const {
        return std::hash<std::string>()(type) ^ std::hash<int>()(a) ^ std::hash<int>()(b) ^std::hash<int>()(route1) ^ std::hash<int>()(route2);
}
};

struct MoveHasher{
    size_t operator()(const Move& m) const {return m.hash();}
};
//pod sortowanie najlepszych ruchów
bool comparing_moves(Move a, Move b){
    return a.cost<b.cost;
}

//end tabu search
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

void printListOfMoves(std::vector<Move> moves){
    for(auto i: moves){
        std::cout<<i.route1<<" "<<i.route2<<" "<<i.a<<" "<<i.b<<" "<<i.cost<<std::endl;
    }
}
void printIntVector(std::vector<int> numb){
    for(auto i: numb){
        std::cout<<i<<" ";
    }
    std::cout<<std::endl;
}

void printSolution(std::vector<Route> solution){
    for(auto x : solution){
        printIntVector(x.sequence);
    }
    std::cout<<std::endl;
}

//counting cost of specyfic solution
double totalCostCount(std::vector<Route>routes,std::vector<Customer>customers, std::vector<std::vector<double>> distances){
    double total_cost = 0.0;
    for (auto& r : routes) {
        auto fc = route_feasible_and_cost(customers, 0, distances, r.sequence);
        if (!fc.first) {
            return 0;
        }
        total_cost += fc.second;
    }
    return total_cost;
}

std::string get_key_route(const std::vector<int>& seq) {
    std::string key;
    for (int x : seq) key += std::to_string(x) + ",";
    return key;
}
std::pair<bool, double> route_feasible_and_cost_cached(
    const std::vector<Customer>& customers, int depot_index,
    const std::vector<std::vector<double>>& distance, const std::vector<int>& route_indexes,std::unordered_map<std::string,std::pair<bool,double>>& cost_cache) {
    std::string key = get_key_route(route_indexes);
    if (cost_cache.count(key)) return cost_cache[key];
    auto result = route_feasible_and_cost(customers, depot_index, distance, route_indexes);
    cost_cache[key] = result;
    return result;
}


// Zastąp wywołania route_feasible_and_cost na route_feasible_and_cost_cached.
// Wyczyść cache na początku każdej iteracji while: cost_cache.clear();

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

    // Savings list
    std::vector<Saving> savings;
    for (int i = 1; i < n; i++)
        for (int j = i + 1; j < n; j++)
            savings.push_back(Saving(i, j, distances[0][i] + distances[0][j] - distances[i][j]));

    std::sort(savings.begin(), savings.end(),
        [](const Saving& a, const Saving& b) { return a.value > b.value; });



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
    
    std::unordered_set<Move,MoveHasher> Tabu;
    std::vector<Route> best_solution = routes; 
    double best_cost= total_cost;
    std::vector<Route> actual_solution = routes;
    std::unordered_map<std::string, std::pair<bool,double>> cost_cache;
    double act_cost=best_cost;        
    //maksymalnie 5 min wykonywania
    int start_time=time(NULL);



    while((time(NULL)-start_time)<300){
        std::cout<<time(NULL)-start_time<<std::endl;
        std::vector<Route>routes_for_tests = actual_solution;
        std::vector<Move>list_of_moves;
     //oganicznie długości listy ruchów
        const int MAX_MOVES=2000;
        int moves_counter=0;
        bool moves_limit=false;
        //generating list of possible moves
        for (int route1=0;route1<actual_solution.size() && !moves_limit;route1++)
        { 
            for (int route2=0;route2<actual_solution.size() && !moves_limit;route2++)
            {
                if(route1==route2){continue;}


                 for (int i=0; i<actual_solution[route1].sequence.size() && !moves_limit;i++)
                {
                 
                    for (int j=0; j<actual_solution[route2].sequence.size() && !moves_limit;j++)
                    {   
                        
                        //swap move

                       
                       //   printIntVector(routes_for_tests[route1].sequence);
                     //   printIntVector(routes_for_tests[route2].sequence);
                     //   std::cout<<i<<" "<<j<<std::endl;
                    //    std::cout<<"gora"<<std::endl;

                        std::swap(routes_for_tests[route1].sequence[i],routes_for_tests[route2].sequence[j]);
                       
                      //  printIntVector(routes_for_tests[route1].sequence);
                       // printIntVector(routes_for_tests[route2].sequence);

                      //  std::cout<<i<<" "<<j<<std::endl;
                        auto change_effect1= route_feasible_and_cost_cached (customers,0, distances, routes_for_tests[route1].sequence, cost_cache);   //za mało argumentów
                        auto change_effect2= route_feasible_and_cost_cached(customers,0, distances, routes_for_tests[route2].sequence, cost_cache);
                        auto original_cost1= route_feasible_and_cost_cached(customers,0, distances, actual_solution[route1].sequence, cost_cache);
                        auto original_cost2= route_feasible_and_cost_cached(customers,0, distances, actual_solution[route2].sequence, cost_cache);

                        //std::cout<<routes_for_tests[route1].load<<" "<<routes_for_tests[route2].load<<std::endl;
                        
                       // std::cout << change_effect1.second<<" "<<change_effect2.second<<" "<<original_cost1.second<<" "<<original_cost2.second<<std::endl;
                       
                        //whether move possible
                        if(change_effect1.first && change_effect2.first){
                            //checking the load
                            int client_index1 = actual_solution[route1].sequence[i];
                            int client_index2 = actual_solution[route2].sequence[j];
                            
                            int newload1=actual_solution[route1].load-customers[client_index1].demand + customers[client_index2].demand;
                            int newload2=actual_solution[route2].load+customers[client_index1].demand - customers[client_index2].demand;
                            //counting cost delta and adding move to the list
                            if(newload1>=0 &&newload1<=capacity && newload2>=0 &&newload2<=capacity){
                                double cost = (change_effect1.second + change_effect2.second) - (original_cost1.second + original_cost2.second); ;
                                list_of_moves.push_back(Move("swap",i, route1,j,route2, cost));
                                moves_counter++;
                                if(moves_counter>=MAX_MOVES){moves_limit=true; break;}
                            }
                        }
                        std::swap(routes_for_tests[route1].sequence[i],routes_for_tests[route2].sequence[j]);
                        //insertion move
                        
                        //delete erase the client from one route1 and add to route2
                        int client_removed=actual_solution[route1].sequence[i]; 
                        routes_for_tests[route2].sequence.insert(routes_for_tests[route2].sequence.begin() + j, client_removed);
                        routes_for_tests[route1].sequence.erase(routes_for_tests[route1].sequence.begin() + i);
                        

                        change_effect1= route_feasible_and_cost_cached(customers,0, distances, routes_for_tests[route1].sequence,cost_cache);
                        change_effect2= route_feasible_and_cost_cached(customers,0, distances, routes_for_tests[route2].sequence,cost_cache);
                        
                        original_cost1= route_feasible_and_cost_cached(customers,0, distances, actual_solution[route1].sequence, cost_cache);
                        original_cost2= route_feasible_and_cost_cached(customers,0, distances, actual_solution[route2].sequence, cost_cache);
                      //  printIntVector(routes_for_tests[route1].sequence);
                     //   printIntVector(routes_for_tests[route2].sequence);
                        
                        //std::cout << change_effect1.second<<" "<<change_effect2.second<<" "<<original_cost1.second<<" "<<original_cost2.second<<std::endl;

                        //whether move possible
                        
                        if(change_effect1.first && change_effect2.first){
                            int client_index = actual_solution[route1].sequence[i];
                            
                            int newload1=actual_solution[route1].load-customers[client_index].demand;
                            int newload2=actual_solution[route2].load+customers[client_index].demand;
                            if(newload1>=0 && newload2<=capacity){
                                double cost =(change_effect1.second + change_effect2.second) - (original_cost1.second + original_cost2.second);
                                list_of_moves.push_back(Move("insert",i, route1,j,route2, cost));
                             //   std::cout<<cost<<" "<< act_cost<< " " << totalCostCount(routes_for_tests,customers,distances)<<std::endl;
                                moves_counter++;
                                if(moves_counter>=MAX_MOVES){moves_limit=true; break;}
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
        //if move wasn't found 

        // chosing the best move 
        sort(list_of_moves.begin(), list_of_moves.end(), comparing_moves);
        if(list_of_moves.size()>1000){
            list_of_moves.resize(1000);
        }
      //  printListOfMoves(list_of_moves);
        //gdy nie ma ruchow
        if(list_of_moves.empty()){ 
            break;
        }
        Move chosen;
        bool found=false;
        for(auto i: list_of_moves){
            if(!Tabu.count(i)){ 
                found=true;
                Tabu.insert(i); 
                chosen=i;
                if (Tabu.size() > 100) Tabu.erase(Tabu.begin()); 
                break;
            }
        }
        //gdy nie ma ruchow z poza tabu
        if (!found) {
            if (!list_of_moves.empty()) {
            chosen = list_of_moves[0];
            Tabu.insert(chosen);
            if (Tabu.size() > 100) Tabu.erase(Tabu.begin());
        } else {
            break;
        }
}



      //  std::cout<< chosen.route1<<" "<<chosen.a<<" "<<chosen.route2<<" "<<chosen.b<<std::endl;
       // printSolution(actual_solution);
        //creating actual solution
        if(chosen.type=="swap"){
             
            //if load is right
             int client_index1 = actual_solution[chosen.route1].sequence[chosen.a];
             int client_index2 = actual_solution[chosen.route2].sequence[chosen.b];
             actual_solution[chosen.route1].load=actual_solution[chosen.route1].load-customers[client_index1].demand + customers[client_index2].demand;
             actual_solution[chosen.route2].load=actual_solution[chosen.route2].load+customers[client_index1].demand - customers[client_index2].demand;
        
            //swaping move
            std::swap(actual_solution[chosen.route1].sequence[chosen.a],actual_solution[chosen.route2].sequence[chosen.b]);
        
        }
        else {
            int client_id_to_move = actual_solution[chosen.route1].sequence[chosen.a];
            int client_demand = customers[client_id_to_move].demand;
            //update of routes
            actual_solution[chosen.route2].sequence.insert(actual_solution[chosen.route2].sequence.begin() + chosen.b, actual_solution[chosen.route1].sequence[chosen.a]);  
            actual_solution[chosen.route1].sequence.erase(actual_solution[chosen.route1].sequence.begin() + chosen.a);
            //update of loads
            actual_solution[chosen.route1].load=actual_solution[chosen.route1].load-client_demand;
            actual_solution[chosen.route2].load=actual_solution[chosen.route2].load+client_demand;
        }

        act_cost=totalCostCount(actual_solution,customers,distances);
        if(act_cost<best_cost){ 
            best_solution=actual_solution;
            best_cost=act_cost;
        }
        
      //  printSolution(actual_solution);
       // printSolution(best_solution);
     //  std::cout<<totalCostCount(routes,customers,distances)<<std::endl;
        std:: cout<<totalCostCount(actual_solution, customers,distances)<<" "<<totalCostCount(best_solution,customers,distances)<<std::endl;    
        list_of_moves.clear();
        routes_for_tests.clear();
        cost_cache.clear();
    }
    //tabu search end

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