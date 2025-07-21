#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include "leftwidget.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
#include <QImage>
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString path;

    LeftWidget * m_lw;
public slots:
    void onStart();
    void onAnimate();
    void onFinish();
    void onLoad();
public slots:
    void onRadiusChanged(int v);
    void onStrokeChanged(int v);
    void onAngleChanged(int v);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
