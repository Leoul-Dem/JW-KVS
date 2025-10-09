#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "concurrent_hashmap.cpp"
#include <iostream>
#include <vector>
#include <atomic>
#include <csignal>

volatile int curr_using = 0;
std::atomic<bool> quit = false;

std::vector<int> pid;

void handle_sigint(int){
    for(auto i : pid){
        kill(i, SIGTERM);
    }
    quit = true;
}

void handle_resizing(){
    
}


int main() {
    signal(SIGINT, handle_sigint);

    const char* socket_path = "/tmp/simple_socket";

    // Create socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    // Remove any existing socket file
    unlink(socket_path);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // Bind socket to a file path
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    std::cout << "Server listening on " << socket_path << "\n";
    std::vector<int>client_fd;

    while (!quit) {
        client_fd.push_back( accept(server_fd, nullptr, nullptr));
        if (client_fd.back() == -1) {
            perror("accept");
            client_fd.erase(client_fd.begin() + client_fd.size()-1);
            return -1;
        }else{
            curr_using++;
        }

        int value;
        ssize_t bytes_read = read(client_fd.back(), &value, sizeof(value));
        while(bytes_read != sizeof(value)){
            int error_flag = -1;
            write(client_fd.back(), &error_flag, sizeof(error_flag));
            bytes_read = read(client_fd.back(), &value, sizeof(value));
        }
        pid.push_back(value);

        int some_addr = 1234;
        write(client_fd.back(), &some_addr, sizeof(some_addr));
    }



    close(server_fd);
    unlink(socket_path);



    return 0;
}


