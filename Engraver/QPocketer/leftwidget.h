#ifndef LEFTWIDGET_H
#define LEFTWIDGET_H

#include <QObject>
#include <QWidget>
#include <QPaintEvent>
#include <QList>
#include <QPainter>
#include <QTimer>
#include <QtMath>
#include <QGraphicsItem>
#define LEFT -1
#define RIGHT 1

#define HORZ -1
#define VERT 1

class ALine{
public:
    float a, b, c;
    ALine(QPointF s, QPointF e){
        a = s.y() - e.y();
        b  = e.x() - s.x();
        c = s.x()*e.y() - s.y()*e.x();
    }
    ~ALine(){}

    ALine offset(float d, int dir)
    {
        ALine new_line = *this;
        new_line.c += dir * d * qSqrt(a*a + b*b);
        return new_line;
    }
    int pointSide(QPoint p)
    {
        if ( a * p.x() + b * p.y() + c >= 0) return RIGHT;
        else if (a * p.x() + b * p.y() + c == 0) return 0;
        else return LEFT;
    }
    QPointF meetPoint(ALine other)
    {
        float d = other.a * b - other.b * a;
        float x = c * other.b - b* other.c;
        float y = a * other.c - c * other.a;
        return QPointF(x/d, y/d);
    }
    void draw(QPainter & p)
    {
        QPointF xzero = QPointF(0, -c/b);
        QPointF yzero = QPointF(-c/a, 0);

        QPointF xthou = QPointF(1000, (-c - a*1000)/b);

        p.drawLine(xzero, yzero);
        p.drawLine(xzero, xthou);
    }
    float distance(QPointF p)
    {
       return qAbs(a * p.x() + b*p.y() + c) / qSqrt(a*a + b*b);
    }
};
class LeftWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LeftWidget(QWidget *parent = nullptr);
public:

    void paintEvent(QPaintEvent *event);
public:
    QVector<QPointF> m_points;
    bool m_finish;
    bool m_animating;
    //for animation
    float m_cx, m_cy;
    //speed
    float m_speed;
    float m_offest;
    //destination
    QPointF m_src;
    QPointF m_dest;

    //lines
    QVector<QPointF> m_old_boundary;
    QVector<QPointF> m_boundary;
    QVector<QPointF> m_curpoints;
    int m_curindex;
    //track
    QVector<QPointF> m_track;
    //ball
    int m_interval;
    int m_stroke;
    int m_radius;
    float m_ygap;;
    QTimer timer;

    //calc polygon points
    void calcPoints();
    bool  pointInLine(QPointF p, QPointF a, QPointF b);
public:
    void mousePressEvent(QMouseEvent *e);
    void startAnimation();
public slots:
    void onTimer();
signals:

};

#endif // LEFTWIDGET_H
