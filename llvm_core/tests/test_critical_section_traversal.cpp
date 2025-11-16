// Forward declarations of lock/unlock functions to simulate synchronization primitives.
void mutex_lock();
void mutex_unlock();
void spin_lock();
void spin_unlock();

void locked_function() {
  mutex_lock();
  // ... critical section ...
  mutex_unlock();
}

void another_locked_function() {
  spin_lock();
  // ... critical section ...
  spin_unlock();
}

void mixed_locks() {
  mutex_lock();
  spin_lock();
  // ... critical section ...
  spin_unlock();
  mutex_unlock();
}

void no_locks() {
  // This function does not contain any locks.
}
