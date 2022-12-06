#include <algorithm>
#include <chrono>
#include <deque>
#include <iterator>
#include <curses.h>
#include <string>
#include <sstream>
#include <random>
#include <iostream>

class Process {
   public:
    // Constructor ----------------------
    Process() {
        m_duration = rand() % 10 + 1;
        m_size = (rand() % 25 + 1) * 10;
        m_jobNo = s_lastJobNo += 1;
    }

    // Functions -----------------------

    const int getDuration() const { return m_duration; }
    const int getSize() const { return m_size; }
    const int getJobNo() const { return m_jobNo; }
    const int getStartIndex() const { return m_startIndex; }

    void decrementDuration() { m_duration -= 1; }
    void setStartIndex(int value) { m_startIndex = value; }

    // Variables ---------------
    static int s_lastJobNo;

   private:
    // Duration of the process in units (iterations of the loop)
    int m_duration;

    // Size of the process in mb?
    int m_size;

    // Job number
    int m_jobNo;

    // Start index of allocated memory
    int m_startIndex = -1;
};

class MemMegaByte {
   public:
    void allocateTo(int jobNo) {
        if (m_allocatedToJob == -1) m_allocatedToJob = jobNo;
    }

    void deallocate() { m_allocatedToJob = -1; }

    bool isAllocated() { return m_allocatedToJob != -1; }

    const int getAllocatedToJob() const { return m_allocatedToJob; }

   private:
    // The job number this block is allocated to, -1 if not allocated.
    int m_allocatedToJob = -1;
};

int Process::s_lastJobNo = 0;

std::deque<MemMegaByte> initializeMemory() {
    std::deque<MemMegaByte> mem = std::deque<MemMegaByte>();

    // Generate a gig of "memory"
    for (int i = 0; i < 1000; i++) {
        mem.push_back(MemMegaByte());
    }

    return mem;
}

std::deque<MemMegaByte> mem;

int GetStartIndexOfNextFitBlock(int currentIndex, Process proc) {
    bool blockFound = false, checkForOverlap = false;
    int start = currentIndex, firstCheckedIndex = currentIndex;
    while (!blockFound) {
        if (checkForOverlap && currentIndex == firstCheckedIndex) {
            return -1;
        } else if (currentIndex >= 1000) {
            currentIndex = 0;
            continue;
        } else if (currentIndex - start == proc.getSize() &&
                   !mem[currentIndex].isAllocated()) {
            blockFound = true;
        } else if (mem[currentIndex].isAllocated()) {
            start = currentIndex += 1;
        }
        currentIndex++;
        checkForOverlap = true;
    }
    return start;
}

void AllocateMemory(int startIndex, Process proc) {
    // std::cout << "Allocating " << proc.getSize() << "mb of memory" << std::endl;
    for (int i = startIndex; i < startIndex + proc.getSize(); i++) {
        mem[i].allocateTo(proc.getJobNo());
    }
}

void DeallocateMemory(Process proc) {
    // std::cout << "Deallocating " << proc.getSize() << "mb of memory"
    //           << std::endl;
    for (int i = proc.getStartIndex();
         i < proc.getStartIndex() + proc.getSize(); i++) {
        mem[i].deallocate();
    }
}

std::string streamToString(std::stringstream& ss) {
  std::string str = "", temp = "";
  while (!ss.eof()) {
    ss >> temp;
    str += temp + " ";
  }
  str = str.substr(0, str.size() - 1);
  ss.str(std::string());
  ss.clear();
  return str;
}

WINDOW *leftWin, *rightWin;
int terminalX, terminalY;

void initNCurses() {
    setlocale(LC_ALL, "");
    initscr(); 
    start_color();

    getmaxyx(stdscr, terminalY, terminalX);
    
    leftWin = newwin(terminalY, terminalX * 0.25, 0, 0);
    rightWin = newwin(terminalY, terminalX * 0.75, 0, (terminalX * 0.25) + 1);
    box(leftWin, 0, 0);
    box(rightWin, 0, 0);
}

void displayProcess(Process process, std::string preText, int y, int x) {
    mvwprintw(leftWin, y, x, "%s%*i      %*iu      %imb", preText.c_str(), 2, process.getJobNo(), 2, process.getDuration(), process.getSize());
}

