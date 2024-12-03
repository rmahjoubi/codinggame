#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <cmath>
#include <limits>
#include <random>


using namespace std;

void display(vector<int> &a){for(int z : a)cerr << z << " ";cerr << endl;}

enum Action {LEFT, RIGHT, UP, DOWN, WAIT, COUNT};
map<Action, string> action_map = {
    {LEFT, "LEFT"},
    {RIGHT, "RIGHT"},
    {UP, "UP"},
    {DOWN, "DOWN"},
    {WAIT, "WAIT"},
};

class GAME;
class ARCADE;
class AGENT;


class GAME{
public:
    int turn{};
    int my_player_id;
    vector<ARCADE> arcades;
    //TODO: are we sure we have four arcades;
    map<int, pair<int,int>> current_elo;

    explicit GAME(int my_player_id, int nb_games);
    void apply_action(Action action);
};


class ARCADE{
public:
    int id{};
    string gpu{};
    string advanced_gpu{};
    //game finished
    bool game_over{};
    //game finished and we are in a reset turn
    int reg0{};
    int reg1{};
    int reg2{};
    int reg3{};
    int reg4{};
    int reg5{};
    int reg6{};
    int my_agent_id{};
    bool restarting{};
    map<int, AGENT> agents{};

    explicit ARCADE(int my_agent_id);
    AGENT& get_agent(int id);
    //ACTION best_action(AGENT &agent);
    void start_game();
    void finish_game();
    void update_ranks();
    void update_advanced_gpu();
    AGENT& my_agent();
    void update(map<string, int> regs, string gpu);
    void apply_action(Action action);
};

class AGENT{
public:
    int id{};
    int position{};
    int stun_timer{};
    int rank{};
    AGENT()= default;
};

//--------------------------------------------ARCADE-----------------------------------------------

ARCADE::ARCADE(int my_agent_id) : my_agent_id(my_agent_id) {
    agents[0].id = 0;
    agents[1].id = 1;
    agents[2].id = 2;
}

AGENT& ARCADE::my_agent(){
    return agents[my_agent_id];
}

void ARCADE::update(map<string, int> regs, string gpu){
    reg0 = regs["reg0"];
    reg1 = regs["reg1"];
    reg2 = regs["reg2"];
    reg3 = regs["reg3"];
    reg4 = regs["reg4"];
    reg5 = regs["reg5"];
    reg6 = regs["reg6"];
    agents[0].position = reg0;
    agents[0].stun_timer = reg3;
    agents[1].position = reg1;
    agents[1].stun_timer = reg4;
    agents[2].position = reg2;
    agents[2].stun_timer = reg5;

    this->gpu = gpu;
    update_advanced_gpu();
    update_ranks();
    //advanced_gpu[gpu.size() * 2 + 2 + agents[1].position] = '1';
    //advanced_gpu[gpu.size() * 3 + 3 + agents[2].position] = '2';
    //cerr << advanced_gpu;
}

AGENT& ARCADE::get_agent(int id){return agents[id];}

void ARCADE::start_game(){
    for(auto& it : agents){
        it.second.position = 0;
        it.second.stun_timer = 0;
        it.second.rank = 1;
    }
    game_over = false;
}

void ARCADE::finish_game(){
   game_over = true;
}

void ARCADE::apply_action(Action action){
    if(game_over){
        // TODO: we should start game
        //arcade.start_game();
        return;
    }
    for(auto& agent : agents){
        if(agent.second.stun_timer > 0){
            agent.second.stun_timer--;
        }
    }
    if(action == WAIT){
        return;
    }
    //cout << action_map[action.action] << endl;
    //TODO we should get a list of actions, and apply them each to each agent
    AGENT& agent = agents[my_agent_id];
    if(agent.stun_timer > 0){
        //cerr << "agent stuned" << endl;
        return;
    }
    int steps = 0;
    switch(action){
        case WAIT:
            return;
        case LEFT:
            steps = 1;
            break;
        case UP:
            steps = 2;
            agent.position += steps;
            return;
        case RIGHT:
            steps = 3;
            break;
        case DOWN:
            steps = 2;
            break;
    }
    for(int i = 1; i <= steps && agent.position + i < 30; i++){
        if(gpu[agent.position + i] == '#'){
            agent.position += i;
            agent.stun_timer = 3;

            return;
        }
    }
    agent.position += steps;
    update_ranks();

    for(auto& agent : agents){
        //cerr << "agent position == " << arcade.my_agent().position << endl;
        //exit(0);
        if(agent.second.position >= 30){
            //someone reached the finish line
            finish_game();
            break;
        }
    }
    update_advanced_gpu();
}

