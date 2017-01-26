#ifndef NITROKEYAPP_THREADWORKER_H
#define NITROKEYAPP_THREADWORKER_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QString>
#include <QVariant>
#include <QMap>
#include <functional>

namespace ThreadWorkerNS {
  using Data = QMap<QString, QVariant>;
  class Worker : public QObject
  {
  Q_OBJECT
  public:
    QMutex mutex;
  public slots:
    void fetch_data() {
      auto data = datafunc();
      emit finished(data);
    }

  signals:
    void finished(Data data);
  private:
    std::function<Data()> datafunc;
  };
}


class ThreadWorker : public QObject {
using namespace ThreadWorkerNS;
Q_OBJECT
Q_DISABLE_COPY(ThreadWorker)


public:
  ThreadWorker(){
    connect(&worker_thread, SIGNAL(started()), &worker, SLOT(fetch_data()));
    connect(&worker, SIGNAL(finished(bool)), this, SLOT(use_data(Data)));
    worker.moveToThread(&worker_thread);
    worker_thread.start();
  }
  ~ThreadWorker(){
    stop_thread();
  }
  void use_data(Data data){

  };

  void stop_thread(){
    worker_thread.quit();
    worker_thread.wait();
  }
private:
  Worker worker;
  QThread worker_thread;

};


#endif //NITROKEYAPP_THREADWORKER_H
