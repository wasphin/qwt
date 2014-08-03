/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_spline_local.h"
#include <qmath.h>

static inline double qwtSlope( const QPointF &p1, const QPointF &p2 )
{
    const double dx = p2.x() - p1.x();
    return dx ? ( p2.y() - p1.y() ) / dx : 0.0;
}

static inline double qwtChordal( const QPointF &p1, const QPointF &p2 )
{
   const double dx = p1.x() - p2.x();
   const double dy = p1.y() - p2.y();

   return qSqrt( dx * dx + dy * dy );
}

static inline void qwtCubicToP( const QPointF &p1, double m1,
    const QPointF &p2, double m2, QPainterPath &path )
{
    const double dx = ( p2.x() - p1.x() ) / 3.0;

    path.cubicTo( p1.x() + dx, p1.y() + m1 * dx,
        p2.x() - dx, p2.y() - m2 * dx,
        p2.x(), p2.y() );
}

static inline double qwtAkima( double s1, double s2, double s3, double s4 )
{
    if ( ( s1 == s2 ) && ( s3 == s4 ) )
    {
        return 0.5 * ( s2 + s3 );
    }

    const double ds12 = qAbs( s2 - s1 );
    const double ds34 = qAbs( s4 - s3 );

    return ( s2 * ds34 + s3 * ds12 ) / ( ds12 + ds34 );
}

static inline double qwtHarmonicMean( 
    double dx1, double dy1, double dx2, double dy2 )
{
    if ( ( dy1 > 0.0 ) == ( dy2 > 0.0 ) )
    {
        if ( ( dy1 != 0.0 ) && ( dy2 != 0.0 ) )
            return 2.0 / ( dx1 / dy1 + dx2 / dy2 );
    }

    return 0.0;
}

static inline double qwtHarmonicMean( double s1, double s2 )
{
    if ( ( s1 > 0.0 ) == ( s2 > 0.0 ) )
    {
        if ( ( s1 != 0.0 ) && ( s2 != 0.0 ) )
            return 2 / ( 1 / s1 + 1 / s2 );
    }

    return 0.0;
}

static inline void qwtLocalEndpoints( const QPolygonF &points,
    QwtSplineLocal::Type type, double tension, double &slopeStart, double &slopeEnd )
{
    slopeStart = slopeEnd = 0.0;

    const int n = points.size();
    if ( type == QwtSplineLocal::HarmonicMean )
    {
        if ( n >= 3 )
        {
            const double s1 = qwtSlope( points[0], points[1] );
            const double s2 = qwtSlope( points[1], points[2] );
            const double s3 = qwtSlope( points[n-3], points[n-2] );
            const double s4 = qwtSlope( points[n-2], points[n-1] );

            slopeStart = 1.5 * s1 - 0.5 * qwtHarmonicMean( s1, s2 );
            slopeEnd = 1.5 * s4 - 0.5 * qwtHarmonicMean( s3, s4 );
        }
    }
    else
    {
        if ( n >= 2 )
        {
            slopeStart = qwtSlope( points[0], points[1] );
            slopeEnd = qwtSlope( points[n-2], points[n-1] );
        }
    }

    slopeStart *= ( 1.0 - tension );
    slopeEnd *= ( 1.0 - tension );
}

static QPainterPath qwtPathCardinal( const QPolygonF &points, 
    double tension, double slopeStart, double slopeEnd )
{
    const double s = 1.0 - tension;
    const int size = points.size();

    const QPointF *p = points.constData();

    QPainterPath path;
    path.moveTo( p[0] );

    double m1 = slopeStart;
    for ( int i = 1; i < size - 1; i++ )
    {
        const double m2 = s * qwtSlope( p[i-1], p[i+1] );
        qwtCubicToP( p[i-1], m1, p[i], m2, path );

        m1 = m2;
    }

    qwtCubicToP( p[size-2], m1, p[size-1], slopeEnd, path );

    return path;
}

