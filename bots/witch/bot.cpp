#include <iostream>
#include <bits/stdc++.h>
#include <string>
#include <vector>
#include<map>
#include<set>
#include <algorithm>
#include <ctime>


#include "MonteCarloTreeSearch/mcts/include/state.h"
#include "MonteCarloTreeSearch/mcts/include/mcts.h"


using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

enum Action { BREW, CAST, REST, LEARN, WAIT };

map<string, Action> action_map = {
	{"BREW", BREW},
	{"CAST", CAST},
	{"REST", REST},
	{"LEARN", LEARN},
	{"WAIT", WAIT},
};


class ACTION;
class INV;
class GAME;
class TOME;

map<int, ACTION> actions_pool;
unordered_set<int> spells_ids;

class INV
{
public:
	int inv_0{}; // tier-0 ingredients in inventory
	int inv_1{};
	int inv_2{};
	int inv_3{};
	int score{}; // amount of rupees
	vector<int> state{};
	// methods
	INV() = default;
	INV(int inv_0, int inv_1, int inv_2, int inv_3, int score);
	bool is_full();
};

class TOME
{
public:
	vector<ACTION> spells;

	TOME(vector<ACTION> spells) : spells(spells)
	{
	};
};

class GAME : public MCTS_state
{
public:
	int turn_count{};
	INV inv;
	TOME tome;
	unordered_set<int> spells{};
	vector<ACTION> potions;
	int last_action_id{-2};
    deque<ACTION> possible_actions{};

	//methods
	GAME(unordered_set<int> spells, vector<ACTION> potions, INV inv, TOME tome, int turn_count);
	GAME(const GAME& other);
    void update_possible_actions();
	bool can_action(int id) const;
	void perform_action(const ACTION& a);
	bool is_terminal() const override;
	MCTS_state* next_state(const MCTS_move* move) const override;
	queue<MCTS_move*>* actions_to_try() const override;
	double rollout() const override;
	bool player1_turn() const override {return true;}
};

class ACTION : public MCTS_move
{
public:
	int action_id{}; // the unique ID of this spell or recipe
	string s_action_type{}; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW, REST
	Action action_type{};
	int delta_0{}; // tier-0 ingredient change
	int delta_1{}; // tier-1 ingredient change
	int delta_2{}; // tier-2 ingredient change
	int delta_3{}; // tier-3 ingredient change
	int price{}; // the price in rupees if this is a potion
	int tome_index{};
	// in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax; For brews, this is the value of the current urgency bonus
	int tax_count{};
	// in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell; For brews, this is how many times you can still gain an urgency bonus
	bool castable{}; // in the first league: always 0; later: 1 if this is a castable player spell
	bool repeatable{}; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell
	int urgency{};

	ACTION(): action_id(-1), action_type(REST), s_action_type("REST")
	{
	};
	ACTION(int action_id, const string& s_action_type, int delta_0, int delta_1, int delta_2, int delta_3,
	       int price, int tome_index, bool castable, bool repeatable);
	bool operator==(const MCTS_move& other) const;
};

//----------------------------GAME METHODS ---------------------------------
GAME::GAME(unordered_set<int> spells, vector<ACTION> potions, INV inv, TOME tome, int turn_count) : spells(spells), potions(potions), turn_count(turn_count),
	inv(inv), tome(tome)
{
    update_possible_actions();
}

GAME::GAME(const GAME& other) : spells(other.spells), potions(other.potions), inv(other.inv), tome(other.tome),
                                turn_count(other.turn_count)
{
}

bool GAME::is_terminal() const
{
	return turn_count >= 100;
}

MCTS_state* GAME::next_state(const MCTS_move* move) const
{
	ACTION *m = (ACTION*) move;
	GAME *new_state = new GAME(*this);
	if (can_action(m->action_id))
	{
		new_state->perform_action(*m);
	}
	else
	{
		throw runtime_error("Action cannot be performed due to invalid conditions.");
	}
	return new_state;
}

queue<MCTS_move*>* GAME::actions_to_try() const
{
	queue<MCTS_move*>* Q = new queue<MCTS_move*>();
	for (auto& s : tome.spells)
	{
		if (can_action(s.action_id))
		{
			Q->push(new ACTION(s));
		}
	}
	for (auto& p : potions)
	{
		if (can_action(p.action_id))
		{
			Q->push(new ACTION(p));
		}
	}
	for (int s : spells)
	{
		if (can_action(s))
		{
			Q->push(new ACTION(actions_pool[s]));
		}
	}
	Q->push(new ACTION(actions_pool[-1]));
	return Q;
}

