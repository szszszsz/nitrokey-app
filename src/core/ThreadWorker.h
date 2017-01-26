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


//typedef QMap<QString, QVariant> Data;
//using Data = QVariant; //< QMap<QString, QVariant> >;
//Q_DECLARE_METATYPE(Data)

namespace ThreadWorkerNS {
  class Worker : public QObject
  {
    Q_DISABLE_COPY(Worker)
    Q_OBJECT
    public:
    Worker(QObject *parent, const std::function<QMap<QString, QVariant>()> &datafunc) :
        QObject(parent),
        datafunc(datafunc) {}

  public slots:
      void fetch_data() {
        QMutexLocker lock(&mutex);
        auto data = datafunc();
        emit finished(data);
      }

    signals:
      void finished(QMap<QString, QVariant> data);
    private:
      QMutex mutex;
      std::function<QMap<QString, QVariant>()> datafunc;
    };

}
//Q_DECLARE_METATYPE(ThreadWorkerNS::Data)


class ThreadWorker : public QObject {
using Worker = ThreadWorkerNS::Worker;
//using Data = ThreadWorkerNS::Data;
Q_OBJECT
Q_DISABLE_COPY(ThreadWorker)

public:

  ThreadWorker(const std::function<QMap<QString, QVariant>()> &datafunc,
               const std::function<void(QMap<QString, QVariant>)> &usefunc, QObject *parent=nullptr) :
      QObject(parent),
        worker(nullptr, datafunc),
        usefunc(usefunc){


    connect(&worker_thread, SIGNAL(started()), &worker, SLOT(fetch_data()));
    connect(&worker, SIGNAL(finished(QMap<QString, QVariant>) ), this, SLOT( use_data(QMap<QString, QVariant>) ));
    worker.moveToThread(&worker_thread);
    worker_thread.start();
  }

  ~ThreadWorker(){
    stop_thread();
  }

  void stop_thread(){
    worker_thread.quit();
    worker_thread.wait();
  }

  private slots:
  void use_data(QMap<QString, QVariant> data){
    usefunc(data);
  };

private:
  Worker worker;
  QThread worker_thread;
  std::function<void(QMap<QString, QVariant>)> usefunc;
};


#endif //NITROKEYAPP_THREADWORKER_H
