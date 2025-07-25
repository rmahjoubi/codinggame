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

enum Action { MOVE, SHOOT, THROW, HUNKER };

enum Direction { UP=1, DOWN, LEFT, RIGHT };

struct ActionParams
{
	Action action{};
	Direction direction{};
	int x = 0;
	int y = 0;
	int def_ag = 0;
	int atk_ag = 0;
};

class GAME
{
public:
	int my_id{};
	int op_id{};
	int width{};
	int height{};
	int turn{};
	map<int, AGENT> agents{};
	vector<int> my_agents{};
	vector<int> op_agents{};
	vector<vector<int>> grid{};
	vector<vector<int>> ag_grid{};

	GAME(int my_id, int width, int height, map<int, AGENT> agents, vector<vector<int>> grid, int turn);
	double apply(const ActionParams& params); //return damage delt or distance traveled
	double evaluate();
	vector<vector<int>> make_snapshot();
	deque<vector<ActionParams>> generate_move_actions_for(int player);
	void execute_move_actions(vector<ActionParams> actions);

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
	bool is_hunkered{};

	AGENT() = default;
	AGENT(int player, int shoot_cooldown, int optimal_range, int soaking_power, int splash_bombs);
	void update(int x, int y, int cooldown, int splash_bombs, int wetness);
};


//--------------------------------------------AGENT-----------------------------------------------

AGENT::AGENT(int player, int shoot_cooldown, int optimal_range, int soaking_power, int splash_bombs) :
	player(player), shoot_cooldown(shoot_cooldown), optimal_range(optimal_range), soaking_power(soaking_power),
	splash_bombs(splash_bombs)
{
	is_hunkered = false;
}

//-----------------------------------------------------------------------------------------------

//-------------------------------------------GAME------------------------------------------------

GAME::GAME(int my_id, int width, int height, map<int, AGENT> agents, vector<vector<int>> grid, int turn) : my_id(my_id),
	width(width), height(height), agents(agents), grid(grid), turn(turn)
{
	ag_grid = vector<vector<int>>(height, vector<int>(width, 0));
	for (auto ag : agents)
	{
		ag_grid[ag.second.y][ag.second.x] = ag.first;
	}
	for (auto ag : agents)
	{
		if (ag.second.player == my_id)
			my_agents.push_back(ag.first);
		else
			op_agents.push_back(ag.first);
	}
	op_id = my_id + 1;
}


double GAME::apply(const ActionParams& params)
{
	Action action = params.action;
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
		switch (params.direction)
		{
			pair<int,int> dest_point{};
			case UP:
				if (params.y - 1 >= 0 && grid[params.y - 1][params.x] == 0)
					dest_point = make_pair(params.y - 1, params.x);
				else return 0;
				break;
			case DOWN:
				if (params.y + 1 < height && grid[params.y + 1][params.x] == 0)
					dest_point = make_pair(params.y + 1, params.x);
				else return 0;
				break;
			case LEFT:
				if (params.x - 1 >= 0 && grid[params.y][params.x - 1] == 0)
					dest_point = make_pair(params.y, params.x - 1);
				else return 0;
				break;
			case RIGHT:
				if (params.x + 1 < width && grid[params.y][params.x + 1] == 0)
					dest_point = make_pair(params.y, params.x + 1);
				else return 0;
				break;
			grid[agents[params.atk_ag].y][agents[params.atk_ag].x] = 0;
			agents[params.atk_ag].y = dest_point.first;
			agents[params.atk_ag].x = dest_point.second;
			grid[agents[params.atk_ag].y][agents[params.atk_ag].x] = params.atk_ag;
			return 1;
		}
	case SHOOT:
		damage = tot_damage;
		dest = manhattan(def_x, def_y, atk_x, atk_y);
		int d = 1;
		if (dest > agents[params.atk_ag].optimal_range * 2)
			return 0;
		if (def_x + 1 < atk_x && grid[def_y][def_x + 1] != 0 && euclidean(def_x + 1, def_y, atk_x, atk_y) > sqrt(2))
		{
			damage = min(tot_damage - ((grid[def_y][def_x + 1] + 1) / 4.0) * tot_damage, damage);
		}
		if (def_y + 1 < atk_y && grid[def_y + 1][def_x] != 0 && euclidean(def_x, def_y + 1, atk_x, atk_y) > sqrt(2))
		{
			damage = min(tot_damage - ((grid[def_y + 1][def_x] + 1) / 4.0) * tot_damage, damage);
		}
		if (def_x - 1 > atk_x && grid[def_y][def_x - 1] != 0 && euclidean(def_x - 1, def_y, atk_x, atk_y) > sqrt(2))
		{
			damage = min(tot_damage - ((grid[def_y][def_x - 1] + 1) / 4.0) * tot_damage, damage);
		}
		if (def_y - 1 > atk_y && grid[def_y - 1][def_x] != 0 && euclidean(def_x, def_y - 1, atk_x, atk_y) > sqrt(2))
		{
			damage = min(tot_damage - ((grid[def_y - 1][def_x] + 1) / 4.0) * tot_damage, damage);
		}
		if (dest > agents[params.atk_ag].optimal_range)
			damage /= 2.0;
		if (agents[params.def_ag].is_hunkered)
			damage -= 1/4.0 * damage;
		agents[params.atk_ag].wetness += static_cast<int>(damage);
		return damage;
	case THROW:
		dest = manhattan(atk_x, atk_y, params.x, params.y);
		if (dest > THROW_RANGE) //check this bug
			return 0;
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
				damage += min(THROW_DAMAGE, 100 - agents[ag_grid[point.first][point.second]].wetness);
				agents[ag_grid[point.first][point.second]].wetness += THROW_DAMAGE;
			}
		}
		return damage;
	case HUNKER:
		agents[params.atk_ag].is_hunkered = true;
		return 0;
	}
	return 0;
}