double GAME::rollout() const
{
	const int max_reward = 23 * 100;
	if (is_terminal()) return static_cast<double>(inv.score) / max_reward;

	long long r;
    ACTION a;
    GAME a_game(*this);
	srand(time(NULL));
    do{
        if (a_game.possible_actions.empty())
		{
			throw runtime_error("run out of available moves and state is not terminal?.");
	    } 
		r = rand() % a_game.possible_actions.size();
        const ACTION& a = possible_actions[r];
        a_game.perform_action(a);
    } while(!is_terminal());

	double res = a_game.inv.score / max_reward;
    return res;
}

void GAME::update_possible_actions(){
    for (auto s : tome.spells)
	    if (can_action(s.action_id))
			possible_actions.push_front(s);
	for (auto p : potions)
		if (can_action(p.action_id))
			possible_actions.push_front(p);
	for (int s : spells)
		if (can_action(s))
			possible_actions.push_front(actions_pool[s]);
	possible_actions.push_front(actions_pool[-1]);
}

bool GAME::can_action(int id) const
{
	ACTION& a = actions_pool[id];
	switch (a.action_type)
	{
	case BREW:
		if (inv.inv_0 >= -a.delta_0 && inv.inv_1 >= -a.delta_1 && inv.inv_2 >= -a.delta_2 && inv.inv_3 >= -a.delta_3 &&
			potions.size() > 0)
			return true;
		break;
	case CAST:
		if (inv.inv_0 >= -a.delta_0 && inv.inv_1 >= -a.delta_1 && inv.inv_2 >= -a.delta_2 && inv.inv_3 >= -a.delta_3)
			return true;
		break;
	case LEARN:
		if (inv.inv_0 >= a.tome_index && tome.spells.size() > 0)
			return true;
		break;
	case REST:
		if (spells.size() < spells_ids.size())
			return true;
		break;
	case WAIT:
		return true;
		break;
	}
	return false;
}


void GAME::perform_action(const ACTION& a)
{
	if (!can_action(a.action_id))
		throw runtime_error("Action cannot be performed due to invalid conditions.");
	last_action_id = a.action_id;
	int pos = -1;
	switch (a.action_type)
	{
	case BREW:
		inv.score += a.price;
	//urgency bonus
		if (a.tax_count > 0)
			inv.score += a.tome_index;

		inv.inv_0 += a.delta_0;
		inv.inv_1 += a.delta_1;
		inv.inv_2 += a.delta_2;
		inv.inv_3 += a.delta_3;
		inv.state = {inv.inv_0, inv.inv_1, inv.inv_2, inv.inv_3, inv.score};
	//potions.erase({a.delta_0, a.delta_1, a.delta_2, a.delta_3});
		if (a.tome_index > 1)
		{
			for (int i = 0; i < potions.size(); i++)
			{
				int id = potions[i].action_id;
				if (id == a.action_id)
				{
					if (i == 0)
					{
						int tmp_tx;
						int tmp_tome;
						if (potions.size() >= 2)
						{
							tmp_tx = potions[1].tax_count;
							tmp_tome = potions[1].tome_index;
							potions[1].tax_count = a.tax_count - 1;
							potions[1].tome_index = a.tome_index;
						}
						if (potions.size() >= 3)
						{
							potions[2].tax_count = tmp_tx;
							potions[2].tome_index = tmp_tome;
						}
					}
					else if (i == 1)
					{
						if (potions.size() >= 3)
						{
							potions[2].tax_count = a.tax_count - 1;
							potions[2].tome_index = a.tome_index;
						}
					}
					break;
				}
			}
		}
		pos = -1;
		for (int i = 0; i < potions.size(); i++)
			if (potions[i].action_id == a.action_id)
			{
				pos = i;
				break;
			}
		if (pos == -1)
			throw runtime_error("potion doesn't exist in potions_ids.");
		potions.erase(potions.begin() + pos);
		break;
	case CAST:
		inv.inv_0 += a.delta_0;
		inv.inv_1 += a.delta_1;
		inv.inv_2 += a.delta_2;
		inv.inv_3 += a.delta_3;
		inv.state = {inv.inv_0, inv.inv_1, inv.inv_2, inv.inv_3, inv.score};
		spells.erase(a.action_id);
		break;
	case LEARN:
		pos = -1;
		for (int i = 0; i < tome.spells.size(); i++)
			if (tome.spells[i].action_id == a.action_id)
				pos = i;
		if (pos == -1)
			throw runtime_error("spell doesn't exist in tome_spells.");
		tome.spells.erase(tome.spells.begin() + pos);
		for (int i = 0; i < tome.spells.size(); i++)
			tome.spells[i].tome_index = i;
		spells.insert(a.action_id);
		spells_ids.insert(a.action_id);
		actions_pool[a.action_id].s_action_type = "CAST";
		actions_pool[a.action_id].action_type = CAST;
		inv.inv_0 -= a.tome_index;
		break;
	case REST:
		spells = spells_ids;
		break;
	case WAIT:
		cerr << "whiy did you wait rest instead" << endl;
	}

    update_possible_actions();
	//cout << inv.inv_0 << " " << inv.inv_1 << " " << inv.inv_2 << " " << inv.inv_3 << endl;
}