static QVector<double> qwtSlopesCardinal( const QPolygonF &points, 
    double tension, double slopeStart, double slopeEnd )
{
    const double s = 1.0 - tension;

    const int size = points.size();
    const QPointF *p = points.constData();

    QVector<double> m(size);
    double *md = m.data();

    md[0] = slopeStart;

    for ( int i = 1; i < size - 1; i++ )
        md[i] = s * qwtSlope( p[i-1], p[i+1] );

    md[size-1] = slopeEnd;

    return m;
}

static QPainterPath qwtPathAkima( const QPolygonF &points, 
    double tension, double slopeStart, double slopeEnd )
{
    const double s = 1.0 - tension;

    const int size = points.size();
    const QPointF *p = points.constData();

    QPainterPath path;
    path.moveTo( p[0] );

    double slope1 = slopeStart;
    double slope2 = qwtSlope( p[0], p[1] );
    double slope3 = qwtSlope( p[1], p[2] );

    double m1 = s * slope1;

    for ( int i = 0; i < size - 3; i++ )
    {
        const double slope4 = qwtSlope( p[i+2],  p[i+3] );

        const double m2 = s * qwtAkima( slope1, slope2, slope3, slope4 );
        qwtCubicToP( p[i], m1, p[i+1], m2, path );

        slope1 = slope2;
        slope2 = slope3;
        slope3 = slope4;

        m1 = m2;
    }

    const double m2 = s * qwtAkima( slope1, slope2, slope3, slopeEnd );

    qwtCubicToP( p[size - 3], m1, p[size - 2], m2, path );
    qwtCubicToP( p[size - 2], m2, p[size - 1], slopeEnd, path );

    return path;
}

static QVector<double> qwtSlopesAkima( const QPolygonF &points, 
    double tension, double slopeStart, double slopeEnd )
{
    const double s = 1.0 - tension;

    const int size = points.size();
    const QPointF *p = points.constData();

    QVector<double> m(size);

    double slope1 = slopeStart;
    double slope2 = qwtSlope( p[0], p[1] );
    double slope3 = qwtSlope( p[1], p[2] );

    double *md = m.data();

    md[0] = s * slope1;

    for ( int i = 0; i < size - 3; i++ )
    {
        const double slope4 = qwtSlope( p[i+2], p[i+3] );

        md[i+1] = s * qwtAkima( slope1, slope2, slope3, slope4 );

        slope1 = slope2;
        slope2 = slope3;
        slope3 = slope4;
    }

    md[size-2] = s * qwtAkima( slope1, slope2, slope3, slopeEnd );
    md[size-1] = slopeEnd;

    return m;
}

static QVector<double> qwtSlopesHarmonicMean( const QPolygonF &points, 
    double tension, double slopeStart, double slopeEnd )
{
    const double s = 1.0 - tension;

    const int size = points.size();
    const QPointF *p = points.constData();

    QVector<double> m(size);

    double dx1 = p[1].x() - p[0].x();
    double dy1 = p[1].y() - p[0].y();

    m[0] = slopeStart;

    for ( int i = 1; i < size - 1; i++ )
    {
        const double dx2 = p[i+1].x() - p[i].x();
        const double dy2 = p[i+1].y() - p[i].y();

        m[1] = s * qwtHarmonicMean( dx1, dy1, dx2, dy2 );

        dx1 = dx2;
        dy1 = dy2;
    }

    m[size-1] = slopeEnd;

    return m; 
}

static QPainterPath qwtPathHarmonicMean( const QPolygonF &points, 
    double tension, double slopeStart, double slopeEnd )
{
    const double s = 1.0 - tension;

    const int size = points.size();
    const QPointF *p = points.constData();

    QPainterPath path;
    path.moveTo( p[0] );

    double dx1 = p[1].x() - p[0].x();
    double dy1 = p[1].y() - p[0].y();
    double m1 = slopeStart;

    for ( int i = 1; i < size - 1; i++ )
    {
        const double dx2 = p[i+1].x() - p[i].x();
        const double dy2 = p[i+1].y() - p[i].y();

        const double m2 = s * qwtHarmonicMean( dx1, dy1, dx2, dy2 );
        path.cubicTo( p[i-1] + QPointF( dx1, dx1 * m1 ) / 3.0,
            p[i] - QPointF( dx1, dx1 * m2 ) / 3.0, p[i] );

        dx1 = dx2;
        dy1 = dy2;
        m1 = m2;
    }

    path.cubicTo( p[size - 2] + QPointF( dx1, dx1 * m1 ) / 3.0,
        p[size - 1] - QPointF( dx1, dx1 * slopeEnd ) / 3.0, p[size - 1] );

    return path; 
}

