/*
 * Program: EECS 348 Assignment 4 -  Recursive Sudoku Solver
 * Author: Samyu Aitha
 * Created: October 14, 2026
 * Input: puzzle1.txt through puzzle5.txt (or filenames supplied as arguments).
 * Output: Original puzzle, every valid solution, or "No solution found".
 * Collaborators: None reported; update this field if applicable.
 * Sources: Claude (initial object-oriented structure), Gemini (comparison)
 * Attribution: Class separation and recursive backtracking adapted from Claude;
 *              input validation, empty-cell indexing, solution streaming.
 */

#include <array>       // for the 9x9 board
#include <cctype>      // need this for isspace
#include <fstream>    // so we can read the puzzle files
#include <iostream>   // used for printing
#include <string>     // filenames and error messages
#include <vector>     // lists of files and blank spots

// board class holds all 81 cells and checks the sudoku rules
class Board {
public:
    static constexpr int SIZE = 9;     // 9 rows and 9 cols
    static constexpr int BOX = 3;      // each box is 3x3
    static constexpr int EMPTY = 0;    // 0 means the cell is blank aka an underscore

    // constructor just sets every cell to 0 to start
    Board() { for (auto& row : cells) row.fill(EMPTY); }

    // reads the puzzle file and makes sure it's exactly 81 valid cells
    bool load(const std::string& filename, std::string& error) {
        std::ifstream input(filename);             // try to open the file
        if (!input) { error = "Cannot open file"; return false; } // file didn't open so save the error and stop
        Board parsed;                              // temp board so we don't mess up the real one if the file is bad
        int count = 0;                             // how many cells we've read so far
        char symbol;                               // current char from the file
        while (input.get(symbol)) {                // keep going till end of file
            if (std::isspace(static_cast<unsigned char>(symbol))) continue; // skip spaces and newlines
            if (symbol != '_' && (symbol < '1' || symbol > '9')) { // only 1 to 9 or underscore allowed
                error = "Unexpected character in puzzle";
                return false;
            }
            if (count == SIZE * SIZE) {            // already have 81 cells so this one is extra and the file is bad
                error = "Puzzle has more than 81 cells";
                return false;
            }
            parsed.cells[count / SIZE][count % SIZE] =
                symbol == '_' ? EMPTY : symbol - '0'; // turn the char into an int
            ++count;                              // move on to the next cell
        }
        if (count != SIZE * SIZE) {                // has to be a full 9x9 grid
            error = "Puzzle must contain exactly 81 cells";
            return false;
        }
        *this = parsed;                            // everything was fine so copy it over
        return true;
    }

    // checks if a number can go here without breaking the row col or box rule
    bool canPlace(int row, int col, int value) const {
        for (int i = 0; i < SIZE; ++i) {           // go through the whole row and col
            if (cells[row][i] == value || cells[i][col] == value) return false; // number is already in the row or col so it can't go here
        }
        const int top = (row / BOX) * BOX;         // top row of the 3x3 box we're in
        const int left = (col / BOX) * BOX;        // left col of the 3x3 box we're in
        for (int r = top; r < top + BOX; ++r) {    // loop over the box rows
            for (int c = left; c < left + BOX; ++c) { // loop over the box cols
                if (cells[r][c] == value) return false; // number already in the box
            }
        }
        return true;                               // returns true since the number isn't anywhere in the row col or box
    }

    // makes sure the starting numbers don't already clash with each other
    bool validGivens() {
        for (int r = 0; r < SIZE; ++r) {           // every row
            for (int c = 0; c < SIZE; ++c) {       // every col
                const int value = cells[r][c];    // save the number that's here
                if (value == EMPTY) continue;     // blanks don't need to be checked so skip them
                cells[r][c] = EMPTY;              // take it out for a sec so it doesn't match itself
                const bool valid = canPlace(r, c, value); // see if it clashes with anything else
                cells[r][c] = value;              // put the number back where it was
                if (!valid) return false;         // found a duplicate so the puzzle is bad
            }
        }
        return true;                               // none of the starting numbers clash so the puzzle is valid
    }

    // getter and setter for a cell the solver uses these a lot
    int get(int row, int col) const { return cells[row][col]; } // returns what's in the cell
    void set(int row, int col, int value) { cells[row][col] = value; } // puts a new value in the cell

    // prints the board with underscores for empty spots
    void print(std::ostream& out) const {
        for (const auto& row : cells) {            // each row
            for (int value : row) {                // each cell in the row
                if (value == EMPTY) out << "_ ";  // blank so print underscore like the input
                else out << value << ' ';          // otherwise just print the number
            }
            out << '\n';                          // new line after each row
        }
    }

private:
    std::array<std::array<int, SIZE>, SIZE> cells; // the actual 9x9 grid
};

