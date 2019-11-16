#include "event_handler.h"

int EventHandler::Init(int wait_time, int size) {
  event_fd_ = epoll_create(size);
  if (event_fd_ < 0) {
    return -1;
  }
  wait_time_ = wait_time;
  return 0;
}

int EventHandler::AddSocket(int socket_fd,
                            std::function<void(EventHandler*)> call_back) {
  auto it = socket_fds_.find(socket_fd);
  if (it != socket_fds_.end()) {
    it->second = call_back;
    return 0;
  }
  printf("Add fd = %d\n", socket_fd);
  SetNoBlock(socket_fd);
  epoll_event ev;
  ev.data.fd = socket_fd;
  ev.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(event_fd_, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
    // Error
    return -1;
  }
  socket_fds_[socket_fd] = call_back;
  Event empty_event;
  empty_event.it = event_table_.end();
  sockets_event_[socket_fd] = empty_event;
  return 0;
}

int EventHandler::RemoveSocket(int socket_fd) {
  if (epoll_ctl(event_fd_, EPOLL_CTL_DEL, socket_fd, nullptr) == -1) {
    return -1;
  }
  socket_fds_.erase(socket_fd);
  sockets_event_.erase(socket_fd);
  return 0;
}

void EventHandler::Run() {
  // pre run check
  if (event_fd_ <= 0) {
    return;
  }
  while (true) {
    int wait_time = GetWaitTime();
    int ret = epoll_wait(event_fd_, data_events_, kEventSize, wait_time);
    if (ret < 0) {
      printf("Epoll get error");
      return;
    }
    // printf("Get %d event\n", ret);
    HandleSocket(ret);
    HandleEvent();
  }
}

int EventHandler::GetWaitTime() {
  if (event_table_.empty()) {
    return wait_time_;
  }
  auto dur = event_table_.begin()->first - local_clock::now();
  return std::max(
      0,
      int(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()));
}

void EventHandler::HandleSocket(int ret) {
  for (int i = 0; i < ret; ++i) {
    auto it = socket_fds_.find(data_events_[i].data.fd);
    if (it == socket_fds_.end()) {
      // TODO: print log
      printf("[Error] Cannot get fd\n");
      continue;
    }
    it->second(this);
  }
}

void EventHandler::HandleEvent() {
  while (!event_table_.empty()) {
    auto now_time = local_clock::now();
    auto it = event_table_.begin();
    if (it->first > now_time) {
      break;
    }
    int fd = it->second;
    event_table_.erase(it);
    auto event_it = sockets_event_.find(fd);
    if (event_it == sockets_event_.end()) {  // Unlikely
      // Fatal error
      return;
    }
    event_it->second.it = event_table_.end();
    event_it->second.cb(this);
  }
}

void EventHandler::AddEvent(int fd, local_clock::time_point next_time,
                            EventCB cb) {
  auto ret = event_table_.insert(std::make_pair(next_time, fd));
  if (!ret.second) {
    // Fatal error
    return;
  }
  auto event_it = sockets_event_.find(fd);
  if (event_it == sockets_event_.end()) {
    // Unlikely, Fatal error
    return;
  }
  event_it->second.it = ret.first;
  event_it->second.cb = cb;
}

void EventHandler::DeleteEvent(int fd) {
  auto event_it = sockets_event_.find(fd);
  if (event_it == sockets_event_.end()) {
    // log: wrong fd
    return;
  }
  if (event_it->second.it != event_table_.end()) {
    event_table_.erase(event_it->second.it);
  }
  event_it->second.it = event_table_.end();
}