QwtSplineLocal::QwtSplineLocal( Type type, double tension ):
    d_type( type ),
    d_tension( 0.0 )
{
    setTension( tension );
}

QwtSplineLocal::~QwtSplineLocal()
{
}

void QwtSplineLocal::setTension( double tension )
{
    d_tension = qBound( 0.0, tension, 1.0 );
}

double QwtSplineLocal::tension() const
{
    return d_tension;
}

QPainterPath QwtSplineLocal::pathP( const QPolygonF &points ) const
{
    if ( parametrization() == ParametrizationX )
    {
        double slopeStart, slopeEnd;
        qwtLocalEndpoints( points, d_type, d_tension, slopeStart, slopeEnd );
    
        return pathClampedX( points, slopeStart, slopeEnd );
    }

    return QwtSplineC1::pathP( points );
}

QPainterPath QwtSplineLocal::pathClampedX( const QPolygonF &points,
    double slopeStart, double slopeEnd ) const
{
    QPainterPath path;

    const int size = points.size();
    if ( size == 0 )
        return path;

    if ( size == 1 )
    {
        path.moveTo( points[0] );
        return path;
    }

    if ( size == 2 )
    {
        path.moveTo( points[0] );
        qwtCubicToP( points[0], slopeStart, points[1], slopeEnd, path );

        return path;
    }

    switch( d_type )
    {
        case QwtSplineLocal::Cardinal:
        {
            path = qwtPathCardinal( points, d_tension, slopeStart, slopeEnd );
            break;
        }
        case QwtSplineLocal::ParabolicBlending:
        {
            // Bessel
            break;
        }
        case QwtSplineLocal::Akima:
        {
            path = qwtPathAkima( points, d_tension, slopeStart, slopeEnd );
            break;
        }
        case QwtSplineLocal::HarmonicMean:
        {
            path = qwtPathHarmonicMean( points, d_tension, slopeStart, slopeEnd );
            break;
        }
        case QwtSplineLocal::PChip:
        {
            break;
        }
        default:
            break;
    }

    return path;
}
    
QVector<double> QwtSplineLocal::slopesX( const QPolygonF &points ) const
{
    double slopeStart, slopeEnd;
    qwtLocalEndpoints( points, d_type, d_tension, slopeStart, slopeEnd );

    return slopesClampedX( points, slopeStart, slopeEnd );
}

QVector<double> QwtSplineLocal::slopesClampedX( const QPolygonF &points, 
    double slopeStart, double slopeEnd ) const
{
    const int size = points.size();
    if ( size <= 1 )
        return QVector<double>();

    QVector<double> m;

    if ( size == 2 )
    {
        m.resize(2);

        m[0] = slopeStart;
        m[1] = slopeEnd;

        return m;
    }

    switch( d_type )
    {
        case QwtSplineLocal::Cardinal:
        {
            m = qwtSlopesCardinal( points, d_tension, slopeStart, slopeEnd );
            break;
        }
        case QwtSplineLocal::ParabolicBlending:
        {
            // Bessel
            break;
        };
        case QwtSplineLocal::Akima:
        {
            m = qwtSlopesAkima( points, d_tension, slopeStart, slopeEnd );
            break;
        }
        case QwtSplineLocal::HarmonicMean:
        {
            m = qwtSlopesHarmonicMean( points, d_tension, slopeStart, slopeEnd );
            break;
        }
        case QwtSplineLocal::PChip:
        {
            break;
        }
        default:
            break;
    }

    return m;
}