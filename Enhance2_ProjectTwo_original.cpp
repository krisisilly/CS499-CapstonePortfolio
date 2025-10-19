// ProjectTwo.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Kris Marchevka
// CS-300-Data Structures & Algorithms: Analysis & Design
// Professor Bhandari

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

class Course { // holds course info
public:
    string courseNumber;
    string courseTitle;
    vector<string> prerequisites;

    Course() {} // default constructor

    Course(string number, string title, vector<string> prereqs) { // constructor to initialize course data
        courseNumber = number; 
        courseTitle = title;
        prerequisites = prereqs;
    }
};

struct TreeNode { // holds course and pointers to left/right nodes
    Course course;
    TreeNode* left;
    TreeNode* right;
    //constructor initializes node w course and sets child pointers to null
    TreeNode(Course c) : course(c), left(nullptr), right(nullptr) {}
};

class BinarySearchTree { // manages course objects
private: // methods for recursion
    TreeNode* root; // root node

    void inOrder(TreeNode* node);
    TreeNode* insert(TreeNode* node, Course course); // insert course into BST
    TreeNode* search(TreeNode* node, string courseNumber); // search by courseNum in BST

public:
    BinarySearchTree() {
        root = nullptr; // constructor initialize root to null
    }

    void insert(Course course) {
        root = insert(root, course);// recurse insert func starts from root
    }

    void inOrder() { // traverse tree and print courses in order
        cout << "Here is a sample schedule:" << endl;
        cout << endl;
        inOrder(root);
    }

    Course* search(string courseNumber) { // search for course by courseNum returns pointer to course 
        TreeNode* node = search(root, courseNumber); // call recursive search
        if (node != nullptr) {
            return &(node->course); // return pointer to found course
        }
        return nullptr; // returns null if not found
    }
};

TreeNode* BinarySearchTree::insert(TreeNode* node, Course course) { // recurse to insert course in BST
    if (node == nullptr) { // if node is null, make new node with course
        return new TreeNode(course);
    }
    // compare courseNums to decide to go left or right
    if (course.courseNumber < node->course.courseNumber) { // left if course is smaller
        node->left = insert(node->left, course);
    } else {
        node->right = insert(node->right, course); // right if course is larger
    }

    return node; // returns node to link it to the tree
}

void BinarySearchTree::inOrder(TreeNode* node) { // recurse to do in-order traversal in BST
    if (node == nullptr) {
        return; // if node is null, do nothing
    }

    inOrder(node->left); // recursively traverse left subtree
    cout << node->course.courseNumber << ", " << node->course.courseTitle << endl;
    inOrder(node->right); // recursively traverse right subtree
}

TreeNode* BinarySearchTree::search(TreeNode* node, string courseNumber) { 
    // recurse to search for course in BST by courseNumber
    if (node == nullptr || node->course.courseNumber == courseNumber) {
        return node; // base case if node is null or course is found, return node
    }

    if (courseNumber < node->course.courseNumber) { // compares courseNums to go left or right
        return search(node->left, courseNumber); // search in left subtree if course is smaller
    }

    return search(node->right, courseNumber); // search in right subtree if course is larger
}

string trim(const string& str) {
    size_t first = str.find_first_not_of(' '); // finds first non-space char
    if (first == string::npos) { // if all spaces, return empty string
        return "";
    }

    size_t last = str.find_last_not_of(' '); // find last non-space char
    return str.substr(first, (last - first + 1)); // return substring w/o leading/trail spaces
}

static void loadCoursesFromFile(BinarySearchTree& bst) {
    // load courses from CVS into BST
    string filename = "CS 300 ABCU_Advising_Program_Input.csv";

    ifstream file(filename); // opens csv
    if (!file.is_open()) {
        cout << "Error opening file." << endl; // error handling if unable to open
        return;
    }

    string line;
    while (getline(file, line)) { // loops through each line in csv
        stringstream ss(line);
        string courseNumber, courseTitle, prereq;
        vector<string> prerequisites;

        // read courseNumber and title
        getline(ss, courseNumber, ',');
        courseNumber = trim(courseNumber);

        getline(ss, courseTitle, ',');
        courseTitle = trim(courseTitle);

        // read all prereqs
        while (getline(ss, prereq, ',')) {
            prereq = trim(prereq);
            if (!prereq.empty()) {
                prerequisites.push_back(prereq);
            }
        }

        // create course and insert into BST
        Course course(courseNumber, courseTitle, prerequisites);
        bst.insert(course);
    }

    file.close();
    cout << "Courses loaded." << endl; // confirms course loading
}

static string toUpper(string str) {
    // convert user input to uppercase
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str; // returns uppercase string
}

// display menu
static void displayMenu() {
    cout << "1. Load Data Structure." << endl;
    cout << "2. Print Course List." << endl;
    cout << "3. Print Course." << endl;
    cout << "9. Exit" << endl;
}

//process user input and menu option choices
static void processMenu(BinarySearchTree& bst) {
    int choice;
    string filename, courseNumber;

    while (true) { // loops until user exits
        displayMenu();
        cout << "What would you like to do? ";
        cin >> choice;
        cout << endl;

        switch (choice) {
        case 1:
            loadCoursesFromFile(bst); // loads fixed csv
            cout << endl;
            break;
        case 2:
            bst.inOrder(); // prints courses in alphanumeric order
            cout << endl;
            break;
        case 3: {
            cout << "What course would you like to know about? ";
            cin >> courseNumber;
            courseNumber = toUpper(courseNumber); // converts to uppercase for uniformity
            cout << endl;
            Course* course = bst.search(courseNumber);
            if (course) {
                cout << course->courseNumber << ", " << course->courseTitle << endl;
                if (course->prerequisites.empty()) {
                    cout << "Prerequisites: None" << endl;
                } else {
                    cout << "Prerequisites: ";
                    for (size_t i = 0; i < course->prerequisites.size(); i++) {
                        cout << course->prerequisites[i];
                        if (i < course->prerequisites.size() - 1) {
                            cout << ", ";
                        }
                    }
                    cout << endl;
                }
            }
            else {
                cout << "Course not found." << endl;
            }
            cout << endl;
            break;
        }
        case 9:
            cout << "Thank you for using the course planner!" << endl;
            cout << endl;
            return; // exit
        default:
            cout << choice << " isn't a choice! Try again." << endl;
            cout << endl;
        }
    }
}

int main()
{
    BinarySearchTree bst; // creates empty bst
    cout << "Welcome to the course planner." << endl;
    processMenu(bst); // starts user menu
    return 0;
}

