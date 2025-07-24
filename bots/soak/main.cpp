#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <stack>

using namespace std;

/**
 * Win the water fight by controlling the most territory, or out-soak your opponent!
 **/

const int THROW_RANGE = 5;
const int THROW_DAMAGE = 30;

int manhattan(const int x1, const int y1, const int x2, const int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

double euclidean(const int x1, const int y1, const int x2, const int y2)
{
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}


class GAME;
class AGENT;

enum Action{MOVE, SHOOT, THROW};

struct ActionParams
{
    Action action{};
    int x = 0;
    int y = 0;
    int def_ag = 0;
    int atk_ag = 0;
};

class GAME
{
public:
    int my_id{};
    int width{};
    int height{};
    map<int, AGENT> agents{};
    vector<vector<int>> grid{};
    vector<vector<int>> ag_grid{};

    GAME(int my_id, int width, int height, map<int, AGENT> agents, vector<vector<int>> grid);
    double apply(Action action, const ActionParams& params, bool test); //return damage delt or distance traveled
    double evaluate();
    vector<vector<int>> make_snapshot();

};

class AGENT
{
public:
    int player{}; // Player id of this agent
    int shoot_cooldown{}; // Number of turns between each of this agent's shots
    int optimal_range{}; // Maximum manhattan distance for greatest damage output
    int soaking_power{}; // Damage output within optimal conditions
    int splash_bombs{}; // Number of splash bombs this can throw this game
    int cooldown{}; // Number of turns before this agent can shoot
    int wetness{}; // Damage (0-100) this agent has taken
    int x{};
    int y{};

    AGENT() = default;
    AGENT(int player, int shoot_cooldown, int optimal_range, int soaking_power, int splash_bombs);
    void update(int x, int y, int cooldown, int splash_bombs, int wetness);

};



//--------------------------------------------AGENT-----------------------------------------------

AGENT::AGENT(int player, int shoot_cooldown, int optimal_range, int soaking_power, int splash_bombs) :
            player(player), shoot_cooldown(shoot_cooldown), optimal_range(optimal_range), soaking_power(soaking_power),
            splash_bombs(splash_bombs)
{
}
//-----------------------------------------------------------------------------------------------

//-------------------------------------------GAME------------------------------------------------

GAME::GAME(int my_id, int width, int height, map<int, AGENT> agents, vector<vector<int>> grid) : my_id(my_id), width(width), height(height), agents(agents), grid(grid)
{
    ag_grid = vector<vector<int>>(height, vector<int>(width, 0));
    for (auto ag : agents)
    {
        ag_grid[ag.second.y][ag.second.x] = ag.first;
    }
}


double GAME::apply(Action action, const ActionParams& params, bool test = false)
{
    const int def_x = agents[params.def_ag].x;
    const int def_y = agents[params.def_ag].y;
    const int atk_x = agents[params.atk_ag].x;
    const int atk_y = agents[params.atk_ag].y;
    double tot_damage = agents[params.atk_ag].soaking_power;
    double dest = 0;
    double damage = 0;
    switch (action)
    {
    case MOVE:
        if ((ag_grid[params.y][params.x] != 0 || grid[params.y][params.x] != 0) && manhattan(params.x, params.y, atk_x, atk_y) <= 1)
            return 0;
        dest =  manhattan(params.x, params.y, atk_x, atk_y);
        if (test) return dest;
        agents[params.atk_ag].x = params.x;
        agents[params.atk_ag].y = params.y;
        return dest;
    case SHOOT:
        damage = tot_damage;
        dest  = euclidean(def_x, def_y, atk_x, atk_y);
        if (dest > agents[params.atk_ag].optimal_range)
            return 0;
        if (def_x + 1 < atk_x && grid[def_y][def_x + 1] != 0 && euclidean(def_x + 1, def_y, atk_x, atk_y) > sqrt(2))
        {
            damage = min(tot_damage - ((grid[def_y][def_x + 1] + 1) / 4.0) * tot_damage, damage);
        }
        if (def_y + 1 < atk_y && grid[def_y + 1][def_x]!= 0 && euclidean(def_x , def_y + 1, atk_x, atk_y) > sqrt(2))
        {
            damage = min(tot_damage - ((grid[def_y + 1][def_x] + 1) / 4.0) * tot_damage, damage );
        }
        if (def_x - 1 > atk_x && grid[def_y][def_x - 1] != 0 && euclidean(def_x - 1, def_y, atk_x, atk_y) > sqrt(2))
        {
            damage = min(tot_damage - ((grid[def_y][def_x - 1] + 1) / 4.0) * tot_damage, damage);
        }
        if (def_y - 1 > atk_y && grid[def_y - 1][def_x] != 0 && euclidean(def_x, def_y - 1, atk_x, atk_y) > sqrt(2))
        {
            damage = min(tot_damage - ((grid[def_y - 1][def_x] + 1) / 4.0) * tot_damage, damage);
        }
        if (test) return damage;
        agents[params.atk_ag].wetness += static_cast<int>(damage);
        return damage;
    case THROW:
        dest = euclidean(atk_x, atk_y, params.x, params.y);
        // if (dest > THROW_RANGE)
            // return 0;
        vector<pair<int, int>> adjacent_points;
        adjacent_points.emplace_back(params.y, params.x);
        // Orthogonal points
        if (params.x + 1 < width) adjacent_points.emplace_back(params.y, params.x + 1);
        if (params.x - 1 >= 0) adjacent_points.emplace_back(params.y, params.x - 1);
        if (params.y + 1 < height) adjacent_points.emplace_back(params.y + 1, params.x);
        if (params.y - 1 >= 0) adjacent_points.emplace_back(params.y - 1, params.x);
        // Diagonal points
        if (params.x + 1 < width && params.y + 1 < height) adjacent_points.emplace_back(params.y + 1, params.x + 1);
        if (params.x + 1 < width && params.y - 1 >= 0) adjacent_points.emplace_back(params.y - 1, params.x + 1);
        if (params.x - 1 >= 0 && params.y + 1 < height) adjacent_points.emplace_back(params.y + 1, params.x - 1);
        if (params.x - 1 >= 0 && params.y - 1 >= 0) adjacent_points.emplace_back(params.y - 1, params.x - 1);

        for (auto point : adjacent_points)
        {
            if (ag_grid[point.first][point.second] != 0)
            {
                if(agents[ag_grid[point.first][point.second]].player == my_id)
                    return 0;
                damage += min(THROW_DAMAGE, 100 - agents[ag_grid[point.first][point.second]].wetness);
                if (!test)
                    agents[ag_grid[point.first][point.second]].wetness += THROW_DAMAGE;
            }
        }
        return damage;
    }
    return 0;
}

double GAME::evaluate()
{
    return 0;
}

vector<vector<int>> GAME::make_snapshot()
{
    vector<vector<int>> snapshot;
    for (auto agent : agents)
    {
        snapshot.push_back({agent.second.x, agent.second.y, agent.second.cooldown == 0 ? 1 : 0, agent.second.splash_bombs > 0 ? 1 : 0, agent.second.player == my_id ? 1 : 0});
    }
    sort(snapshot.begin(), snapshot.end());
}

//-----------------------------------------------------------------------------------------------
//-------------------------------------------Q-learning---------------------------------------------

class Q_learning
{
    public:

};

//-----------------------------------------------------------------------------------------------

void AGENT::update(const int x, const int y, const int cooldown, const int splash_bombs, const int wetness)
{
    this->x = x;
    this->y = y;
    this->cooldown = cooldown;
    this->splash_bombs = splash_bombs;
    this->wetness = wetness;
}



int main()
{
    int my_id; // Your player id (0 or 1)
    cin >> my_id; cin.ignore();
    cerr << my_id << endl;
    int op_id = (my_id == 0) ? 1 : 0; // The player you are playing against
    int agent_data_count; // Total number of agents in the game
    cin >> agent_data_count; cin.ignore();
    cerr << agent_data_count << endl;
    map<int, AGENT> agents;
    map<int, vector<int>> players;
    for (int i = 0; i < agent_data_count; i++) {
        int agent_id; // Unique identifier for this agent
        int player; // Player id of this agent
        int shoot_cooldown; // Number of turns between each of this agent's shots
        int optimal_range; // Maximum manhattan distance for greatest damage output
        int soaking_power; // Damage output within optimal conditions
        int splash_bombs; // Number of splash bombs this can throw this game
        cin >> agent_id >> player >> shoot_cooldown >> optimal_range >> soaking_power >> splash_bombs; cin.ignore();
        cerr << agent_id << " " << player << " " << shoot_cooldown << " " << optimal_range << " " << soaking_power <<
            " " << splash_bombs << endl;
        const AGENT agent(player, shoot_cooldown, optimal_range, soaking_power, splash_bombs);
        agents[agent_id] = agent;
        if (player == my_id) players[my_id].push_back(agent_id);
        else players[op_id].push_back(agent_id);
    }
    int width; // Width of the game map
    int height; // Height of the game map
    cin >> width >> height; cin.ignore();
    cerr << width << " " << height << endl;
    vector<vector<int>> grid(height, vector<int>(width));
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int x; // X coordinate, 0 is left edge
            int y; // Y coordinate, 0 is top edge
            int tile_type;
            cin >> x >> y >> tile_type; cin.ignore();
            cerr << x << " " << y << " " << tile_type << endl;
            grid[y][x] = tile_type;
        }
    }

    // game loop
    while (1) {
        int agent_count; // Total number of agents still in the game
        cin >> agent_count; cin.ignore();
        cerr << agent_count << endl;
        map<int, AGENT> agents_tmp;
        players[my_id].clear();
        players[op_id].clear();
        for (int i = 0; i < agent_count; i++) {
            int agent_id;
            int x;
            int y;
            int cooldown; // Number of turns before this agent can shoot
            int splash_bombs;
            int wetness; // Damage (0-100) this agent has taken
            cin >> agent_id >> x >> y >> cooldown >> splash_bombs >> wetness; cin.ignore();
            cerr << agent_id << " " << x << " " << y << " " << cooldown << " " << splash_bombs << " " << wetness <<
                endl;
            agents_tmp[agent_id] = agents[agent_id];
            agents_tmp[agent_id].update(x, y, cooldown, splash_bombs, wetness);
            if (agents[agent_id].player == my_id) players[my_id].push_back(agent_id);
            else players[op_id].push_back(agent_id);
        }
        agents = agents_tmp;
        GAME game(my_id, width, height, agents, grid);
        int my_agent_count; // Number of alive agents controlled by you
        cin >> my_agent_count; cin.ignore();
        cerr << my_agent_count << endl;
        for (int id : players[my_id])
        {
            if (agents[id].splash_bombs == 0 && game.apply(SHOOT, {.def_ag=players[op_id][0], .atk_ag=id}, true)){
                cout << id << "; SHOOT "  << players[op_id][0] << endl;
            }
            else{
            double max_damage = 0;
            double tmp = 0;
            int tmp_x = agents[id].x;
            int tmp_y = agents[id].y;
            pair<int, int> target{};
            for (int j = 0; j < height; j++)
            {
                for (int i = 0; i < width; i++)
                {
                    agents[id].x = i;
                    agents[id].y = j;
                    tmp = game.apply(THROW, ActionParams{.x=i, .y=j, .atk_ag=id}, true);
                    if (tmp > max_damage)
                    {
                        max_damage = tmp;
                        target = make_pair(i, j);
                    }
                }
            }
            agents[id].x = tmp_x;
            agents[id].y = tmp_y;

            if (manhattan(target.first, target.second, agents[id].x, agents[id].y) <= THROW_RANGE + 1)
                cout << id << "; MOVE " << target.first << " " << target.second << "; THROW " << target.first << " " << target.second << endl;
            else
                cout << "MOVE " << target.first << " " << target.second << endl;
            }
        }
    }
}