// solver class does the recursive backtracking and prints solutions as it finds them
class Solver {
public:
    // constructor copies the board and saves where all the blanks are
    explicit Solver(const Board& initial) : board(initial), solutionCount(0) {
        for (int r = 0; r < Board::SIZE; ++r) {   // go through each row
            for (int c = 0; c < Board::SIZE; ++c) { // and each col
                if (board.get(r, c) == Board::EMPTY) { // if it's blank
                    blanks.push_back({r, c});     // remember where it is
                }
            }
        }
    }

    // solves the puzzle and returns how many solutions there were
    unsigned long long solveAll(std::ostream& out) {
        solutionCount = 0;                        // reset count just in case
        if (!board.validGivens()) return 0;       // starting numbers conflict so no point trying
        search(0, out);                           // start at the first blank
        return solutionCount;                     // return how many we found
    }

private:
    struct Position { int row; int col; };       // just a row and col for a blank cell
    Board board;                                  // the board we try numbers on
    std::vector<Position> blanks;                 // list of all the empty spots
    unsigned long long solutionCount;             // how many solutions printed so far

    // recursive part fills in a blank then undoes it to try the next number
    void search(std::size_t index, std::ostream& out) {
        if (index == blanks.size()) {             // no blanks left so it's solved
            ++solutionCount;                      // add one to the solution count
            out << "Solution " << solutionCount << ":\n"; // label the solution
            board.print(out);                     // print it right away instead of storing it
            out << '\n';                          // blank line between solutions
            return;                               // go back up the recursion and look for more solutions
        }
        const Position pos = blanks[index];       // grab the next blank
        for (int digit = 1; digit <= Board::SIZE; ++digit) { // try 1 through 9
            if (!board.canPlace(pos.row, pos.col, digit)) continue; // skip if it breaks a rule
            board.set(pos.row, pos.col, digit);    // put the number in
            search(index + 1, out);               // move on to the next blank
            board.set(pos.row, pos.col, Board::EMPTY); // undo it so we can try the next number
        }
    }
};

// puzzle runner handles one file at a time loading solving and printing
class PuzzleRunner {
public:
    explicit PuzzleRunner(const std::string& path) : filename(path) {} // just saves the filename

    // runs everything for one puzzle file
    void run(std::ostream& out) const {
        out << "========================================\n"; // divider line at the start
        out << "File: " << filename << '\n';     // show which file we're on
        Board puzzle;                             // new empty board
        std::string error;                        // holds the error msg if something goes wrong
        if (!puzzle.load(filename, error)) {      // if the file couldn't be read or was formatted wrong
            out << "Input error: " << error << "\n\n"; // print what went wrong
            return;
        }
        out << "Original puzzle:\n";           // label for the original puzzle
        puzzle.print(out);                        // print the starting board
        out << '\n';                              // space before the solutions
        Solver solver(puzzle);                    // make a solver with a copy of the board
        const auto count = solver.solveAll(out);  // find and print all the solutions
        if (count == 0) out << "No solution found\n"; // no solutions so let the user know
        else out << "Solutions found: " << count << '\n'; // say how many we got
        out << "========================================\n\n"; // divider line at the end
    }

private:
    std::string filename;                         // the name of the file this runner handles
};

// sudoku app just loops through all the files and runs each one
class SudokuApp {
public:
    explicit SudokuApp(const std::vector<std::string>& paths) : filenames(paths) {} // save the list of files
    // goes through every file and solves it
    void run() const {
        for (const auto& filename : filenames) {  // loop over each file
            PuzzleRunner runner(filename);        // make a runner for this file
            runner.run(std::cout);                 // print the puzzle and solutions
        }
    }
private:
    std::vector<std::string> filenames;           // all the puzzle files in order
};

// main just sets up the files and runs the app
int main(int argc, char* argv[]) {
    std::vector<std::string> files;               // files we want to solve
    if (argc > 1) {                               // if filenames were passed in use those
        for (int i = 1; i < argc; ++i) files.push_back(argv[i]); // add each one
    } else {                                      // otherwise use the default 5 puzzles
        for (int i = 1; i <= 5; ++i) {
            files.push_back("puzzle" + std::to_string(i) + ".txt"); // puzzle1 to puzzle5
        }
    }
    SudokuApp app(files);                         // make the app with the list of files
    app.run();                                    // solve and print every puzzle
    return 0;                                     // end the program
}
