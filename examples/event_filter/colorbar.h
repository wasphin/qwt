/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include <qwt_global.h>
#include <qwidget.h>

class ColorBar: public QWidget
{
    Q_OBJECT

public:
    ColorBar( Qt::Orientation = Qt::Horizontal, QWidget * = NULL );

    virtual void setOrientation( Qt::Orientation );
    Qt::Orientation orientation() const { return d_orientation; }

    void setRange( const QColor &light, const QColor &dark );
    void setLight( const QColor &light );
    void setDark( const QColor &dark );

    QColor light() const { return d_light; }
    QColor dark() const { return d_dark; }

Q_SIGNALS:
    void selected( const QColor & );

protected:
    virtual void mousePressEvent( QMouseEvent * ) QWT_OVERRIDE;
    virtual void paintEvent( QPaintEvent * ) QWT_OVERRIDE;

    void drawColorBar( QPainter *, const QRect & ) const;

private:
    Qt::Orientation d_orientation;
    QColor d_light;
    QColor d_dark;
};
