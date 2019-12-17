#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
using namespace std;

//prototype functions
void add_new_process_to_the_waitlist(int present_time);
string time_format(int present_time);
void assign_memory_to_the_process(int present_time);
void remove_process(int present_time);
void display_turnaround_times();

//initializations
const int max_time = 100000, max_memory_size = 30000;
int num_of_processes = 0, size_of_page = 0, memory_size = 0, last_announcement = -1;
string file_name = "";

//classes, structs and functions
class PROCESS {
public:
    int pid, arriving_time, burst_time, request_memory_size, time_added_to_memory, is_active, time_finished;
};
struct FRAME {
    int assigned, proc_assign, page_number;
};
struct frame_list {
    vector<FRAME> frames;
    int entries, size_of_page;
};
frame_list create_frame_list(int entries, int size_of_page) {
    frame_list f;
    f.frames.resize(entries);
    f.size_of_page = size_of_page;
    f.entries = entries;
    for (int i = 0; i < f.entries; i++) {
        f.frames[i].assigned = 0;
        f.frames[i].page_number = 0;
        f.frames[i].proc_assign = 0;
    }
    return f;
}
void print_frame_list(frame_list list) {
    bool in_free_block = false;
    int start = 0;
    cout << "\tMemory map:" << endl;
    for (int i = 0; i < list.entries; i++) {
        if (!in_free_block && !list.frames[i].assigned) {
            in_free_block = true;
            start = i;
        }
        else if (in_free_block && list.frames[i].assigned) {
            in_free_block = false;
            cout << "\t\t" << start * list.size_of_page << "-" << (i * list.size_of_page) - 1 << ": Free frames\n";
        }
        if (list.frames[i].assigned) {
            cout << "\t\t" << i * list.size_of_page << "-" << ((i + 1) * list.size_of_page) - 1 << ": Process" << list.frames[i].proc_assign << ", Page " << list.frames[i].page_number << endl;
        }
    }
    if (in_free_block) {
        cout << "\t\t" << start * list.size_of_page << "-" << ((list.entries) * list.size_of_page) - 1 << ": Free frames" << endl;
    }
}
frame_list free_memory_for_pid(frame_list list, int pid) {
    for (int i = 0; i < list.entries; i++) {
        if (list.frames[i].proc_assign == pid) {
            list.frames[i].proc_assign = 0;
            list.frames[i].page_number = 0;
            list.frames[i].assigned = 0;
        }
    }
    return list;
}
int empty_frame_list(frame_list list) {
    for (int i = 0; i < list.entries; i++) {
        if (list.frames[i].assigned) {
            return 0;
        }
    }
    return 1;
}
int acceptable_memory_check(frame_list list, PROCESS proc) {
    int num_free_frames = 0;
    for (int i = 0; i < list.entries; i++) {
        if (!list.frames[i].assigned) {
            num_free_frames++;
        }
    }
    return (num_free_frames * list.size_of_page) >= proc.request_memory_size;
}
frame_list add_process_to_queue(frame_list list, PROCESS proc) {
    int remaining_mem, current_page = 1;
    remaining_mem = proc.request_memory_size;
    for (int i = 0; i < list.entries; i++) {
        if (!list.frames[i].assigned) {
            list.frames[i].assigned = 1;
            list.frames[i].page_number = current_page;
            list.frames[i].proc_assign = proc.pid;
            current_page++;
            remaining_mem -= list.size_of_page;
        }
        if (remaining_mem <= 0) {
            break;
        }
    }
    return list;
}
struct process_queue {
    int capacity, size, front, tail;
    vector<PROCESS> elements;
};
process_queue make_a_wait_queue(int length) {
    process_queue q;
    q.elements.resize(length);
    q.size = 0;
    q.capacity = length;
    q.front = 0;
    q.tail = -1;
    return q;
}
process_queue add_process_to_queue(process_queue q, PROCESS proc) {
    if (q.size == q.capacity) {
        cout << "the queue is full!" << endl;
        exit(2);
    }
    q.size++;
    q.tail++;
    if (q.tail == q.capacity) {
        q.tail = 0;
    }
    q.elements[q.tail] = proc;
    return q;
}
PROCESS peek_queue_at_index(process_queue q, int index) {
    return q.elements[index];
}
int iterate_queue_index(process_queue q, int index) {
    return q.front + index;
}
void display_process_queue(process_queue q) {
    PROCESS proc;
    cout << "\tInput queue: [ ";
    for (int i = 0; i < q.size; i++) {
        proc = peek_queue_at_index(q, iterate_queue_index(q, i));
        cout << proc.pid << " ";
    }
    cout << "]" << endl;
}
int has_next_element(process_queue q) {
    return q.size == 0 ? 0 : 1;
}
process_queue delete_process_for_queue(process_queue q, int index) {
    int prev = 0;
    for (int i = 0; i < q.size; i += 1) {
        if (i > index) {
            q.elements[prev] = q.elements[i];
        }
        prev = i;
    }
    q.size--;
    q.tail--;
    return q;
}
vector<PROCESS> processList;
process_queue waitList;
frame_list frameList;
void clock_loop() {
    long present_time = 0;
    while (1) {
        add_new_process_to_the_waitlist(present_time);
        remove_process(present_time);
        assign_memory_to_the_process(present_time);
        present_time++;
        if (present_time > max_time) {
            printf("deadlock\n");
            break;
        }
        if (waitList.size == 0 && empty_frame_list(frameList)) {
            break;
        }
    }
    display_turnaround_times();
}
void read_input() {
    while (true) {
        cout << "enter memory size (0 - 30000): "; cin >> memory_size;
        cout << "enter page size: "; cin >> size_of_page;
        if (memory_size > 0 && size_of_page > 0 && (memory_size) % (size_of_page) == 0 && memory_size <= max_memory_size)
            break;
        cout << "Invalid input! " << endl;
    }
}
void read_the_data() {
    cout << "enter file name: "; cin >> file_name;
    ifstream myFile;
    myFile.open(file_name);
    if (!myFile) {
        perror("file cannot be opened");
        read_the_data();
    }
    if (myFile.is_open()) {
        myFile >> num_of_processes;
        if (myFile.fail()) {
            perror("input file error number of processes");
            exit(-1);
        }
        processList.resize(num_of_processes);
        for (int i = 0; i < num_of_processes; i++) {
            myFile >> processList[i].pid;
            if (myFile.fail()) {
                perror("input file error id number");
                exit(-1);
            }
            myFile >> processList[i].arriving_time >> processList[i].burst_time;
            if (myFile.fail()) {
                perror("input file error arriving time / life time");
                exit(-1);
            }
            int memory_request_num = 0, memory_request_size[10000] = { 0 }, sum = 0;
            myFile >> memory_request_num;
            if (myFile.fail()) {
                perror("memory size error");
                exit(-1);
            }
            for (int j = 0; j < memory_request_num; j++) {
                myFile >> memory_request_size[j];
                sum += memory_request_size[j];
            }
            processList[i].request_memory_size = sum;
            processList[i].is_active = 0;
            processList[i].time_added_to_memory = -1;
            processList[i].time_finished = -1;
        }
    }
    myFile.close();
}
void add_new_process_to_the_waitlist(int present_time) {
    PROCESS proc;
    for (int i = 0; i < num_of_processes; i++) {
        proc = processList[i];
        if (proc.arriving_time == present_time) {
            string print_time = time_format(present_time);
            cout << print_time << "Process " << proc.pid << " arrives" << endl;
            waitList = add_process_to_queue(waitList, proc);
            display_process_queue(waitList);
        }
    }
}
string time_format(int present_time) {
    string result = "";
    if (last_announcement == present_time) {
        result = "\t";
    }
    else {
        result = "t = " + to_string(present_time) + "\t";
    }
    last_announcement = present_time;
    return result;
}
void remove_process(int present_time) {
    int i, time_spent_in_memory;
    for (i = 0; i < num_of_processes; i++) {
        time_spent_in_memory = present_time - processList[i].time_added_to_memory;
        if (processList[i].is_active && (time_spent_in_memory >= processList[i].burst_time)) {
            cout << time_format(present_time) << "Process " << processList[i].pid << " completes" << endl;
            processList[i].is_active = 0;
            processList[i].time_finished = present_time;
            frameList = free_memory_for_pid(frameList, processList[i].pid);
            print_frame_list(frameList);
        }
    }
}
void display_turnaround_times() {
    int i;
    float total = 0;
    for (i = 0; i < num_of_processes; i += 1) {
        total += processList[i].time_finished - processList[i].arriving_time;
    }
    cout << "Avg turnaround time: " << total / num_of_processes << endl;
}
void assign_memory_to_the_process(int present_time) {
    int index, limit;
    PROCESS proc;
    limit = waitList.size;
    for (int i = 0; i < limit; i++) {
        index = iterate_queue_index(waitList, i);
        proc = waitList.elements[index];
        if (acceptable_memory_check(frameList, proc)) {
            cout << time_format(present_time) << "MM moves Process " << proc.pid << " to memory" << endl;
            frameList = add_process_to_queue(frameList, proc);
            for (int j = 0; j < num_of_processes; j++) {
                if (processList[j].pid == proc.pid) {
                    processList[j].is_active = 1;
                    processList[j].time_added_to_memory = present_time;
                    waitList = delete_process_for_queue(waitList, i);
                }
            }
            display_process_queue(waitList);
            print_frame_list(frameList);
        }
    }
}

//main
int main() {
    read_input();
    read_the_data();
    waitList = make_a_wait_queue(num_of_processes);
    frameList = create_frame_list(memory_size / size_of_page, size_of_page);
    clock_loop();
    return 0;
}
