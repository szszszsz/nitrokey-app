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
using Data = QMap<QString, QVariant>;
//Q_DECLARE_METATYPE(Data)

namespace ThreadWorkerNS {
  class Worker : public QObject
  {
    Q_OBJECT
    Q_DISABLE_COPY(Worker)
    public:
    Worker(QObject *parent, const std::function<Data()> &datafunc) :
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
      std::function<Data()> datafunc;
    };

}

class ThreadWorker : public QObject {
using Worker = ThreadWorkerNS::Worker;
Q_OBJECT
Q_DISABLE_COPY(ThreadWorker)

public:

  ThreadWorker(const std::function<Data()> &datafunc,
               const std::function<void(Data)> &usefunc, QObject *parent=nullptr) :
      QObject(parent),
        worker(nullptr, datafunc),
        usefunc(usefunc){

    connect(&worker, SIGNAL(finished()), this, SLOT(worker_finished()), Qt::QueuedConnection);
    connect(&worker_thread, SIGNAL(started()), &worker, SLOT(fetch_data()), Qt::QueuedConnection);
    connect(&worker, SIGNAL(finished(QMap<QString, QVariant>) ),
            this, SLOT( use_data(QMap<QString, QVariant>) ), Qt::QueuedConnection);
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
  void worker_finished(){
    qDebug() << "worker finished";
  };
  void use_data(QMap<QString, QVariant> data){
    QMutexLocker lock(&mutex);
    usefunc(data);
  };

private:
  Worker worker;
  QThread worker_thread;
  std::function<void(Data)> usefunc;
  QMutex mutex;
};


#endif //NITROKEYAPP_THREADWORKER_H
