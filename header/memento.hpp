#ifndef MEMENTO_HPP
#define MEMENTO_HPP

#include "data_buffer.hpp"

class Memento {
public:
  class Snapshot : public DataBuffer {
  };

  Snapshot save();
  void load(const Memento::Snapshot &state);
  virtual ~Memento();

private:
  virtual void _saveToSnapshot(Memento::Snapshot &snapshot) const = 0;
  virtual void _loadFromSnapshot(Memento::Snapshot &snapshot) = 0;
};

#endif