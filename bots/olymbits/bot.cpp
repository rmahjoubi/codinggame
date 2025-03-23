#include <iostream>
#include <bits/stdc++.h>
#include <string>
#include <vector>
#include<map>
#include<set>
#include <algorithm>
#include <ctime>

#include <iostream>
#include <cassert>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <chrono>
//#define DEBUG


using namespace std;

#define MCTS_STATE_H

#include <stdexcept>
#include <queue>


using namespace std;


struct MCTS_move {
    virtual ~MCTS_move() = default;
    virtual bool operator==(const MCTS_move& other) const = 0;             // implement this!
    virtual string sprint() const { return "Not implemented"; }   // and optionally this
};


/** Implement all pure virtual methods. Notes:
 * - rollout() must return something in [0, 1] for UCT to work as intended and specifically
 * the winning chance of player1.
 * - player1 is determined by player1_turn()
 */
class MCTS_state {
public:
    // Implement these:
    virtual ~MCTS_state() = default;
    virtual queue<MCTS_move *> *actions_to_try() const = 0;
    virtual MCTS_state *next_state(const MCTS_move *move) const = 0;
    virtual double rollout() const = 0;
    virtual bool is_terminal() const = 0;
    virtual void print() const {
        cout << "Printing not implemented" << endl;
    }
    virtual bool player1_turn() const = 0;     // MCTS is for two-player games mostly -> (keeps win rate)
};



#define MCTS_H

#include <vector>
#include <queue>
#include <iomanip>


#define STARTING_NUMBER_OF_CHILDREN 4   // expected number so that we can preallocate this many pointers
#define PARALLEL_ROLLOUTS                // whether or not to do multiple parallel rollouts


using namespace std;

/** Ideas for improvements:
 * - state should probably be const like move is (currently problematic because of Quoridor's example)
 * - Instead of a FIFO Queue use a Priority Queue with priority on most probable (better) actions to be explored first
  or maybe this should just be an iterable and we let the implementation decide but these have no superclasses in C++ it seems
 * - vectors, queues and these structures allocate data on the heap anyway so there is little point in using the heap for them
 * so use stack instead?
 */


class MCTS_node {
    bool terminal;
    unsigned int size;
    unsigned int number_of_simulations;
    double score;                       // e.g. number of wins (could be int but double is more general if we use evaluation functions)
    MCTS_state *state;                  // current state
    const MCTS_move *move;              // move to get here from parent node's state
    vector<MCTS_node *> *children;
    MCTS_node *parent;
    queue<MCTS_move *> *untried_actions;
    void backpropagate(double w, int n);
public:
    MCTS_node(MCTS_node *parent, MCTS_state *state, const MCTS_move *move);
    ~MCTS_node();
    bool is_fully_expanded() const;
    bool is_terminal() const;
    const MCTS_move *get_move() const;
    unsigned int get_size() const;
    void expand();
    void rollout();
    MCTS_node *select_best_child(double c) const;
    MCTS_node *advance_tree(const MCTS_move *m);
    const MCTS_state *get_current_state() const;
    void print_stats() const;
    double calculate_winrate(bool player1turn) const;
};



class MCTS_tree {
    MCTS_node *root;
public:
    MCTS_tree(MCTS_state *starting_state);
    ~MCTS_tree();
    MCTS_node *select(double c=1.41);        // select child node to expand according to tree policy (UCT)
    MCTS_node *select_best_child();          // select the most promising child of the root node
    void grow_tree(int max_iter, double max_time_in_seconds);
    void advance_tree(const MCTS_move *move);      // if the move is applicable advance the tree, else start over
    unsigned int get_size() const;
    const MCTS_state *get_current_state() const;
    void print_stats() const;
};


class MCTS_agent {                           // example of an agent based on the MCTS_tree. One can also use the tree directly.
    MCTS_tree *tree;
    int max_iter, max_seconds;
public:
    MCTS_agent(MCTS_state *starting_state, int max_iter = 100000, int max_seconds = 30);
    ~MCTS_agent();
    const MCTS_move *genmove(const MCTS_move *enemy_move);
    const MCTS_state *get_current_state() const;
    void feedback() const { tree->print_stats(); }
};



