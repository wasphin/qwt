#include <qwt_dial.h>
#include <qwt_dial_needle.h>

class AttitudeIndicatorNeedle: public QwtDialNeedle
{
public:
    AttitudeIndicatorNeedle(const QColor &);

    virtual void draw(QPainter *, const QPointF &, double length,
        double direction, QPalette::ColorGroup) const;
};

class AttitudeIndicator: public QwtDial
{
    Q_OBJECT

public:
    AttitudeIndicator(QWidget *parent = NULL);

    double angle() const { return value(); }
    double gradient() const { return d_gradient; }

public Q_SLOTS:
    void setGradient(double);
    void setAngle(double angle) { setValue(angle); }

protected:
    virtual void keyPressEvent(QKeyEvent *);

    virtual void drawScale(QPainter *, const QPointF &center,
        double radius, double origin, double arcMin, double arcMax) const;

    virtual void drawScaleContents(QPainter *painter,
        const QPointF &center, double radius) const;

private:
    double d_gradient;
};
