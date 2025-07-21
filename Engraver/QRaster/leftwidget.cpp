#include "leftwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QRect>
LeftWidget::LeftWidget(QWidget *parent) : QWidget(parent)
{
   m_finish = false;
   m_radius = 3;
   m_stroke = 1;
   m_speed = 3;
   m_animating = false;
   m_ygap = 15;
   m_interval = 30;
   m_angle = 0.2;
}

float calcDist(QPointF a, QPointF b)
{
    return qSqrt( (a.x() - b.x()) * (a.x() - b.x()) + (a.y() - b.y()) * (a.y() - b.y()));
}
void LeftWidget::paintEvent(QPaintEvent *event)
{
    QPainter pa(this);

    //test
//    ALine line(QPointF(100,100),-0.2);
//    line.draw(pa);

//    ALine line2(QPointF(100,100),QPointF(0,0));
//    line2.draw(pa);
    //
    int i;
    for(i = 0; i<m_points.size(); i++)
    {
        QPointF p = m_points[i];
        pa.fillRect(QRect(p.x()-5,p.y()-5,10,10), Qt::red);
    }

    if(m_finish)
    {
        QPolygonF po(m_points);
        pa.drawPolygon(po);


    }

    if(m_animating)
    {
        //track track
        int i;
        pa.setPen( QPen(Qt::red,1, Qt::DashLine));
        for(i = 0; i<m_track.size()-1; i++)
        {
            pa.drawLine( m_track[i].x(), m_track[i].y(),m_track[i+1].x(), m_track[i+1].y() );

        }
        pa.setPen( QPen(Qt::black, m_stroke));
        pa.setBrush( Qt::blue);
        pa.drawEllipse(m_cx - m_radius, m_cy - m_radius, m_radius*2, m_radius*2);

        pa.drawEllipse(m_dest.x() - m_radius, m_dest.y() - m_radius, m_radius*2, m_radius*2);
    }
}




void LeftWidget::startAnimation()
{
    if(m_points.size() < 3) return;
    m_track.clear();
    //calc top most, bottom most
    m_last = m_points[0];
    int i;
    int top_index = 0;
    int ps = m_points.size();


    for(i = 0; i<ps; i++)
    {
        if(calcDist(QPointF(0,0), m_points[i] ) < calcDist(QPointF(0,0), m_points[top_index] ))
            top_index = i;

    }

    //calc left one connected
    int left_index;
    int right_index;

    if( m_points[(top_index+1)%ps].x() > m_points[(top_index+ps - 1)%ps].x() )
    {
        left_index = (top_index+ps - 1)%ps;
        right_index = (top_index + 1)%ps;
    }
    else{
        right_index = (top_index+ps - 1)%ps;
        left_index = (top_index + 1)%ps;

    }



    ALine first_line = ALine(m_points[top_index], m_points[left_index]);
    ALine last_line = ALine(m_points[right_index], m_points[top_index]);

    float y = m_points[top_index].y() + m_ygap ;

//    if(m_points[left_index].y() < m_points[top_index].y() )
//        y = m_points[top_index].y() - m_ygap ;
    float x = first_line.x(y);
    //configure first line

    //

    m_track.append( QPointF(x,y));
    m_track.append( QPointF(x,y));

    m_cx = x;
    m_cy = y;


    ALine first_path  = ALine(QPointF(x,y), m_angle);

    int farest = -1;

    for(i = 0; i<m_points.size(); i++)
    {
        if(i != top_index)
        {
            if(first_path.distance( m_points[i]) > farest)
            {
                farest = first_path.distance( m_points[i]);
                m_last = m_points[i];
            }
        }
    }




    m_src = QPointF(x,y);
    m_dest = findDest(m_src, RIGHT);
    m_offset = 0;

    m_dir = HORZ;
    m_side = RIGHT;


    m_animating = true;
    update();
    timer.start(m_interval);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()) );

}

QPointF LeftWidget::findDest(QPointF src, int side)
{
    int i;
    ALine line(src, m_angle);
    float max_dist = -9999;
    int pc = m_points.size();
    QPointF ret;
    for(i = 0; i<pc; i++)
    {
        int j = (i+1) % pc;
        ALine cur_line(m_points[i], m_points[j]);
        QPointF mp = cur_line.meetPoint(line);

        QRectF bRect = QRectF(m_points[i], m_points[j] );
        float d1 = calcDist(m_points[i], mp);
        float d2 = calcDist(m_points[j], mp);
        float d = calcDist(m_points[i], m_points[j]);
        if( qAbs(d1+d2-d) < 1 && ((side == LEFT && mp.x() < m_cx) || (side == RIGHT && mp.x() > m_cx)) )
        {
            int dist = calcDist(mp, src);
            if(dist > max_dist)
            {
                ret = mp;
                max_dist = dist;
            }
        }
    }
    if(max_dist == -9999)
        return QPointF(-1, -1);
    return ret;
}

void LeftWidget::onTimer()
{
    if(calcDist( QPointF(m_cx, m_cy), m_last) <= 2*m_speed )
    {
        timer.stop();
    }
    //check if arrived
    float dist =calcDist(QPointF(m_cx, m_cy), m_dest);

    if( 0 < dist && dist  <= m_speed)
    {
        m_cx = m_dest.x();
        m_cy = m_dest.y();
    }
    else if(dist == 0)
    {
        m_dir = (int)m_dir * -1; //change direction
        m_track.append(QPointF(m_cx, m_cy));
        m_track.append(QPointF(m_cx, m_cy));
        if(m_dir == HORZ)
        {
            m_side *= -1;

            m_offset = 0;
            m_src = QPointF(m_cx, m_cy);
            m_dest = findDest(m_src, m_side);

        }else{ //vertical
            m_src = QPointF(m_cx, m_cy);
            m_offset = 0;

            m_dest = QPointF(m_cx, m_cy + m_ygap);

        }
        if(m_dest == QPointF(-1,-1))
        {
            timer.stop();
            return;
        }
    }else{
        if(m_dir == HORZ)
        {
            m_offset += m_speed;
            m_cx = m_src.x() + m_offset * qCos(m_angle) * m_side;
            m_cy = m_src.y() - m_offset * qSin(m_angle) * m_side;

        }
        if(m_dir == VERT)
        {
            m_cy +=  m_speed;


        }
        m_track.pop_back();
        m_track.append(QPointF(m_cx, m_cy));
    }
    update();
}
void LeftWidget::mousePressEvent(QMouseEvent *e)
{
    if(m_animating || m_finish) return;
    m_points.append(e->pos());
    update();
}