/*** MCTS NODE ***/
MCTS_node::MCTS_node(MCTS_node *parent, MCTS_state *state, const MCTS_move *move)
        : parent(parent), state(state), move(move), score(0.0), number_of_simulations(0), size(0) {
    children = new vector<MCTS_node *>();
    children->reserve(STARTING_NUMBER_OF_CHILDREN);
    untried_actions = state->actions_to_try();
    terminal = state->is_terminal();
}

MCTS_node::~MCTS_node() {
    delete state;
    delete move;
    for (auto *child : *children) {
        delete child;
    }
    delete children;
    while (!untried_actions->empty()) {
        delete untried_actions->front();    // if a move is here then it is not a part of a child node and needs to be deleted here
        untried_actions->pop();
    }
    delete untried_actions;
}

void MCTS_node::expand() {
    if (is_terminal()) {              // can legitimately happen in end-game situations
        rollout();                    // keep rolling out, eventually causing UCT to pick another node to expand due to exploration
        return;
    } else if (is_fully_expanded()) {
        cerr << "Warning: Cannot expanded this node any more!" << endl;
        return;
    }
    // get next untried action
    MCTS_move *next_move = untried_actions->front();     // get value
    untried_actions->pop();                              // remove it
    MCTS_state *next_state = state->next_state(next_move);
    // build a new MCTS node from it
    MCTS_node *new_node = new MCTS_node(this, next_state, next_move);
    // rollout, updating its stats
    new_node->rollout();
    // add new node to tree
    children->push_back(new_node);
}

void MCTS_node::rollout() {
//#ifdef PARALLEL_ROLLOUTS
//    // schedule Jobs
//    static JobScheduler scheduler;               // static so that we don't create new threads every time (!)
//    double results[NUMBER_OF_THREADS]{-1};
//    for (int i = 0 ; i < NUMBER_OF_THREADS ; i++) {
//        scheduler.schedule(new RolloutJob(state, &results[i]));
//    }
//    // wait for all simulations to finish
//    scheduler.waitUntilJobsHaveFinished();
//    // aggregate results
//    double score_sum = 0.0;
//    for (int i = 0 ; i < NUMBER_OF_THREADS ; i++) {
//        if (results[i] >= 0.0 && results[i] <= 1.0){
//            score_sum += results[i];
//        } else {    // should not happen
//            cerr << "Warning: Invalid result when aggregating parallel rollouts" << endl;
//        }
//    }
//    backpropagate(score_sum, NUMBER_OF_THREADS);
//#else
    double w = state->rollout();
    backpropagate(w, 1);
//#endif
}

void MCTS_node::backpropagate(double w, int n) {
    score += w;
    number_of_simulations += n;
    if (parent != NULL) {
        parent->size++;
        parent->backpropagate(w, n);
    }
}

bool MCTS_node::is_fully_expanded() const {
    return is_terminal() || untried_actions->empty();
}

bool MCTS_node::is_terminal() const {
    return terminal;
}

unsigned int MCTS_node::get_size() const {
    return size;
}

MCTS_node *MCTS_node::select_best_child(double c) const {
    /** selects best child based on the winrate of whose turn it is to play */
    if (children->empty()) return NULL;
    else if (children->size() == 1) return children->at(0);
    else {
        double uct, max = -1;
        MCTS_node *argmax = NULL;
        for (auto *child : *children) {
            double winrate = child->score / ((double) child->number_of_simulations);
            // If its the opponent's move apply UCT based on his winrate i.e. our loss rate.   <-------
            if (!state->player1_turn()){
                winrate = 1.0 - winrate;
            }
            if (c > 0) {
                uct = winrate +
                      c * sqrt(log((double) this->number_of_simulations) / ((double) child->number_of_simulations));
            } else {
                uct = winrate;
            }
            if (uct > max) {
                max = uct;
                argmax = child;
            }
        }
        return argmax;
    }
}

