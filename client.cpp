#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <csignal>

volatile int paused = 0;

void handle_sigusr1(int) {
    paused = 1;
}

void handle_sigusr2(int) {
    paused = 0;
}

void handle_sigterm(int){

}

int connect_to_server(){
    const char* socket_path = "/tmp/simple_socket";
    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        return -1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        return -1;
    }

    return client_fd;
}

int main() {

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    signal(SIGTERM, handle_sigterm);



    int client_fd = connect_to_server();
    if (client_fd == -1) {
        return 1;
    }

    int pid = getpid();

    int mem_fd;
    ssize_t bytes_read;
    do{
        write(client_fd, &pid, sizeof(pid));
        bytes_read= read(client_fd, &mem_fd, sizeof(mem_fd));
    }while(bytes_read != sizeof(mem_fd));


    while(paused == 0){

        // DO SOMETHING WITH KVS

    }



    close(client_fd);
    return 0;
}
