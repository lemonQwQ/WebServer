#include "epoller.h"

Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)), events_(maxEvent) {
  assert(epollFd_ >= 0 && events_.size() > 0);  
}

Epoller::~Epoller() {
  close(epollFd_);    
}

bool Epoller::AddFd(int fd, uint32_t events) {
  if (fd < 0) return false;
  epoll_event event = {0};
  event.data.fd = fd;
  event.events = events;
  return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event);    
} 

bool Epoller::ModFd(int fd, uint32_t events) {
  if (fd < 0) return false;
  epoll_event event = {0};
  event.data.fd = fd;
  event.events = events;
  return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event);
}

bool Epoller::DelFd(int fd) {
  if (fd < 0) return false;
  return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
}

int Epoller::Wait(int timeoutMs) {
  return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t idx) const {
  assert(idx >= 0 && idx < events_.size());
  return events_[idx].data.fd;
}

uint32_t Epoller::GetEvents(size_t idx) const {
  assert(idx >= 0 && idx < events_.size());
  return events_[idx].events;
}