deque<vector<ActionParams>> GAME::generate_move_actions_for(int player)
{
	vector<int> l_agents = player == my_id ? my_agents : op_agents;
	deque<vector<ActionParams>> actions;
	deque<vector<ActionParams>> tmp_actions = {{}};
	for (auto ag : l_agents)
	{
		for (const auto& tmp_combo : tmp_actions)
		{
			vector<ActionParams> combo;
			if (agents[ag].y - 1 >= 0 && grid[agents[ag].y - 1][agents[ag].x] == 0)
			{
				combo = tmp_combo;
				combo.emplace_back(ActionParams{ .action=MOVE, .direction=UP, .atk_ag=ag, .x=agents[ag].x, .y=agents[ag].y});
				actions.emplace_back(combo);
			}
			if (agents[ag].y + 1 < height && grid[agents[ag].y + 1][agents[ag].x] == 0)
			{
				combo = tmp_combo;
				combo.emplace_back(ActionParams{ .action=MOVE, .direction=DOWN, .atk_ag=ag, .x=agents[ag].x, .y=agents[ag].y } );
				actions.emplace_back(combo);
			}
			if (agents[ag].x - 1 >= 0 && grid[agents[ag].y][agents[ag].x - 1] == 0)
			{
				combo = tmp_combo;
				combo.emplace_back(ActionParams{ .action=MOVE, .direction=LEFT, .atk_ag=ag, .x=agents[ag].x, .y=agents[ag].y } );
				actions.emplace_back(combo);
			}
			if (agents[ag].x + 1 < width && grid[agents[ag].y][agents[ag].x + 1] == 0)
			{
				combo = tmp_combo;
				combo.emplace_back(ActionParams{ .action=MOVE, .direction=RIGHT, .atk_ag=ag, .x=agents[ag].x, .y=agents[ag].y } );
				actions.emplace_back(combo);
			}
		}
		tmp_actions = actions;
	}
	return actions;
}


