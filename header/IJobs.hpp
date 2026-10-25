#ifndef IJOBS_HPP
#define IJOBS_HPP

#include <functional>
class IJobs {
public:
  virtual ~IJobs();
  virtual void execute() = 0;
};

#pragma region Job
class Job : public IJobs {
private:
  std::function<void()> m_func;

public:
  Job(std::function<void()> func);
  void execute() override;
};
#pragma endregion

#endif