MCTS_node *MCTS_node::advance_tree(const MCTS_move *m) {
    // Find child with this m and delete all others
    MCTS_node *next = NULL;
    for (auto *child: *children) {
        if (*(child->move) == *(m)) {
            next = child;
        } else {
            delete child;
        }
    }
    // remove children from queue so that they won't be re-deleted by the destructor when this node dies (!)
    this->children->clear();
    // if not found then we have to create a new node
    if (next == NULL) {
        // Note: UCT may lead to not fully explored tree even for short-term children due to terminal nodes being chosen
        cout << "INFO: Didn't find child node. Had to start over." << endl;
        MCTS_state *next_state = state->next_state(m);
        next = new MCTS_node(NULL, next_state, NULL);
    } else {
        next->parent = NULL;     // make parent NULL
        // IMPORTANT: m and next->move can be the same here if we pass the move from select_best_child()
        // (which is what we will typically be doing). If not then it's the caller's responsibility to delete m (!)
    }
    // return the next root
    return next;
}


/*** MCTS TREE ***/
MCTS_node *MCTS_tree::select(double c) {
    MCTS_node *node = root;
    while (!node->is_terminal()) {
        if (!node->is_fully_expanded()) {
            return node;
        } else {
            node = node->select_best_child(c);
        }
    }
    return node;
}

MCTS_tree::MCTS_tree(MCTS_state *starting_state) {
    assert(starting_state != NULL);
    root = new MCTS_node(NULL, starting_state, NULL);
}

MCTS_tree::~MCTS_tree() {
    delete root;
}



void MCTS_tree::grow_tree(int max_iter, double max_time_in_milliseconds) {
    MCTS_node *node;
    #ifdef DEBUG
    std::cout << "Growing tree..." << std::endl;
    #endif

    auto start_t = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < max_iter; i++) {
        // select node to expand according to tree policy
        node = select();
        // expand it (this will perform a rollout and backpropagate the results)
        node->expand();
        // check if we need to stop
        auto now_t = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double, std::milli>(now_t - start_t).count();
        if (dt > max_time_in_milliseconds) {
            #ifdef DEBUG
            std::cout << "Early stopping: Made " << (i + 1) << " iterations in " << dt << " ms." << std::endl;
            #endif
            break;
        }
    }

    #ifdef DEBUG
    auto now_t = std::chrono::high_resolution_clock::now();
    double dt = std::chrono::duration<double, std::milli>(now_t - start_t).count();
    std::cout << "Finished in " << dt << " ms." << std::endl;
    #endif
}

//void MCTS_tree::grow_tree(int max_iter, double max_time_in_seconds) {
//    MCTS_node *node;
//    double dt;
//    #ifdef DEBUG
//    cout << "Growing tree..." << endl;
//    #endif
//    time_t start_t, now_t;
//    time(&start_t);
//    for (int i = 0 ; i < max_iter ; i++){
//        // select node to expand according to tree policy
//        node = select();
//        // expand it (this will perform a rollout and backpropagate the results)
//        node->expand();
//        // check if we need to stop
//        time(&now_t);
//        dt = difftime(now_t, start_t);
//        if (dt > max_time_in_seconds) {
//            #ifdef DEBUG
//            cout << "Early stopping: Made " << (i + 1) << " iterations in " << dt << " seconds." << endl;
//            #endif
//            break;
//        }
//    }
//    #ifdef DEBUG
//    time(&now_t);
//    dt = difftime(now_t, start_t);
//    cout << "Finished in " << dt << " seconds." << endl;
//    #endif
//}

unsigned int MCTS_tree::get_size() const {
    return root->get_size();
}

const MCTS_move *MCTS_node::get_move() const {
    return move;
}

const MCTS_state *MCTS_node::get_current_state() const { return state; }

