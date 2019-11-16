/**
 * @file event_handler.h
 * @author SWH
 * @brief Using epoll to handle event
 * @version 0.1
 * @date 2019-11-09
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef __EVENT_HANDLER_H__
#define __EVENT_HANDLER_H__

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <functional>
#include <unordered_map>
#include <map>
#include <chrono>

#define kEventSize 16

using local_clock = std::chrono::steady_clock;
//注意： 类设计的时候不考虑线程安全。 设计时认为应当只有一个线程在运行
class EventHandler {
 public:
  int Init(int wait_time = 20, int size = 256);
  int AddSocket(int socket_fd, std::function<void(EventHandler*)> call_back);
  int RemoveSocket(int socket_fd);
  void AddEvent(int fd, local_clock
::time_point next, std::function<void(EventHandler*)> cb);
  void DeleteEvent(int fd);
  void Run();
 private:
  void HandleEvent();
  int GetWaitTime();
  void HandleSocket(int num);
  int wait_time_;
  struct epoll_event event_;
  epoll_event data_events_[kEventSize];
  int event_fd_;
  std::unordered_map<int, std::function<void(EventHandler*)> > socket_fds_;
  std::map<local_clock::time_point, int> event_table_;
  struct Event{
    std::map<local_clock::time_point, int>::iterator it;
    std::function<void(EventHandler*) > cb;
  };
  std::unordered_map<int, Event> sockets_event_;//Must keep the iterator valid
};

using EventCB = std::function<void(EventHandler*)>;

/**
 * @brief Set the No Block object
 *
 * @param socket_fd
 * @return int 0 success -1 error
 */
inline int SetNoBlock(int socket_fd) {
  int opts;
  opts = fcntl(socket_fd, F_GETFL);
  if (opts < 0) {
    return -1;
  }
  opts = opts | O_NONBLOCK;
  if (fcntl(socket_fd, F_SETFL, opts)) {
  }
  return 0;
}

#endif  //__EVENT_HANDLER_H__