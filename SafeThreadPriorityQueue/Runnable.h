#ifndef RUNNABLE_H
#define RUNNABLE_H


class Runnable {
 public:
  virtual ~Runnable() {}
  virtual void Do() = 0;
};

#endif // RUNNABLE_H
