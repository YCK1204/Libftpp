#include "memento.hpp"

Memento::Snapshot Memento::save() {
    Snapshot s;

    _saveToSnapshot(s);
    return s;
}

void Memento::load(const Memento::Snapshot &state) {
    Snapshot s = state;
    _loadFromSnapshot(s);
}
Memento::~Memento() {}
