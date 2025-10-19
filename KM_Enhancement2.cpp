// CS-499 Computer Science Capstone - Enhancement 2 - Algorithms & Data Structures
// Kris Marchevka

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <cctype>
#include <functional>

using namespace std;

//utility

static inline string trim(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static inline string toUpper(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
    return s;
}

//model

class Course {
public:
    string courseNumber;               // ex "CSCI300"
    string courseTitle;                // ex "Data Structures"
    vector<string> prerequisites;      // list of course numbers

    Course() {}
    Course(string number, string title, const vector<string>& prereqs)
        : courseNumber(std::move(number)), courseTitle(std::move(title)), prerequisites(prereqs) {}
};

// unbalanced binary search tree

struct TreeNode {
    Course course;
    TreeNode* left;
    TreeNode* right;
    explicit TreeNode(const Course& c) : course(c), left(nullptr), right(nullptr) {}
};

class BinarySearchTree {
    TreeNode* root = nullptr;

    static TreeNode* insertRec(TreeNode* node, const Course& c) {
        if (!node) return new TreeNode(c);
        if (c.courseNumber < node->course.courseNumber) node->left = insertRec(node->left, c);
        else node->right = insertRec(node->right, c);
        return node;
    }

    static TreeNode* searchRec(TreeNode* node, const string& key) {
        if (!node || node->course.courseNumber == key) return node;
        return (key < node->course.courseNumber)
            ? searchRec(node->left, key)
            : searchRec(node->right, key);
    }

    static void inOrderRec(TreeNode* node) {
        if (!node) return;
        inOrderRec(node->left);
        cout << node->course.courseNumber << ", " << node->course.courseTitle << '\n';
        inOrderRec(node->right);
    }

    static void destroy(TreeNode* node) {
        if (!node) return;
        destroy(node->left); destroy(node->right); delete node;
    }

public:
    ~BinarySearchTree() { destroy(root); }
    void insert(const Course& c) { root = insertRec(root, c); }
    const Course* search(const string& key) const {
        TreeNode* n = searchRec(root, key);
        return n ? &n->course : nullptr;
    }
    void printInOrder() const {
        cout << "Here is a sample schedule:\n\n";
        inOrderRec(root);
    }
};

// avl tree

struct AVLNode {
    Course course;
    AVLNode* left;
    AVLNode* right;
    int height;
    explicit AVLNode(const Course& c) : course(c), left(nullptr), right(nullptr), height(1) {}
};

class AVLTree {
    AVLNode* root = nullptr;

    static int h(AVLNode* n) { return n ? n->height : 0; }
    static int bf(AVLNode* n) { return n ? h(n->left) - h(n->right) : 0; }

    static void update(AVLNode* n) {
        if (n) n->height = 1 + max(h(n->left), h(n->right));
    }

    static AVLNode* rotateRight(AVLNode* y) {
        AVLNode* x = y->left;
        AVLNode* T2 = x->right;
        x->right = y; y->left = T2;
        update(y); update(x);
        return x;
    }

    static AVLNode* rotateLeft(AVLNode* x) {
        AVLNode* y = x->right;
        AVLNode* T2 = y->left;
        y->left = x; x->right = T2;
        update(x); update(y);
        return y;
    }

    static AVLNode* balance(AVLNode* node) {
        update(node);
        int b = bf(node);
        if (b > 1) {
            if (bf(node->left) < 0) node->left = rotateLeft(node->left);
            return rotateRight(node);
        }
        if (b < -1) {
            if (bf(node->right) > 0) node->right = rotateRight(node->right);
            return rotateLeft(node);
        }
        return node;
    }

    static AVLNode* insertRec(AVLNode* node, const Course& c) {
        if (!node) return new AVLNode(c);
        if (c.courseNumber < node->course.courseNumber) node->left = insertRec(node->left, c);
        else node->right = insertRec(node->right, c);
        return balance(node);
    }

    static const Course* searchRec(AVLNode* node, const string& key) {
        if (!node) return nullptr;
        if (node->course.courseNumber == key) return &node->course;
        if (key < node->course.courseNumber) return searchRec(node->left, key);
        return searchRec(node->right, key);
    }

    static void destroy(AVLNode* n) {
        if (!n) return;
        destroy(n->left); destroy(n->right); delete n;
    }

public:
    ~AVLTree() { destroy(root); }
    void insert(const Course& c) { root = insertRec(root, c); }
    const Course* search(const string& key) const { return searchRec(root, key); }
};

// course catalog structure

class CourseCatalog {
public:
    // baselines with alternatives
    vector<Course> vec;                                 // linear-scan baseline
    BinarySearchTree bst;                               // original structure
    AVLTree avl;                                        // balanced tree
    unordered_map<string, Course> hmap;                 // hash index

    // Graph: prereq -> list of dependent courses
    unordered_map<string, vector<string>> graph;
    unordered_set<string> courseCodes;                  // set of all codes

    // diagnostics
    vector<string> malformedRows;
    vector<pair<string, string>> missingPrereqs;         // course, missingPrereq

    bool loaded = false;

    void clear() {
        vec.clear();
        hmap.clear();
        graph.clear();
        courseCodes.clear();
        malformedRows.clear();
        missingPrereqs.clear();
        loaded = false;
        // bst & avl destructors clear on scope end so now we rely on fresh instances:
        bst = BinarySearchTree();
        avl = AVLTree();
    }

    // Load CSV, build all structures, and validate
    bool loadAll(const string& filename = "CS 300 ABCU_Advising_Program_Input.csv") {
        clear();
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Error opening file: " << filename << '\n';
            return false;
        }

        string line;
        size_t lineNo = 0;
        vector<Course> staged;

        while (getline(file, line)) {
            ++lineNo;
            if (trim(line).empty()) continue;
            stringstream ss(line);
            string num, title, prereq;
            vector<string> prereqs;

            if (!getline(ss, num, ',')) { malformedRows.push_back("Line " + to_string(lineNo) + ": missing course number"); continue; }
            if (!getline(ss, title, ',')) { malformedRows.push_back("Line " + to_string(lineNo) + ": missing course title"); continue; }

            num = toUpper(trim(num));
            title = trim(title);
            if (num.empty() || title.empty()) {
                malformedRows.push_back("Line " + to_string(lineNo) + ": empty course number/title");
                continue;
            }

            while (getline(ss, prereq, ',')) {
                prereq = toUpper(trim(prereq));
                if (!prereq.empty()) prereqs.push_back(prereq);
            }

            Course c(num, title, prereqs);
            staged.push_back(c);
            courseCodes.insert(num);
        }
        file.close();

        // build structures
        for (const auto& c : staged) {
            vec.push_back(c);
            hmap[c.courseNumber] = c;
            bst.insert(c);
            avl.insert(c);
        }

        // build graph (prereq -> course) and record missing prereqs
        for (const auto& c : staged) {
            for (const auto& p : c.prerequisites) {
                if (!courseCodes.count(p)) {
                    missingPrereqs.emplace_back(c.courseNumber, p);
                }
                else {
                    graph[p].push_back(c.courseNumber);
                }
            }
            // ensures each course appears as a vertex
            if (!graph.count(c.courseNumber)) graph[c.courseNumber] = {};
        }

        loaded = true;
        cout << "Courses loaded (" << vec.size() << ").\n";
        if (!malformedRows.empty() || !missingPrereqs.empty()) {
            cout << "\n=== CSV Warnings ===\n";
            for (const auto& m : malformedRows) cout << m << '\n';
            for (const auto& miss : missingPrereqs)
                cout << "Missing prereq: " << miss.first << " requires " << miss.second << " (not found)\n";
            cout << "====================\n\n";
        }
        return true;
    }

    // search helpers
    const Course* findVector(const string& key) const {
        for (const auto& c : vec) if (c.courseNumber == key) return &c;
        return nullptr;
    }
    const Course* findHash(const string& key) const {
        auto it = hmap.find(key);
        return it == hmap.end() ? nullptr : &it->second;
    }
    const Course* findBST(const string& key) const { return bst.search(key); }
    const Course* findAVL(const string& key) const { return avl.search(key); }

    // graph algos

    // DFS cycle detection (returns any cycle path found)
    bool hasCycle() const {
        enum State { UNVIS = 0, VISITING = 1, VISITED = 2 };
        unordered_map<string, int> state;
        for (auto& kv : graph) state[kv.first] = UNVIS;

        function<bool(const string&)> dfs = [&](const string& u)->bool {
            state[u] = VISITING;
            auto it = graph.find(u);
            if (it != graph.end()) {
                for (const auto& v : it->second) {
                    if (state[v] == VISITING) return true;        // back-edge
                    if (state[v] == UNVIS && dfs(v)) return true; // deeper cycle
                }
            }
            state[u] = VISITED;
            return false;
            };

        for (auto& kv : graph) {
            if (state[kv.first] == UNVIS && dfs(kv.first)) return true;
        }
        return false;
    }

    // Kahn's algorithm for topological order
    vector<string> topoOrder(bool& ok) const {
        unordered_map<string, int> indeg;
        for (auto& kv : graph) {
            if (!indeg.count(kv.first)) indeg[kv.first] = 0;
            for (auto& v : kv.second) indeg[v]++;
        }
        queue<string> q;
        for (auto& kv : indeg) if (kv.second == 0) q.push(kv.first);

        vector<string> order;
        while (!q.empty()) {
            string u = q.front(); q.pop();
            order.push_back(u);
            auto it = graph.find(u);
            if (it != graph.end()) {
                for (auto& v : it->second) {
                    if (--indeg[v] == 0) q.push(v);
                }
            }
        }
        ok = (order.size() == indeg.size());
        return order;
    }

    // benchmarking search time
    void benchmarkSearches(size_t repeatsPerKey = 200) {
        if (!loaded) { cout << "Load data first.\n\n"; return; }
        if (vec.empty()) { cout << "No data.\n\n"; return; }

        vector<string> keys;
        keys.reserve(vec.size());
        for (const auto& c : vec) keys.push_back(c.courseNumber);

        auto bench = [&](const string& name, auto finder) {
            using clock = chrono::high_resolution_clock;
            auto t0 = clock::now();
            size_t hits = 0;
            for (const auto& k : keys) {
                for (size_t r = 0; r < repeatsPerKey; ++r) {
                    hits += (finder(k) != nullptr);
                }
            }
            auto t1 = clock::now();
            auto us = chrono::duration_cast<chrono::microseconds>(t1 - t0).count();
            cout << name << " total: " << us << " us (microseconds)"
                << "  (checks: " << hits << ")\n";
            };

        cout << "Benchmarking search (" << keys.size() << " keys x " << repeatsPerKey << " reps)...\n";
        bench("Vector (linear)", [this](const string& k) { return this->findVector(k); });
        bench("BST (unbalanced)", [this](const string& k) { return this->findBST(k); });
        bench("AVL (balanced)", [this](const string& k) { return this->findAVL(k); });
        bench("Hash (unordered_map)", [this](const string& k) { return this->findHash(k); });
        cout << '\n';
    }
};