void MCTS_node::print_stats() const {
    #define TOPK 10
    if (number_of_simulations == 0) {
        cout << "Tree not expanded yet" << endl;
        return;
    }
    cout << "___ INFO _______________________" << endl
         << "Tree size: " << size << endl
         << "Number of simulations: " << number_of_simulations << endl
         << "Branching factor at root: " << children->size() << endl
         << "Chances of P1 winning: " << setprecision(4) << 100.0 * (score / number_of_simulations) << "%" << endl;
    // sort children based on winrate of player's turn for this node (!)
    if (state->player1_turn()) {
        std::sort(children->begin(), children->end(), [](const MCTS_node *n1, const MCTS_node *n2){
            return n1->calculate_winrate(true) > n2->calculate_winrate(true);
        });
    } else {
        std::sort(children->begin(), children->end(), [](const MCTS_node *n1, const MCTS_node *n2){
            return n1->calculate_winrate(false) > n2->calculate_winrate(false);
        });
    }
    // print TOPK of them along with their winrates
    cout << "Best moves:" << endl;
    for (int i = 0 ; i < children->size() && i < TOPK ; i++) {
        cout << "  " << i + 1 << ". " << children->at(i)->move->sprint() << "  -->  "
             << setprecision(4) << 100.0 * children->at(i)->calculate_winrate(state->player1_turn()) << "%" << endl;
    }
    cout << "________________________________" << endl;
}

double MCTS_node::calculate_winrate(bool player1turn) const {
    if (player1turn) {
        return score / number_of_simulations;
    } else {
        return 1.0 - score / number_of_simulations;
    }
}

void MCTS_tree::advance_tree(const MCTS_move *move) {
    MCTS_node *old_root = root;
    root = root->advance_tree(move);
    delete old_root;       // this won't delete the new root since we have emptied old_root's children
}

const MCTS_state *MCTS_tree::get_current_state() const { return root->get_current_state(); }

MCTS_node *MCTS_tree::select_best_child() {
    return root->select_best_child(0.0);
}

void MCTS_tree::print_stats() const { root->print_stats(); }


/*** MCTS agent ***/
MCTS_agent::MCTS_agent(MCTS_state *starting_state, int max_iter, int max_seconds)
: max_iter(max_iter), max_seconds(max_seconds) {
    tree = new MCTS_tree(starting_state);
}

const MCTS_move *MCTS_agent::genmove(const MCTS_move *enemy_move) {
    if (enemy_move != NULL) {
        tree->advance_tree(enemy_move);
    }
    // If game ended from opponent move, we can't do anything
    if (tree->get_current_state()->is_terminal()) {
        return NULL;
    }
    #ifdef DEBUG
    cout << "___ DEBUG ______________________" << endl
         << "Growing tree..." << endl;
    #endif
    tree->grow_tree(max_iter, max_seconds);
    #ifdef DEBUG
    cout << "Tree size: " << tree->get_size() << endl
         << "________________________________" << endl;
    #endif
    MCTS_node *best_child = tree->select_best_child();
    if (best_child == NULL) {
        cerr << "Warning: Tree root has no children! Possibly terminal node!" << endl;
        return NULL;
    }
    const MCTS_move *best_move = best_child->get_move();
    tree->advance_tree(best_move);
    return best_move;
}

MCTS_agent::~MCTS_agent() {
    delete tree;
}

const MCTS_state *MCTS_agent::get_current_state() const { return tree->get_current_state(); }





using namespace std;

int current_turn{};
int maximal_traversed{};



void display(vector<int> &a){for(int z : a)cerr << z << " ";cerr << endl;}

enum Action {WAIT, LEFT, RIGHT, UP, DOWN};
map<Action, string> action_map = {
    {WAIT, "WAIT"},
    {LEFT, "LEFT"},
    {RIGHT, "RIGHT"},
    {UP, "UP"},
    {DOWN, "DOWN"},
};

class TURN;
class ARCADE;
class AGENT;
class ACTION;


class TURN : public MCTS_state{
public:
    int count{};
    vector<ARCADE> arcades;
    //TODO: are we sure we have four arcades;

    TURN(vector<ARCADE> arcades, int count);
    void next(ACTION action);
    int f_maximal_traversed();
    int arcade_perfect_score(int id);
    bool is_terminal() const override;
    MCTS_state* next_state(const MCTS_move* move) const override;
    queue<MCTS_move*>* actions_to_try() const override;
    double rollout() const override;
    bool player1_turn() const override {return true;}
};

