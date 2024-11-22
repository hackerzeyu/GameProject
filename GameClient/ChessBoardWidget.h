#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class ChessBoardWidget : public QWidget
{
    Q_OBJECT

public:
    ChessBoardWidget(QTcpSocket* socket,QString m_name,QString roomName,bool flag,QWidget *parent = nullptr);
    ~ChessBoardWidget();
    void init();
    void initUI();
    void drawChessBoard();
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool GameJudge(QVector<QPoint> &chessPositions,QPoint &pos);
public slots:
    void render(int x,int y);
private:
    QVector<QPoint> whiteChessPositions;
    QVector<QPoint> blackChessPositions;
    int m_index;
    bool m_flag;
    QString m_name;
private:
    Ui::Widget *ui;
    QTcpSocket* m_socket;
    QString m_roomName;
};
#endif // CHESSBOARDWIDGET_H
