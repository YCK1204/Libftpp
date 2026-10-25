#include "IJobs.hpp"

IJobs::~IJobs() {}

Job::Job(std::function<void()> func) : m_func(func) {}
void Job::execute() { this->m_func(); }