class ACTION : public MCTS_move{
public:
    Action action{};

    ACTION(){};
    ACTION(Action action){this->action = action;}
    void print();
    void apply(AGENT& agent, ARCADE& arcade);
    bool operator==(const MCTS_move& other) const;
};


class ARCADE{
public:
    int id{};
    string gpu{};
    string advanced_gpu{};
    int reg0{};
    int reg1{};
    int reg2{};
    int reg3{};
    int reg4{};
    int reg5{};
    int reg6{};
    vector<pair<int,int>> aagents{};
    int my_agent_id{};
    int my_traversed{};
    bool restarting{};
    int rank{};
    map<int, AGENT> agents{};

    ARCADE(int my_agent_id, map<string, int> regs, string gpu, int id, int my_traversed);
    AGENT& get_agent(int id);
    ACTION best_action(AGENT &agent);
    void restart();
    virtual void update_rank();
    virtual void update_advanced_gpu();
    virtual void apply_action(ACTION& action, int agent_id);
    AGENT& my_agent();
};

class HURDLE : public ARCADE{
    int traversed{};
    
    void update_advanced_gpu() override;
    void update_rank() override;
    void apply_action(ACTION& action, int agent_id) override;
};

void HURDLE::apply_action(ACTION& action, int agent_id){
    pair<int, int> agent = aagents[agent_id];
    if(agent.second > 0){
        //cerr << "agent stuned" << endl;
        agent.second--;
        return;
    }
    int steps = 0;
    switch(action.action){
        case WAIT:
            return;
        case LEFT:
            steps = 1;
            break;
        case UP:
            steps = 2;
            agent.first += steps;
            traversed += steps;
            if (gpu[agent.first] == '#')
            {
                agent.second = 3;
            }
            return;
        case RIGHT:
            steps = 3;
            break;
        case DOWN:
            steps = 2;
            break;
    }
    for(int i = 1; i <= steps && agent.first + i < 30; i++){
        if(gpu[agent.first + i] == '#'){
            agent.first += i;
            traversed += i;
            agent.second = 3;

            return;
        }
    }
    agent.first += steps;
    traversed += steps;
}

class  ARCHERY: public ARCADE{
    deque<int> wind{};

    void update_advanced_gpu() override;
    void update_rank() override;
    void apply_action(ACTION& action, int agent_id) override;
};

void ARCHERY::apply_action(ACTION& action, int agent_id){
    pair<int, int> agent = aagents[agent_id];
    switch(action.action){
        case WAIT:
            return;
        case LEFT:
            agent.first -= wind.front();
            wind.pop_front();
            break;
        case UP:
            agent.second += wind.front();
            wind.pop_front();
            break;
        case RIGHT:
            agent.first += wind.front();
            wind.pop_front();
            break;
        case DOWN:
            agent.second -= wind.front();
            wind.pop_front();
            break;
    }
}

class ROLLER: public ARCADE{
    map<Action, pair<int,int>> effects{};
    void update_advanced_gpu() override;
    void update_rank() override;
    void apply_action(ACTION& action, int agent_id) override;
};

void ROLLER::apply_action(ACTION& action, int agent_id){
    pair<int, int> agent = aagents[agent_id];
    if (agent.second < 0){
        agent.second++;
        return;
    }
    agent.first += effects[action.action].first;
    agent.second += effects[action.action].second;
    //TODO : if two agents in the same space
}

class DIVING: public ARCADE{
    deque<Action> expected;
    void update_advanced_gpu() override;
    void update_rank() override;
    void apply_action(ACTION& action, int agent_id) override;
};


void DIVING::apply_action(ACTION& action, int agent_id){
    pair<int, int> agent = aagents[agent_id];
    if (agent.second < 0){
        agent.second++;
        return;
    }
    if(action.action == expected.front()){
        agent.first++;
    }
    else{
        agent.first = 0;
    }
    expected.pop_front();
}




class AGENT{
public:
    int id{};
    int position{};
    int stun_timer{};

    AGENT(){}
};