//ACTION ARCADE::best_action(AGENT &agent){
//    set<int> hurdles;
//    for(int i = 0; i < gpu.size(); i++){
//        if(gpu[i] == '#'){
//            hurdles.insert(i);
//        }
//    }
//
//    auto it = hurdles.upper_bound(agent.position);
//
//    ACTION action(RIGHT, my_agent());
//    if(it  != hurdles.end()){
//        if(*it - agent.position == 5 || *it - agent.position == 1 || *it - agent.position == 3)
//            action.action = UP;
//        else if(*it - agent.position > 3)
//            action.action = RIGHT;
//        else if(*it - agent.position == 2)
//            action.action = RIGHT;
//        else
//            action.action = RIGHT;
//    }
//
//    return action;
//}

void ARCADE::update_ranks(){
    vector<pair<int,int>> tmp;
    for (auto & [fst, snd] : agents){
       tmp.emplace_back(snd.position, snd.id);
    }
    sort(tmp.rbegin(), tmp.rend());
    for (int i = 0; i < tmp.size(); i++){
       agents[tmp[i].second].rank = i + 1;
    }
}

void ARCADE::update_advanced_gpu(){
    string tmp = gpu;
    tmp[agents[0].position] = '0';
    advanced_gpu = gpu + "\n" + tmp;
}

//--------------------------------------------TURN-----------------------------------------------

GAME::GAME(int my_player_id, int nb_games): my_player_id(my_player_id){

    vector<ARCADE> arcades;
    arcades.reserve(nb_games);
    for (int i = 0; i < nb_games; i++) {
        arcades.emplace_back(this->my_player_id);
    }
    this->arcades = std::move(arcades);
    for(int i = 0; i < arcades.size(); i++){
        arcades[i].id = i;
    }
    turn = 0;
}

void GAME::apply_action(Action action){
    for(ARCADE& arcade : arcades){
        arcade.apply_action(action);
    }
    turn++;
}

//--------------------------------------------  MONTE CARLO -----------------------------------------------


class MCTSNode {
public:
    // Game state would be passed from your simulation
    GAME* gameState;
    std::vector<std::unique_ptr<MCTSNode>> children;
    MCTSNode* parent;

    // Number of visits and total score for this node
    int visits;
    double totalScore;

    // Possible actions (4 actions as specified)
    Action actionTaken;

    MCTSNode(GAME* state, MCTSNode* parentNode = nullptr, Action action = WAIT)
        : gameState(state), parent(parentNode), visits(0),
          totalScore(0), actionTaken(action) {}
};

class MonteCarloTreeSearch {
private:
    // Function pointers to game simulation methods
    GAME* (*copyState)(GAME*);
    void (*freeState)(GAME*);
    bool (*isTerminal)(GAME*);
    double (*getScore)(GAME*);
    GAME* (*takeAction)(GAME*, Action);

    // Random number generation
    std::mt19937 rng;

    const double EXPLORATION_CONSTANT = std::sqrt(2.0);
    const int MAX_ITERATIONS = 1000;
    const int MAX_DEPTH = 100;

    // Selection phase: choose the most promising node to explore
    MCTSNode* select(MCTSNode* node) {
        while (!isTerminal(node->gameState)) {
            if (node->children.empty()) {
                return expand(node);
            }

            // Select child using UCB1
            node = selectBestChild(node);
        }
        return node;
    }

    // Expansion phase: create child nodes
    MCTSNode* expand(MCTSNode* node) {
        // Try all 4 possible actions
        for (int i = 0; i < COUNT; ++i) {
            auto action = static_cast<Action>(i);
            GAME* newState = takeAction(node->gameState, action);
            if (newState) {
                auto childNode = std::make_unique<MCTSNode>(newState, node, action);
                node->children.push_back(std::move(childNode));
            }
        }

        // Return the first child if available
        return node->children.empty() ? node : node->children[0].get();
    }

    // Simulation phase: play out from the node
    double simulate(GAME* state) {
        GAME* simulationState = copyState(state);
        int depth = 0;

        while (!isTerminal(simulationState) && depth < MAX_DEPTH) {
            // Randomly choose an action
            auto action = static_cast<Action>(rng() % 4);
            GAME* nextState = takeAction(simulationState, action);

            if (!nextState) continue;

            // Free previous state to prevent memory leak
            freeState(simulationState);
            simulationState = nextState;

            depth++;
        }

        // Get the score based on game completion
        double score = getScore(simulationState);
        freeState(simulationState);

        return score;
    }