void GAME::execute_move_actions(vector<ActionParams> actions)
{
	map<int, ActionParams> moved{};
	vector<int> ag_undo_actions;
	for (auto ac : actions)
	{
		int success = static_cast<int>(apply(ac));
		if (!success) // agents collide
		{
			pair<int,int> dest_point{};
			switch (ac.direction)
			{
			case UP:
				dest_point = make_pair(agents[ac.atk_ag].y - 1, agents[ac.atk_ag].x);
				break;
			case DOWN:
				dest_point = make_pair(agents[ac.atk_ag].y + 1, agents[ac.atk_ag].x);
				break;
			case LEFT:
				dest_point = make_pair(agents[ac.atk_ag].y, agents[ac.atk_ag].x - 1);
				break;
			case RIGHT:
				dest_point = make_pair(agents[ac.atk_ag].y, agents[ac.atk_ag].x + 1);
				break;
			}
			if (moved.contains(ag_grid[dest_point.first][dest_point.second]))
			{
				ag_undo_actions.push_back(ag_grid[dest_point.first][dest_point.second]);
			}
		}
		else
		{
			moved[ac.atk_ag] = ac;
		}
	}
	// undo actions
	for (auto ag : ag_undo_actions)
	{
		ag_grid[agents[ag].y][agents[ag].x] = 0;
		agents[ag].y = moved[ag].y;
		agents[ag].x = moved[ag].x;
		ag_grid[agents[ag].y][agents[ag].x] = ag;
	}
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
		snapshot.push_back({
			agent.second.x, agent.second.y, agent.second.cooldown == 0 ? 1 : 0, agent.second.splash_bombs > 0 ? 1 : 0,
			agent.second.player == my_id ? 1 : 0
		});
	}
	sort(snapshot.begin(), snapshot.end());
}

//-----------------------------------------------------------------------------------------------
//-------------------------------------------Q-learning---------------------------------------------

void q_learn(GAME game)
{
	map<vector<vector<int>>, deque<vector<ActionParams>>> Q;
	vector<vector<int>> init_state = game.make_snapshot();
	Q[init_state] = game.generate_move_actions_for(game.my_id);

	for (auto ac : Q[init_state])
	{

	}
}

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
	cin >> my_id;
	cin.ignore();
	cerr << my_id << endl;
	int op_id = (my_id == 0) ? 1 : 0; // The player you are playing against
	int agent_data_count; // Total number of agents in the game
	cin >> agent_data_count;
	cin.ignore();
	cerr << agent_data_count << endl;
	map<int, AGENT> agents;
	map<int, vector<int>> players;
	for (int i = 0; i < agent_data_count; i++)
	{
		int agent_id; // Unique identifier for this agent
		int player; // Player id of this agent
		int shoot_cooldown; // Number of turns between each of this agent's shots
		int optimal_range; // Maximum manhattan distance for greatest damage output
		int soaking_power; // Damage output within optimal conditions
		int splash_bombs; // Number of splash bombs this can throw this game
		cin >> agent_id >> player >> shoot_cooldown >> optimal_range >> soaking_power >> splash_bombs;
		cin.ignore();
		cerr << agent_id << " " << player << " " << shoot_cooldown << " " << optimal_range << " " << soaking_power <<
			" " << splash_bombs << endl;
		const AGENT agent(player, shoot_cooldown, optimal_range, soaking_power, splash_bombs);
		agents[agent_id] = agent;
		if (player == my_id) players[my_id].push_back(agent_id);
		else players[op_id].push_back(agent_id);
	}
	int width; // Width of the game map
	int height; // Height of the game map
	cin >> width >> height;
	cin.ignore();
	cerr << width << " " << height << endl;
	vector<vector<int>> grid(height, vector<int>(width));
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int x; // X coordinate, 0 is left edge
			int y; // Y coordinate, 0 is top edge
			int tile_type;
			cin >> x >> y >> tile_type;
			cin.ignore();
			cerr << x << " " << y << " " << tile_type << endl;
			grid[y][x] = tile_type;
		}
	}

	// game loop
	while (1)
	{
		int agent_count; // Total number of agents still in the game
		cin >> agent_count;
		cin.ignore();
		cerr << agent_count << endl;
		map<int, AGENT> agents_tmp;
		players[my_id].clear();
		players[op_id].clear();
		for (int i = 0; i < agent_count; i++)
		{
			int agent_id;
			int x;
			int y;
			int cooldown; // Number of turns before this agent can shoot
			int splash_bombs;
			int wetness; // Damage (0-100) this agent has taken
			cin >> agent_id >> x >> y >> cooldown >> splash_bombs >> wetness;
			cin.ignore();
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
		cin >> my_agent_count;
		cin.ignore();
		cerr << my_agent_count << endl;
		for (int id : players[my_id])
		{
		}
	}
}