//--------------------------------------------ARCADE-----------------------------------------------

ARCADE::ARCADE(int my_agent_id, map<string, int> regs, string gpu, int id, int my_traversed) : my_agent_id(my_agent_id), gpu(gpu), id(id), my_traversed(my_traversed){

    agents[0].id = 0;
    agents[1].id = 1;
    agents[2].id = 2;
    if(gpu == "GAME_OVER"){
        restart();
        return;
    }
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
    update_advanced_gpu();
    update_rank();
}

AGENT& ARCADE::my_agent(){
    return agents[my_agent_id];
}

AGENT& ARCADE::get_agent(int id){return agents[id];}

void ARCADE::restart(){
    for(auto& it : agents){
        it.second.position = 0;
        it.second.stun_timer = 0;
    }
    restarting = true;
    update_advanced_gpu();
    update_rank();
}


ACTION ARCADE::best_action(AGENT &agent){
    set<int> hurdles;
    for(int i = 0; i < gpu.size(); i++){
        if(gpu[i] == '#'){
            hurdles.insert(i);
        }
    }

    auto it = hurdles.upper_bound(agent.position);

    ACTION action;
    action.action = RIGHT;
    if(it  != hurdles.end()){
        if(*it - agent.position == 5 || *it - agent.position == 1 || *it - agent.position == 3)
            action.action = UP;
        else if(*it - agent.position > 3)
            action.action = RIGHT;
        else if(*it - agent.position == 2)
            action.action = LEFT;
        else
            action.action = RIGHT;
    }

    return action;
}

void ARCADE::update_rank(){
    int count = 0;
    for(auto& agent : agents){
        if(agent.second.position > my_agent().position) count++;
    }
    switch (count){
        case 0:
            rank = 1;
            break;
        case 1:
            rank = 2;
            break;
        case 2:
            rank = 3;
            break;
    }
}

void ARCADE::update_advanced_gpu(){
    string tmp = gpu;
    tmp[agents[0].position] = char(int('0') + agents[0].stun_timer);
    advanced_gpu = gpu + "\n" + tmp;
}

//--------------------------------------------ACTIONS-----------------------------------------------

void ACTION::apply(AGENT &agent, ARCADE &arcade){
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
            arcade.my_traversed += steps;
            if (arcade.gpu[agent.position] == '#')
            {
                agent.stun_timer = 3;
            }
            return;
        case RIGHT:
            steps = 3;
            break;
        case DOWN:
            steps = 2;
            break;
    }
    for(int i = 1; i <= steps && agent.position + i < 30; i++){
        if(arcade.gpu[agent.position + i] == '#'){
            agent.position += i;
            arcade.my_traversed += i;
            agent.stun_timer = 3;

            return;
        }
    }
    agent.position += steps;
    arcade.my_traversed += steps;
}


void ACTION::print(){
    cout << action_map[this->action] << endl;
}

bool ACTION::operator==(const MCTS_move& other) const{
    const ACTION &a = (const ACTION&) other;
    return action == a.action;
}


//--------------------------------------------TURN-----------------------------------------------



TURN::TURN(vector<ARCADE> arcades, int count) : arcades(arcades), count(count){
}

bool TURN::is_terminal() const
{
    return count >= 100;
}

MCTS_state* TURN::next_state(const MCTS_move *move) const{
    ACTION *a = (ACTION*) move;
    TURN *new_state = new TURN(*this);
    new_state->next(*a);
    return new_state;
}

queue<MCTS_move*>* TURN::actions_to_try() const{
	queue<MCTS_move*>* Q = new queue<MCTS_move*>();
    vector<Action> all_a = {LEFT, UP, RIGHT};
    for(auto a : all_a){
        Q->push(new ACTION(a));
    }
    return Q;
}