//----------------------------INV METHODS ---------------------------------
INV::INV(int inv_0, int inv_1, int inv_2, int inv_3, int score) : inv_0(inv_0), inv_1(inv_1), inv_2(inv_2),
                                                                  inv_3(inv_3), score(score),
                                                                  state{inv_0, inv_1, inv_2, inv_3, score}
{
}

bool INV::is_full()
{
	if (inv_0 >= 0 + inv_1 >= 0 + inv_2 >= 0 + inv_3 >= 10)
		return true;
	else
		return false;
}

//----------------------------ACTIONS METHODS ---------------------------------

ACTION::ACTION(int action_id, const string& s_action_type, int delta_0, int delta_1, int delta_2, int delta_3,
               int price, int tome_index, bool castable, bool repeatable)
	: action_id(action_id),
	  s_action_type(s_action_type),
	  delta_0(delta_0),
	  delta_1(delta_1),
	  delta_2(delta_2),
	  delta_3(delta_3),
	  price(price),
	  tome_index(tome_index),
	  castable(castable),
	  repeatable(repeatable)
{
	action_type = action_map[s_action_type];
}

bool ACTION::operator==(const MCTS_move& other) const
{
	const ACTION &m = (const ACTION&) other;
	return action_id == m.action_id;
}


//-----------------------------------bfs------------------------------------

int get_action(map<vector<int>, vector<int>>& par, vector<int> final_action)
{
	vector<int> path;
	while (par.find(final_action) != par.end())
	{
		path.push_back(final_action[5]);
		final_action = par[final_action];
	}
	if (path.size() > 1)
		return path[path.size() - 1];
	else if (path.size() == 1)
		return path[0];
	else
		return -1;
}

ACTION bfs(GAME game)
{
	map<vector<int>, vector<int>> par;
	queue<GAME> q;
	q.push(game);
	map<vector<int>, bool> visited;
	visited[game.inv.state] = true;
	int current_score = game.inv.score;
	while (!q.empty())
	{
		GAME g = q.front();
		q.pop();
		for (int s : g.spells)
		{
			if (g.can_action(s))
			{
				GAME new_game = g;
				new_game.perform_action(actions_pool[s]);
				if (!visited[new_game.inv.state])
				{
					q.push(new_game);
					par[{
						new_game.inv.inv_0, new_game.inv.inv_1, new_game.inv.inv_2, new_game.inv.inv_3,
						new_game.inv.score, new_game.last_action_id
					}] = {g.inv.inv_0, g.inv.inv_1, g.inv.inv_2, g.inv.inv_3, g.inv.score, g.last_action_id};
					visited[new_game.inv.state] = true;
				}
			}
		}
		for (ACTION p : g.potions)
		{
			if (g.can_action(p.action_id))
			{
				GAME new_game = g;
				new_game.perform_action(p);
				par[{
					new_game.inv.inv_0, new_game.inv.inv_1, new_game.inv.inv_2, new_game.inv.inv_3, new_game.inv.score,
					new_game.last_action_id
				}] = {g.inv.inv_0, g.inv.inv_1, g.inv.inv_2, g.inv.inv_3, g.inv.score, g.last_action_id};
				int id = get_action(par, {
					                    new_game.inv.inv_0, new_game.inv.inv_1, new_game.inv.inv_2, new_game.inv.inv_3,
					                    new_game.inv.score, new_game.last_action_id
				                    });
				cerr << "brewing id:" << actions_pool[id].s_action_type << "id==" << id << endl;
				return actions_pool[id];
			}
		}
		if (g.spells.size() < spells_ids.size())
		{
			GAME new_game = g;
			new_game.perform_action(actions_pool[-1]);
			q.push(new_game);
			par[{
				new_game.inv.inv_0, new_game.inv.inv_1, new_game.inv.inv_2, new_game.inv.inv_3, new_game.inv.score,
				new_game.last_action_id
			}] = {g.inv.inv_0, g.inv.inv_1, g.inv.inv_2, g.inv.inv_3, g.inv.score, g.last_action_id};
			visited[new_game.inv.state] = true;
		}
		for (ACTION s : g.tome.spells)
		{
			if (g.can_action(s.action_id))
			{
				GAME new_game = g;
				new_game.perform_action(actions_pool[s.action_id]);
				if (!visited[new_game.inv.state])
				{
					q.push(new_game);
					par[{
						new_game.inv.inv_0, new_game.inv.inv_1, new_game.inv.inv_2, new_game.inv.inv_3,
						new_game.inv.score, new_game.last_action_id
					}] = {g.inv.inv_0, g.inv.inv_1, g.inv.inv_2, g.inv.inv_3, g.inv.score, g.last_action_id};
					visited[new_game.inv.state] = true;
				}
			}
		}
	}
	cerr << "exiting with res" << endl;
	ACTION action;
	return action;
}

