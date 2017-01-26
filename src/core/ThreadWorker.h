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
#include <QDebug>


//typedef QMap<QString, QVariant> Data;
//using Data = QVariant; //< QMap<QString, QVariant> >;
//Q_DECLARE_METATYPE(Data)

namespace ThreadWorkerNS {
  class Worker : public QObject
  {
    Q_OBJECT
    Q_DISABLE_COPY(Worker)
    public:
    Worker(QObject *parent, const std::function<QMap<QString, QVariant>()> &datafunc) :
        QObject(parent),
        datafunc(datafunc) {}

  public slots:
      void fetch_data() {
        QMutexLocker lock(&mutex);
        auto data = datafunc();
        emit finished();
        emit finished(data);
      }

    signals:
      void finished(QMap<QString, QVariant> data);
      void finished();
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


    connect(&worker, SIGNAL(finished()), this, SLOT(worker_finished()), Qt::QueuedConnection);
    connect(&worker_thread, SIGNAL(started()), &worker, SLOT(fetch_data()), Qt::QueuedConnection);
    connect(&worker, SIGNAL(finished(QMap<QString, QVariant>) ),
            this, SLOT( use_data(QMap<QString, QVariant>) ), Qt::QueuedConnection);
    worker.moveToThread(&worker_thread);
    worker_thread.start();
    mutex.lock();
  }

  ~ThreadWorker(){
    stop_thread();
  }

  void stop_thread(){
    worker_thread.quit();
    worker_thread.wait();
  }

  void wait(){
//    QMutexLocker lock(&mutex);
    mutex.lock();
    mutex.unlock();
  }

  private slots:
  void worker_finished(){
    qDebug() << "worker finished";
  };
  void use_data(QMap<QString, QVariant> data){
//    QMutexLocker lock(&mutex);
    usefunc(data);
    mutex.unlock();
  };

private:
  Worker worker;
  QThread worker_thread;
  std::function<void(QMap<QString, QVariant>)> usefunc;
  QMutex mutex;
};


#endif //NITROKEYAPP_THREADWORKER_H
