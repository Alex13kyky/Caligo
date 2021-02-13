#include "videowidget.h"

VideoWidget::VideoWidget(QWidget *parent) : QVideoWidget(parent)
// Inherits QVideoWidget. Needs only for emit signals.
{
}

void VideoWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}
