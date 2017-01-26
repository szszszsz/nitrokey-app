#ifndef NITROKEYAPP_THREADWORKER_H
#define NITROKEYAPP_THREADWORKER_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QString>
#include <QVariant>
#include <QMap>
#include <functional>
#include <QMutexLocker>

namespace ThreadWorkerNS {
  using Data = QMap<QString, QVariant>;
  class Worker : public QObject
  {
    Q_DISABLE_COPY(Worker)
    Q_OBJECT
    public:
    Worker(QObject *parent, const std::function<Data()> &datafunc) :
        QObject(parent),
        datafunc(datafunc) {}

  public slots:
      void fetch_data() {
        QMutexLocker lock(&mutex);
        auto data = datafunc();
        emit finished(data);
      }

    signals:
      void finished(Data data);
    private:
      QMutex mutex;
      std::function<Data()> datafunc;
    };
}


class ThreadWorker : public QObject {
using Worker = ThreadWorkerNS::Worker;
using Data = ThreadWorkerNS::Data;
Q_OBJECT
Q_DISABLE_COPY(ThreadWorker)

public:

  ThreadWorker(const std::function<Data()> &datafunc,
               const std::function<void(Data)> &usefunc, QObject *parent=nullptr) :
      QObject(parent),
        worker(this, datafunc),
        usefunc(usefunc){
    connect(&worker_thread, SIGNAL(started()), &worker, SLOT(fetch_data()));
    connect(&worker, SIGNAL(finished(Data)), this, SLOT(use_data(Data)));
    worker.moveToThread(&worker_thread);
    worker_thread.start();
  }

  ~ThreadWorker(){
    stop_thread();
  }
  void use_data(Data data){
    usefunc(data);
  };

  void stop_thread(){
    worker_thread.quit();
    worker_thread.wait();
  }
private:
  Worker worker;
  QThread worker_thread;
  std::function<void(Data)> usefunc;
};


#endif //NITROKEYAPP_THREADWORKER_H
