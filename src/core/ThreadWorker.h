#ifndef NITROKEYAPP_THREADWORKER_H
#define NITROKEYAPP_THREADWORKER_H

#include <QObject>
#include <QMutex>
#include <QThread>

namespace ThreadWorkerNS {
  class Worker : public QObject
  {
  Q_OBJECT
  public:
    QMutex mutex;
//    struct {
//      } data;
  public slots:
    void fetch_data();
  signals:
    void finished(bool connected);
  };
}


class ThreadWorker : public QObject {
Q_OBJECT
Q_DISABLE_COPY(ThreadWorker)


public:

private:
  ThreadWorkerNS::Worker worker;
  QThread worker_thread;

};


#endif //NITROKEYAPP_THREADWORKER_H