double TURN::rollout() const{
    vector<Action> all_a = {LEFT, UP, RIGHT};
    int total_traversed = 0;
    for(auto& arcade : arcades)
        total_traversed += arcade.my_traversed;
	if (is_terminal()) return static_cast<double>(total_traversed / maximal_traversed);
    long long r;
    ACTION a;
    TURN a_turn(*this);
    srand(time(NULL));
    do{
        r = rand() % all_a.size();
        ACTION a = ACTION(all_a[r]);
        a_turn.next(a);
    }while(!a_turn.is_terminal() && a_turn.count - this->count < 12);
    total_traversed = 0;
    for(auto& arcade : arcades)
        total_traversed += arcade.my_traversed;
    double res = double(total_traversed) / maximal_traversed;
    return res;
}

int TURN::f_maximal_traversed(){
    int res = 0;
    for(auto arcade : arcades){
        TURN tmp = *this;
        tmp.arcades = {arcade};
        while(!tmp.is_terminal()){
            ACTION action = arcade.best_action(tmp.arcades[0].my_agent());
            tmp.next(action);
        }
        res += tmp.arcades[0].my_traversed;
    }
   return res;
}

void TURN::next(ACTION action){
    count++;
    if(action.action == WAIT){
        return;
    }
    for(ARCADE& arcade : arcades){
        if(arcade.restarting){
            arcade.restarting = false;
            continue;
        }
        //for(auto& agent : arcade.agents){
        //    if(agent.second.stun_timer > 0){
        //        agent.second.stun_timer--;
        //    }
        //}
        if (arcade.agents[arcade.my_agent_id].stun_timer > 0)
        {
            arcade.agents[arcade.my_agent_id].stun_timer--;
            continue;
        }
        //cout << action_map[action.action] << endl;
        action.apply(arcade.my_agent(), arcade);
        arcade.update_rank();

        for(auto& agent : arcade.agents){
            //cerr << "agent position == " << arcade.my_agent().position << endl;
            //exit(0);
            if(agent.second.position >= 30){
                arcade.restart();
                break;
            }
        }
        arcade.update_advanced_gpu();
        //cerr << "arcade id == " << arcade.id << endl;
        //cerr << arcade.advanced_gpu;

    }
}



int TURN::arcade_perfect_score(int id){
    TURN tmp = *this;
    int res;
    while(!tmp.arcades[id].restarting){
        ACTION action = tmp.arcades[id].best_action(tmp.arcades[id].my_agent());
        tmp.next(action);
        res++;
    }
    return res;
}


//TODO: after each turn reduce stun timer

//--------------------------------------------arcade-----------------------------------------------
/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main()
{
    int player_idx;
    cin >> player_idx; cin.ignore();
    cerr << player_idx << endl;
    int nb_games;
    cin >> nb_games; cin.ignore();
    cerr << nb_games << endl;

    int max_iter = 10000000;
    int max_seconds = 50 ;

    //arcades.reserve(nb_games);
    //for (int i = 0; i < nb_games; i++) {
    //    arcades.emplace_back(player_idx);
    //}
    //for(int i = 0; i < arcades.size(); i++){
    //    arcades[i].id = i;
    //}

    // game loop
    while (1) {
        vector<ARCADE> arcades;
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
            ARCADE arcade(player_idx, {{"reg0",reg_0},{"reg1", reg_1},{"reg2", reg_2},{"reg3", reg_3},{"reg4", reg_4},{"reg5", reg_5},{"reg6", reg_6}}, gpu, i, 0);
            arcades.push_back(arcade);
        }
        TURN turn(arcades, current_turn);
        maximal_traversed = turn.f_maximal_traversed();
        MCTS_state *state = new TURN(arcades, current_turn);
		MCTS_tree* tree = new MCTS_tree(state);
		tree->grow_tree(max_iter, max_seconds);
        cerr << "gorw tree done" << endl;
		MCTS_node *best_child = tree->select_best_child();
		if (best_child == NULL)
			throw runtime_error("No best child found from main.");
		const MCTS_move *best_move = best_child->get_move();
		const ACTION *res = dynamic_cast<const ACTION *>(best_move);
		if (!res)
		{
			throw runtime_error("Best move is not an action.");
		}
        cerr << res->action <<"----"<< endl;
        current_turn++;
        cout << action_map[res->action] << endl;
        cerr << res->action <<"----"<< endl;

        //return 0;

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
    }
}


