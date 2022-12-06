#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <iterator>

class Process {
   public:
    // Constructor ----------------------
    Process() {
        m_duration = rand() % 10 + 1;
        m_size = rand() % 250 + 1;
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
    std::cout << "Allocating " << proc.getSize() << "mb of memory" << std::endl;
    for (int i = startIndex; i < startIndex + proc.getSize(); i++) {
        mem[i].allocateTo(proc.getJobNo());
    }
}

void DeallocateMemory(Process proc) {
    std::cout << "Deallocating " << proc.getSize() << "mb of memory"
              << std::endl;
    for (int i = proc.getStartIndex();
         i < proc.getStartIndex() + proc.getSize(); i++) {
        mem[i].deallocate();
    }
}

int main() {
    bool running = true;
    // The processes that are "Running"
    std::deque<Process> processes = std::deque<Process>();
    std::deque<Process> active = std::deque<Process>();
    mem = initializeMemory();

    // Generate the processes
    for (int i = 0; i < 20; i++) {
        processes.push_front(Process());
    }

    std::random_shuffle(processes.begin(), processes.end(),
                        [&](int i) { return std::rand() % i; });

    int currentMemindex = 0;
    // "Run" the processes
    while (running) {
        auto start = std::chrono::steady_clock::now();
        // Get the next process
        auto process = processes.front();
        bool processIsActive = false;

        std::cout << process.getJobNo() << "  " << process.getDuration()
                  << "cycles"
                  << "  " << process.getSize() << "mb" << std::endl;

        // Run Next fit algo
        int index = GetStartIndexOfNextFitBlock(currentMemindex, process);
        if (index != -1) {
            AllocateMemory(index, process);
            process.setStartIndex(index);
            processIsActive = true;
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
                (*p).decrementDuration();
                i++;
            }
        }
        // replacement algo?

        // delay end of loop.
        auto end = std::chrono::steady_clock::now();
        while (((std::chrono::duration<double>)(end - start)).count() < 2) {
            end = std::chrono::steady_clock::now();
        }
        if (processIsActive) {
            processes.pop_front();
            active.push_back(process);
        }
        if (processes.empty()) running = false;
    }
    std::cin.get();
    return 1;
}