void initColors() {
    // Empty Block
    init_pair(1, COLOR_WHITE, COLOR_WHITE);

    // Filled blocks
    init_pair(2, COLOR_BLACK, COLOR_RED);
    init_pair(3, COLOR_BLACK, COLOR_GREEN);
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);
    init_pair(5, COLOR_BLACK, COLOR_BLUE);
    init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(7, COLOR_BLACK, COLOR_CYAN);
}

void drawBlocks() {
    std::stringstream ss;
    int i = 0;
    int nextColor = 1;
    int lastId = -1;
    for (int x = 2; x <= 30; x = x + 3) {
        for (int y = 1; y < 11; y++) {
            auto block = mem[i * 10];
            int pair = 1;
            char firstChar = ' ', secondChar = ' ';
            if (block.isAllocated()) {
                std::string str;
                int id = block.getAllocatedToJob();
                if (id != lastId) {
                    nextColor += 1;
                    if (nextColor > 7) nextColor = 2;
                    lastId = id;
                }
                pair = nextColor;
                ss << block.getAllocatedToJob();
                ss >> str;
                ss.clear();
                if (str.length() == 2) {
                    firstChar = (char)str[0];
                    secondChar = (char)str[1];
                } else {
                    firstChar = '0';
                    secondChar = (char)str[0];
                }
            }
            mvwaddch(rightWin, y, x, firstChar | COLOR_PAIR(pair));
            mvwaddch(rightWin, y, x + 1, secondChar | COLOR_PAIR(pair));
            wrefresh(rightWin);
            i++;
        }
    }
}


int main() {
    clear();
    refresh();
    initNCurses();
    initColors();
    wrefresh(leftWin);
    wrefresh(rightWin);

    bool running = true;
    // The processes that are "Running"
    std::deque<Process> processes = std::deque<Process>();
    std::deque<Process> active = std::deque<Process>();
    mem = initializeMemory();

    // Generate the processes
    for (int i = 0; i < 20; i++) {
        processes.push_front(Process());
    }

    std::mt19937 rng(1234);
    std::shuffle(processes.begin(), processes.end(), rng);

    int currentMemindex = 0;
    // "Run" the processes
    drawBlocks();

    while (running) {
        wclear(leftWin);
        box(leftWin, 0, 0);
        auto start = std::chrono::steady_clock::now();
        // Get the next process
        auto process = processes.front();
        bool processIsActive = false;
        
        mvwprintw(leftWin, 1, 1, "        JobNo   Time     Size");
        if (!processes.empty())
            displayProcess(process, "Current ", 2, 1);  
        else 
            mvwprintw(leftWin, 2, 1, "Current");
        mvwprintw(leftWin, 3, 1, "------------------------------");
        mvwprintw(leftWin, terminalY - 2, 1, "Jobs in Queue: %li", processes.size());

        // Run Next fit algo
        if (!processes.empty()) {
            int index = GetStartIndexOfNextFitBlock(currentMemindex, process);
            if (index != -1) {
                AllocateMemory(index, process);
                process.setStartIndex(index);
                processIsActive = true;
            }
        }

        int i = 0;
        while (i < active.size()) {
            auto p = &active[i];
            if ((*p).getDuration() < 1) {
                // remove from deque
                auto iter = active.begin();
                std::advance(iter, i);
                DeallocateMemory(*p);
                active.erase(iter);
            } else {
                displayProcess(*p, "        ", i + 4, 1);
                (*p).decrementDuration();
                i++;
            }
        }
        wclear(rightWin);
        box(rightWin, 0, 0);
        drawBlocks();
        wrefresh(leftWin);

        // delay end of loop.
        auto end = std::chrono::steady_clock::now();
        while (((std::chrono::duration<double>)(end - start)).count() < 2) {
            end = std::chrono::steady_clock::now();
        }
        if (processIsActive) {
            processes.pop_front();
            active.push_back(process);
        }
        if (processes.empty() && active.empty()) running = false;
    }
    endwin();
    delwin(leftWin);
    delwin(rightWin);
    std::cin.get();
    return 1;
}