    // Backpropagation phase: update node statistics
    void backpropagate(MCTSNode* node, double score) {
        while (node) {
            node->visits++;
            node->totalScore += score;
            node = node->parent;
        }
    }

    // UCB1 selection strategy
    MCTSNode* selectBestChild(MCTSNode* node) {
        double bestUCB = -std::numeric_limits<double>::infinity();
        MCTSNode* bestChild = nullptr;

        for (auto& child : node->children) {
            if (child->visits == 0) {
                return child.get();
            }

            double exploitation = child->totalScore / child->visits;
            double exploration = std::sqrt(std::log(node->visits) / child->visits);
            double ucb = exploitation + EXPLORATION_CONSTANT * exploration;

            if (ucb > bestUCB) {
                bestUCB = ucb;
                bestChild = child.get();
            }
        }

        return bestChild;
    }

public:
    // Constructor to set game simulation function pointers
    MonteCarloTreeSearch(
        GAME* (*copyStateFunc)(GAME*),
        void (*freeStateFunc)(GAME*),
        bool (*isTerminalFunc)(GAME*),
        double (*getScoreFunc)(GAME*),
        GAME* (*takeActionFunc)(GAME*, Action)
    ) :
        copyState(copyStateFunc),
        freeState(freeStateFunc),
        isTerminal(isTerminalFunc),
        getScore(getScoreFunc),
        takeAction(takeActionFunc),
        rng(std::random_device()())
    {}

    // Main MCTS method to find the best action
    Action findBestAction(GAME* initialState) {
        // Create root node
        auto root = std::make_unique<MCTSNode>(copyState(initialState));

        // Run MCTS iterations
        for (int i = 0; i < MAX_ITERATIONS; ++i) {
            // Copy initial state to prevent modification
            GAME* iterationState = copyState(initialState);

            // Selection
            MCTSNode* selectedNode = select(root.get());

            // Simulation
            double score = simulate(selectedNode->gameState);

            // Backpropagation
            backpropagate(selectedNode, score);

            // Free the iteration state
            freeState(iterationState);
        }

        // Select the best child based on most visits
        MCTSNode* bestChild = selectBestChild(root.get());
        Action bestAction = bestChild ? bestChild->actionTaken : WAIT;

        return bestAction;
    }
};

//--------------------------------------------  MONTE CARLO -----------------------------------------------


//TODO: after each turn reduce stun timer

//--------------------------------------------arcade-----------------------------------------------
/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

GAME* copyState(GAME* game) {
    return new GAME(*game);
}

void freeState(GAME* game) {
    delete game;
}

bool isTerminal(GAME* game) {
    // Terminal condition: all arcades at perfect score
    return std::all_of(game->arcades.begin(), game->arcades.end(),
        [](const ARCADE& arcade) {
            return arcade.game_over;
        }
    ) || game->turn == 100;
}

double getScore(GAME* game) {

    // Reward efficiency: lower turns means higher score
    return 1.0 / (game->turn + 1.0);
}

GAME* takeAction(GAME* game, int actionIndex) {
    // Create a copy of the game state
    GAME* newGame = new GAME(*game);

    // Assuming you have 4 actions matching the actionIndex
    Action action = static_cast<Action>(actionIndex);
    // TODO: Map actionIndex to appropriate ACTION
    newGame->apply_action(action);

    return newGame;
}

int main()
{
    int player_idx;
    cin >> player_idx; cin.ignore();
    cerr << player_idx << endl;
    int nb_games;
    cin >> nb_games; cin.ignore();
    cerr << nb_games << endl;

    GAME game(player_idx, nb_games);
    // game loop
    while (1) {
        for (int i = 0; i < 3; i++) {
            string score_info;
            getline(cin, score_info);
            cerr << score_info << endl;
        }
        for (int i = 0; i < nb_games; i++) {
            string gpu;
            int reg_0;
            int reg_1;
            int reg_2;
            int reg_3;
            int reg_4;
            int reg_5;
            int reg_6;
            cin >> gpu >> reg_0 >> reg_1 >> reg_2 >> reg_3 >> reg_4 >> reg_5 >> reg_6; cin.ignore();
            cerr << gpu << " " << reg_0 << " " << reg_1 << " " << reg_2 << " " << reg_3 << " " << reg_4 << " " << reg_5 << " " << reg_6 << endl;
            game.arcades[i].update({{"reg0",reg_0},{"reg1", reg_1},{"reg2", reg_2},{"reg3", reg_3},{"reg4", reg_4},{"reg5", reg_5},{"reg6", reg_6}}, gpu);
        }
        //break;

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
    }
}

