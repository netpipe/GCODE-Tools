#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "svgreader.h"
#include "leftwidget.h"
#include <QFileDialog>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->btn_svg, SIGNAL(clicked()), this , SLOT(onLoad()));
    QObject::connect(ui->btn_start, SIGNAL(clicked()), this , SLOT(onStart()));
    QObject::connect(ui->btn_move, SIGNAL(clicked()), this , SLOT(onAnimate()));
    QObject::connect(ui->btn_finish, SIGNAL(clicked()), this , SLOT(onFinish()));


    QObject::connect(ui->sb_radius, SIGNAL(valueChanged(int)), this , SLOT(onRadiusChanged(int)));
    QObject::connect(ui->sb_stroke, SIGNAL(valueChanged(int)), this , SLOT(onStrokeChanged(int)));
    QObject::connect(ui->sb_angle, SIGNAL(valueChanged(int)), this , SLOT(onAngleChanged(int)));


    m_lw = new LeftWidget(this->centralWidget());
    m_lw->setGeometry( ui->lb_left->geometry() );



}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLoad()
{
    QString newPath = QFileDialog::getOpenFileName(this, "Open SVG",
                                                          ".", tr("SVG files (*.svg)"));
    if (newPath.isEmpty())
           return;

    path = newPath;
    // To set a graphic scene objects, received them with a method getElements
    m_lw->m_points = SvgReader::getElements(path);
    m_lw->m_finish = true;
    m_lw->m_animating = false;
    m_lw->update();
}
//when press load button
void MainWindow::onStart()
{
    m_lw->m_points.clear();
    m_lw->m_animating = m_lw->m_finish = false;
    m_lw->update();
}


void MainWindow::onFinish()
{
    m_lw->m_finish = true;
    m_lw->update();

}
//when start animation
void MainWindow::onAnimate()
{

    m_lw->startAnimation();
}

void MainWindow::onRadiusChanged(int v)
{
    m_lw->m_radius = v;
}
void MainWindow::onStrokeChanged(int v)
{
    m_lw->m_stroke = v;
}
void MainWindow::onAngleChanged(int v)
{
    m_lw->m_angle = (float)v * 3.14/180.0;
}