// menu

static void displayMenu() {
    cout << "1. Load Data Structures\n";
    cout << "2. Print Course List (alphabetical)\n";
    cout << "3. Print Course\n";
    cout << "4. Validate Prerequisites (missing & cycles)\n";
    cout << "5. Print Topological Order\n";
    cout << "6. Benchmark Searches\n";
    cout << "9. Exit\n";
}

static void printCourseInfo(const CourseCatalog& cat, const string& code) {
    const Course* c = cat.findHash(code); // fast path by default
    if (!c) { cout << "Course not found.\n\n"; return; }
    cout << c->courseNumber << ", " << c->courseTitle << '\n';
    if (c->prerequisites.empty()) {
        cout << "Prerequisites: None\n\n";
    }
    else {
        cout << "Prerequisites: ";
        for (size_t i = 0; i < c->prerequisites.size(); ++i) {
            cout << c->prerequisites[i] << (i + 1 < c->prerequisites.size() ? ", " : "");
        }
        cout << "\n\n";
    }
}

static void processMenu(CourseCatalog& catalog) {
    while (true) {
        displayMenu();
        cout << "What would you like to do? ";
        int choice;
        if (!(cin >> choice)) return;
        cout << '\n';

        switch (choice) {
        case 1: {
            catalog.loadAll(); // default
            cout << '\n';
            break;
        }
        case 2:
            if (!catalog.loaded) { cout << "Load data first.\n\n"; break; }
            catalog.bst.printInOrder();
            cout << '\n';
            break;
        case 3: {
            if (!catalog.loaded) { cout << "Load data first.\n\n"; break; }
            cout << "What course would you like to know about? ";
            string code; cin >> code; code = toUpper(code);
            cout << '\n';
            printCourseInfo(catalog, code);
            break;
        }
        case 4: {
            if (!catalog.loaded) { cout << "Load data first.\n\n"; break; }
            bool cyc = catalog.hasCycle();
            if (!catalog.missingPrereqs.empty()) {
                cout << "Missing prerequisite references detected (" << catalog.missingPrereqs.size() << ").\n";
            }
            else {
                cout << "No missing prerequisite references.\n";
            }
            cout << (cyc ? "Cycle detected in prerequisites (invalid DAG).\n\n"
                : "No cycles detected (valid DAG).\n\n");
            break;
        }
        case 5: {
            if (!catalog.loaded) { cout << "Load data first.\n\n"; break; }
            bool ok = false;
            auto order = catalog.topoOrder(ok);
            if (!ok) {
                cout << "Cannot print topological order: cycle(s) present.\n\n";
            }
            else {
                cout << "Valid course order (prereqs first):\n\n";
                for (const auto& c : order) cout << c << '\n';
                cout << '\n';
            }
            break;
        }
        case 6:
            catalog.benchmarkSearches(); // uses current data
            break;
        case 9:
            cout << "Thank you for using the course planner!\n\n";
            return;
        default:
            cout << choice << " isn't a choice! Try again.\n\n";
        }
    }
}

// main

int main() {
    cout << "Welcome to the course planner.\n";
    CourseCatalog catalog;
    processMenu(catalog);
    return 0;
}