//--------------------------------------------------------------------------

int main()
{
	//cout << "Hello World!" << endl;
	// game loop
	int turn = 0;
	int max_iter = 1000;
	int max_seconds = 3;

	while (1)
	{
		int action_count; // the number of spells and recipes in play
		cin >> action_count;
		cerr << action_count << "\n";
		unordered_set<int> spells;
		vector<ACTION> potions;
		vector<INV> invs;
		vector<ACTION> tmp;
		TOME tome(tmp);
		actions_pool.clear();
		spells_ids.clear();
		for (int i = 0; i < action_count; i++)
		{
			int action_id; // the unique ID of this spell or recipe
			string action_type; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
			int delta_0; // tier-0 ingredient change
			int delta_1; // tier-1 ingredient change
			int delta_2; // tier-2 ingredient change
			int delta_3; // tier-3 ingredient change
			int price; // the price in rupees if this is a potion
			int tome_index;
			// in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax; For brews, this is the value of the current urgency bonus
			int tax_count;
			// in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell; For brews, this is how many times you can still gain an urgency bonus
			bool castable; // in the first league: always 0; later: 1 if this is a castable player spell
			bool repeatable; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell
			cin >> action_id >> action_type >> delta_0 >> delta_1 >> delta_2 >> delta_3 >> price >> tome_index >>
				tax_count >> castable >> repeatable;
			cerr << action_id << " " << action_type << " " << delta_0 << " " << delta_1 << " " << delta_2 << " " <<
				delta_3 << " " << price << " " << tome_index << " " << tax_count << " " << castable << " " << repeatable
				<< endl;
			if (action_type == "CAST" || action_type == "BREW" || action_type == "LEARN")
			{
				ACTION action(action_id, action_type, delta_0, delta_1, delta_2, delta_3, price, tome_index, castable,
				              repeatable);
				actions_pool[action_id] = action;
				if (action.action_type == CAST)
					spells_ids.insert(action_id);
				if (action.action_type == CAST && castable)
					spells.insert(action_id);
				if (action.action_type == BREW)
					potions.push_back(action);
				if (action.action_type == LEARN)
				{
					tome.spells.push_back(action);
				}
			}
		}
		ACTION action;
		actions_pool[-1] = action;
		for (int i = 0; i < 2; i++)
		{
			int inv_0; // tier-0 ingredients in inventory
			int inv_1;
			int inv_2;
			int inv_3;
			int score; // amount of rupees
			cin >> inv_0 >> inv_1 >> inv_2 >> inv_3 >> score;
			cerr << inv_0 << " " << inv_1 << " " << inv_2 << " " << inv_3 << " " << score << endl;
			INV inv(inv_0, inv_1, inv_2, inv_3, score);
			invs.push_back(inv);
		}
		//GAME game(spells, potions, invs[0], tome);
		MCTS_state *state = new GAME(spells, potions, invs[0], tome, turn);
		MCTS_tree* tree = new MCTS_tree(state);
		tree->grow_tree(max_iter, max_seconds);
		MCTS_node *best_child = tree->select_best_child();
		if (best_child == NULL)
			throw runtime_error("No best child found from main.");
		const MCTS_move *best_move = best_child->get_move();
		const ACTION *res = dynamic_cast<const ACTION *>(best_move);

		if (!res)
		{
			throw runtime_error("Best move is not an action.");
		}

		//MCTS_agent agent(state, 1000, 2);

		//ACTION res = bfs(game);

		// in the first league: BREW <id> | WAIT; later: BREW <id> | CAST <id> [<times>] | LEARN <id> | REST | WAIT
		if (res->action_type == REST)
			cout << "REST" << endl;
		else
			cout << res->s_action_type << " " << res->action_id << endl;
		turn++;
	}
}
